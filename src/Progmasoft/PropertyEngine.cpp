// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Strategy/Engine.hpp>

#include <Progmasoft/Runner/Files.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

namespace Progmasoft::Catch3::Detail
{

    namespace
    {

        /// Stores failing examples so that the next run replays them first.
        ///
        /// One file per property, named by a hash of the property name, with
        /// one replay token per line. The database never throws: an
        /// unreadable or unwritable directory simply behaves as an empty
        /// one, since a test must not fail because of its cache.
        class ExampleDatabase final
        {
        public:
            explicit ExampleDatabase(const PropertyOptions &options)
            {
                std::string directory = options.DatabasePath;
                if (directory.empty())
                {
                    // The value only names a directory; it is never logged.
                    directory = ReadEnvironment("CATCH3_PROPERTY_DATABASE");
                }
                if (directory.empty())
                {
                    return;
                }
                std::string name = options.Name;
                if (name.empty())
                {
                    name = DefaultPropertyName();
                }
                if (name.empty())
                {
                    name = "unnamed property";
                }
                file_ = std::filesystem::path(directory) / (Hash(name) + ".txt");
            }

            [[nodiscard]] std::vector<std::string>
            Load() const
            {
                std::vector<std::string> tokens;
                if (file_.empty())
                {
                    return tokens;
                }
                std::ifstream input(file_);
                std::string line;
                while (std::getline(input, line))
                {
                    if (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    if (!line.empty())
                    {
                        tokens.push_back(line);
                    }
                }
                return tokens;
            }

            /// Changes the stored tokens under a lock: the file is read
            /// again, `change` edits what it holds, and the result replaces
            /// the file in one rename. Workers of a parallel run, retries and
            /// repeats of one property may write at the same time; none of
            /// them loses what another stored.
            template<typename Change>
            void
            Update(Change change) const
            {
                if (file_.empty())
                {
                    return;
                }
                std::error_code error;
                std::filesystem::create_directories(file_.parent_path(), error);
                const Runner::FileLock lock(file_, std::chrono::seconds(2),
                                            std::chrono::seconds(10));
                auto tokens = Load();
                change(tokens);
                if (tokens.empty())
                {
                    std::filesystem::remove(file_, error);
                    return;
                }
                const auto temporary = std::filesystem::path(
                    file_.string() + "." + Runner::UniqueName() + ".tmp");
                {
                    std::ofstream output(temporary,
                                         std::ios::binary | std::ios::trunc);
                    for (const auto &token : tokens)
                    {
                        output << token << '\n';
                    }
                    if (!output)
                    {
                        output.close();
                        std::filesystem::remove(temporary, error);
                        return;
                    }
                }
                // rename replaces the target atomically on every platform
                // the library supports, so a reader sees the old or the new
                // file, never a half-written one.
                std::filesystem::rename(temporary, file_, error);
                if (error)
                {
                    std::filesystem::remove(temporary, error);
                }
            }

        private:
            [[nodiscard]] static std::string
            ReadEnvironment(const char *name)
            {
#if defined(_WIN32)
                // The CRT deprecates getenv in favour of the copying form.
                char *value = nullptr;
                std::size_t length = 0;
                if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
                {
                    return {};
                }
                std::string result(value);
                std::free(value);
                return result;
#else
                const char *value = std::getenv(name);
                return value == nullptr ? std::string() : std::string(value);
#endif
            }

            [[nodiscard]] static std::string
            Hash(std::string_view text)
            {
                // FNV-1a 64: stable across platforms and library versions.
                std::uint64_t hash = 0xCBF29CE484222325ULL;
                for (const char character : text)
                {
                    hash ^= static_cast<unsigned char>(character);
                    hash *= 0x100000001B3ULL;
                }
                std::ostringstream name;
                name << std::hex << hash;
                return name.str();
            }

            std::filesystem::path file_;
        };

        struct LabelCount final
        {
            std::size_t Count = 0;
            double Required = 0.0;
        };

        class Engine final
        {
        public:
            Engine(const PropertyOptions &options, const TrialRunner &runner)
                : options_(options)
                , runner_(runner)
                , database_(options)
            {}

            PropertyResult
            Run()
            {
                if (options_.Replay.has_value())
                {
                    return RunReplay(*options_.Replay);
                }
                if (options_.Trials == 0)
                {
                    return Invalid("Trials must be greater than zero");
                }
                if (auto stored = ReplayDatabase())
                {
                    return std::move(*stored);
                }
                if (auto failure = RunTrials())
                {
                    return std::move(*failure);
                }
                if (auto failure = SearchTargets())
                {
                    return std::move(*failure);
                }
                return Finish();
            }

        private:
            struct Execution final
            {
                TrialOutcome Outcome;
                std::vector<std::uint64_t> Choices;
            };

            Execution
            Execute(ChoiceSource source, bool describe)
            {
                Execution execution{ runner_(source, describe), {} };
                execution.Choices = source.Choices();
                return execution;
            }

            Execution
            Replay(const std::vector<std::uint64_t> &choices,
                   std::size_t size,
                   bool describe)
            {
                return Execute(ChoiceSource(std::span(choices), size), describe);
            }

            static PropertyResult
            Invalid(std::string message)
            {
                PropertyResult result;
                result.Status = PropertyStatus::kInvalidConfiguration;
                result.Error = std::move(message);
                return result;
            }

            PropertyResult
            RunReplay(const std::string &token)
            {
                const auto decoded = DecodeChoices(token);
                if (!decoded.has_value())
                {
                    return Invalid("PropertyOptions::Replay is not a replay "
                                   "token: "
                                   + token);
                }
                auto execution = Replay(decoded->Choices, decoded->Size, true);
                trialsRun_ = 1;
                if (execution.Outcome.Verdict == TrialVerdict::kFailed)
                {
                    return Report(std::move(execution),
                                  decoded->Size,
                                  0,
                                  1,
                                  0);
                }
                PropertyResult result;
                result.TrialsRun = 1;
                if (execution.Outcome.Verdict == TrialVerdict::kErrored)
                {
                    result.Status = PropertyStatus::kErrored;
                    result.Error = execution.Outcome.Reason;
                }
                return result;
            }

            /// Replays the stored examples of this property. One that still
            /// fails is shrunk further and reported; the others are dropped.
            std::optional<PropertyResult>
            ReplayDatabase()
            {
                const auto tokens = database_.Load();
                for (const auto &token : tokens)
                {
                    const auto decoded = DecodeChoices(token);
                    if (!decoded.has_value())
                    {
                        continue;
                    }
                    auto execution
                        = Replay(decoded->Choices, decoded->Size, false);
                    if (execution.Outcome.Verdict == TrialVerdict::kFailed)
                    {
                        ++trialsRun_;
                        return ShrinkAndReport(std::move(execution),
                                               decoded->Size,
                                               0,
                                               trialsRun_);
                    }
                }
                if (!tokens.empty())
                {
                    // Only the examples checked here are dropped; another
                    // process may have stored new ones meanwhile.
                    database_.Update([&](std::vector<std::string> &stored) {
                        std::erase_if(stored, [&](const std::string &token) {
                            return std::find(tokens.begin(), tokens.end(), token)
                                   != tokens.end();
                        });
                    });
                }
                return std::nullopt;
            }

            std::optional<PropertyResult>
            RunTrials()
            {
                const std::size_t allowedDiscards
                    = options_.Trials * options_.MaxDiscardRatio;
                std::size_t attempt = 0;
                while (validTrials_ < options_.Trials)
                {
                    const std::uint64_t seed
                        = Random::DeriveSeed(options_.Seed, attempt);
                    // The size grows over the first half of the trials and
                    // stays at its maximum for the second half.
                    const std::size_t size = std::min<std::size_t>(
                        100, attempt * 200U / options_.Trials);
                    ++attempt;
                    auto execution = Execute(ChoiceSource(seed, size), false);
                    ++trialsRun_;
                    switch (execution.Outcome.Verdict)
                    {
                        case TrialVerdict::kPassed:
                            Count(execution);
                            ++validTrials_;
                            break;
                        case TrialVerdict::kFailed:
                            return ShrinkAndReport(
                                std::move(execution), size, seed, trialsRun_);
                        case TrialVerdict::kErrored:
                        {
                            PropertyResult result;
                            result.Status = PropertyStatus::kErrored;
                            result.TrialsRun = trialsRun_;
                            result.Error = execution.Outcome.Reason;
                            return result;
                        }
                        case TrialVerdict::kDiscarded:
                        case TrialVerdict::kInvalid:
                            if (++discarded_ > allowedDiscards)
                            {
                                PropertyResult result;
                                result.Status = PropertyStatus::kGaveUp;
                                result.TrialsRun = trialsRun_;
                                result.Discarded = discarded_;
                                result.Error
                                    = std::to_string(discarded_)
                                      + " trials were discarded, more than "
                                        "MaxDiscardRatio allows, with only "
                                      + std::to_string(validTrials_)
                                      + " valid; loosen the filter or the "
                                        "assumption, or build the input so "
                                        "that it is valid by construction";
                                return result;
                            }
                            break;
                    }
                }
                return std::nullopt;
            }

            void
            Count(const Execution &execution)
            {
                const auto &record = execution.Outcome.Record;
                for (const auto &label : record.Labels)
                {
                    ++labels_[label].Count;
                }
                for (const auto &cover : record.Covers)
                {
                    auto &entry = covers_[cover.Label];
                    entry.Required = std::max(entry.Required, cover.Required);
                    if (cover.Held)
                    {
                        ++entry.Count;
                    }
                }
                for (const auto &target : record.Targets)
                {
                    auto found = best_.find(target.Label);
                    if (found == best_.end() || target.Value > found->second.first)
                    {
                        best_[target.Label] = { target.Value, execution.Choices };
                    }
                }
            }

            /// Hill climbing on the choices of the best input of each
            /// target: one mutated choice per step, kept when the score
            /// rises. Inputs reached this way are regular trials, so a
            /// failure among them is shrunk and reported as usual.
            std::optional<PropertyResult>
            SearchTargets()
            {
                if (best_.empty() || options_.TargetSteps == 0)
                {
                    return std::nullopt;
                }
                Random random(options_.Seed ^ 0x7A26E7ULL);
                std::vector<std::string> labels;
                for (const auto &entry : best_)
                {
                    labels.push_back(entry.first);
                }
                for (std::size_t step = 0; step < options_.TargetSteps; ++step)
                {
                    const auto &label = labels[step % labels.size()];
                    auto &[bestScore, bestChoices] = best_[label];
                    if (bestChoices.empty())
                    {
                        continue;
                    }
                    auto candidate = bestChoices;
                    auto &choice = candidate[static_cast<std::size_t>(
                        random.Below(candidate.size()))];
                    switch (random.Below(6))
                    {
                        case 0:
                            ++choice;
                            break;
                        case 1:
                            choice = choice == 0 ? 0 : choice - 1U;
                            break;
                        case 2:
                            choice = choice * 2U + 1U;
                            break;
                        case 3:
                            choice /= 2U;
                            break;
                        case 4:
                            choice = random.Below(choice + 2U);
                            break;
                        default:
                            choice = random.Next();
                            break;
                    }
                    auto execution = Replay(candidate, 100, false);
                    ++trialsRun_;
                    if (execution.Outcome.Verdict == TrialVerdict::kFailed)
                    {
                        return ShrinkAndReport(
                            std::move(execution), 100, 0, trialsRun_);
                    }
                    if (execution.Outcome.Verdict != TrialVerdict::kPassed)
                    {
                        continue;
                    }
                    for (const auto &target : execution.Outcome.Record.Targets)
                    {
                        if (target.Label == label && target.Value > bestScore)
                        {
                            bestScore = target.Value;
                            bestChoices = execution.Choices;
                        }
                    }
                }
                return std::nullopt;
            }

            PropertyResult
            ShrinkAndReport(Execution failing,
                            std::size_t size,
                            std::uint64_t trialSeed,
                            std::size_t trial)
            {
                std::size_t calls = 0;
                const auto minimized = ShrinkChoices(
                    failing.Choices,
                    options_.MaxShrinkSteps,
                    [&](const std::vector<std::uint64_t> &candidate)
                        -> std::optional<std::vector<std::uint64_t>> {
                        auto execution = Replay(candidate, size, false);
                        if (execution.Outcome.Verdict != TrialVerdict::kFailed)
                        {
                            return std::nullopt;
                        }
                        return std::move(execution.Choices);
                    },
                    calls);
                auto minimal = Replay(minimized, size, true);
                if (minimal.Outcome.Verdict != TrialVerdict::kFailed)
                {
                    // A flaky body: the minimized input passed on replay.
                    // Report the original input, which did fail.
                    minimal = Replay(failing.Choices, size, true);
                }
                return Report(std::move(minimal), size, trialSeed, trial, calls);
            }

            PropertyResult
            Report(Execution failing,
                   std::size_t size,
                   std::uint64_t trialSeed,
                   std::size_t trial,
                   std::size_t calls)
            {
                PropertyFailure failure;
                failure.RootSeed = options_.Seed;
                failure.TrialSeed = trialSeed;
                failure.Trial = trial;
                failure.ShrinkSteps = calls;
                failure.Counterexample = failing.Outcome.Description;
                failure.Reason = failing.Outcome.Reason;
                failure.Replay = EncodeChoices(size, failing.Choices);
                failure.Notes = std::move(failing.Outcome.Record.Notes);

                database_.Update([&](std::vector<std::string> &stored) {
                    if (std::find(stored.begin(), stored.end(), failure.Replay)
                        == stored.end())
                    {
                        stored.insert(stored.begin(), failure.Replay);
                    }
                });

                PropertyResult result;
                result.Status = PropertyStatus::kFailed;
                result.TrialsRun = trialsRun_;
                result.Discarded = discarded_;
                result.Failure = std::move(failure);
                return result;
            }

            PropertyResult
            Finish()
            {
                PropertyResult result;
                result.Status = PropertyStatus::kPassed;
                result.TrialsRun = trialsRun_;
                result.Discarded = discarded_;
                result.ShowStatistics = options_.ShowStatistics;
                const auto percentage = [this](std::size_t count) {
                    return validTrials_ == 0
                               ? 0.0
                               : 100.0 * static_cast<double>(count)
                                     / static_cast<double>(validTrials_);
                };
                for (const auto &[label, entry] : labels_)
                {
                    result.Statistics.push_back(
                        { label, entry.Count, percentage(entry.Count), 0.0 });
                }
                std::string missing;
                for (const auto &[label, entry] : covers_)
                {
                    const double reached = percentage(entry.Count);
                    result.Statistics.push_back(
                        { label, entry.Count, reached, entry.Required });
                    if (reached < entry.Required)
                    {
                        missing += (missing.empty() ? "" : ", ") + label;
                    }
                }
                if (!missing.empty())
                {
                    result.Status = PropertyStatus::kInsufficientCoverage;
                    result.Error = "too few trials covered " + missing;
                }
                return result;
            }

            const PropertyOptions &options_;
            const TrialRunner &runner_;
            ExampleDatabase database_;
            std::size_t trialsRun_ = 0;
            std::size_t validTrials_ = 0;
            std::size_t discarded_ = 0;
            std::map<std::string, LabelCount> labels_;
            std::map<std::string, LabelCount> covers_;
            std::map<std::string,
                     std::pair<double, std::vector<std::uint64_t>>>
                best_;
        };

    } // namespace

    PropertyResult
    RunPropertyEngine(const PropertyOptions &options, const TrialRunner &runner)
    {
        Engine engine(options, runner);
        return engine.Run();
    }

} // namespace Progmasoft::Catch3::Detail
