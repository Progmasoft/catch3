// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <algorithm>
#include <cmath>
#include <system_error>

#include "Files.hpp"
#include "History.hpp"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        constexpr int kHistoryVersion = 1;
        constexpr std::size_t kRecordsPerTest = 50;
        constexpr std::size_t kRunsKept = 100;

        /// Runs that did not finish on their own say nothing about how long
        /// the test takes.
        bool
        HasDuration(Outcome outcome)
        {
            return outcome == Outcome::kPassed || outcome == Outcome::kFailed
                   || outcome == Outcome::kFailedAsExpected
                   || outcome == Outcome::kSkipped;
        }

        bool
        Counts(Outcome outcome)
        {
            return outcome != Outcome::kNotRun && outcome != Outcome::kBlocked
                   && outcome != Outcome::kUpToDate;
        }

        JsonValue
        RecordToJson(const TestRecord &record)
        {
            JsonValue value = JsonValue::Object();
            value.Set("run", record.run);
            value.Set("outcome", OutcomeName(record.outcome));
            if (record.category != FailureCategory::kNone)
            {
                value.Set("category", CategoryName(record.category));
            }
            value.Set("duration", record.durationMicroseconds);
            value.Set("attempts", record.attempts);
            if (record.flaky)
            {
                value.Set("flaky", true);
            }
            if (!record.signature.empty())
            {
                value.Set("signature", record.signature);
            }
            if (!record.fingerprint.empty())
            {
                value.Set("fingerprint", record.fingerprint);
            }
            return value;
        }

        TestRecord
        RecordFromJson(const JsonValue &value)
        {
            TestRecord record;
            record.run = value["run"].AsInteger();
            record.outcome = OutcomeFromName(value["outcome"].AsString())
                                 .value_or(Outcome::kNotRun);
            record.category = CategoryFromName(value["category"].AsString())
                                  .value_or(FailureCategory::kNone);
            record.durationMicroseconds = value["duration"].AsInteger();
            record.attempts
                = static_cast<std::size_t>(value["attempts"].AsUnsigned());
            record.flaky = value["flaky"].AsBoolean();
            record.signature = value["signature"].AsString();
            record.fingerprint = value["fingerprint"].AsString();
            return record;
        }
    } // namespace

    const TestRecord *
    TestHistory::Last() const noexcept
    {
        for (auto record = records.rbegin(); record != records.rend(); ++record)
        {
            if (Counts(record->outcome))
            {
                return &*record;
            }
        }
        return nullptr;
    }

    std::optional<std::int64_t>
    TestHistory::DurationPercentile(double fraction) const
    {
        std::vector<std::int64_t> durations;
        for (auto record = records.rbegin();
             record != records.rend() && durations.size() < 20;
             ++record)
        {
            if (HasDuration(record->outcome))
            {
                durations.push_back(record->durationMicroseconds);
            }
        }
        if (durations.empty())
        {
            return std::nullopt;
        }
        std::sort(durations.begin(), durations.end());
        const auto position = static_cast<std::size_t>(
            std::ceil(fraction * static_cast<double>(durations.size())));
        return durations[std::min(durations.size() - 1,
                                  position == 0 ? 0 : position - 1)];
    }

    double
    TestHistory::FailureProbability() const
    {
        // A beta prior worth one run: a test without history fails with
        // probability 0.3, a common share for new code.
        double failures = 0.3;
        double total = 1.0;
        double weight = 1.0;
        std::size_t seen = 0;
        for (auto record = records.rbegin();
             record != records.rend() && seen < 30;
             ++record)
        {
            if (!Counts(record->outcome))
            {
                continue;
            }
            ++seen;
            const bool failed = IsFailure(record->outcome) || record->flaky;
            failures += failed ? weight : 0.0;
            total += weight;
            weight *= 0.8;
        }
        double probability = failures / total;
        if (const TestRecord *last = Last();
            last != nullptr && IsFailure(last->outcome))
        {
            // What failed last time most often fails again.
            probability = std::max(probability, 0.7);
        }
        return probability;
    }

    std::size_t
    TestHistory::FlakyEvents(std::size_t window) const
    {
        std::size_t events = 0;
        std::size_t seen = 0;
        const TestRecord *newer = nullptr;
        for (auto record = records.rbegin();
             record != records.rend() && seen < window;
             ++record)
        {
            if (!Counts(record->outcome))
            {
                continue;
            }
            ++seen;
            if (record->flaky)
            {
                ++events;
            }
            else if (newer != nullptr && !newer->flaky
                     && !record->fingerprint.empty()
                     && record->fingerprint == newer->fingerprint
                     && IsFailure(record->outcome) != IsFailure(newer->outcome))
            {
                // The verdict changed while nothing the test depends on did.
                ++events;
            }
            newer = &*record;
        }
        return events;
    }

    std::size_t
    TestHistory::PassStreak() const
    {
        std::size_t streak = 0;
        for (auto record = records.rbegin(); record != records.rend(); ++record)
        {
            if (!Counts(record->outcome))
            {
                continue;
            }
            if (record->outcome != Outcome::kPassed || record->flaky)
            {
                break;
            }
            ++streak;
        }
        return streak;
    }

    std::filesystem::path
    HistoryFile(const std::filesystem::path &directory,
                const std::string &binaryName)
    {
        std::string name;
        for (const char character : binaryName)
        {
            const bool safe = (character >= 'a' && character <= 'z')
                              || (character >= 'A' && character <= 'Z')
                              || (character >= '0' && character <= '9')
                              || character == '-' || character == '_'
                              || character == '.';
            name.push_back(safe ? character : '_');
        }
        return directory / (name + ".history.json");
    }

    std::string
    History::KeyOf(const TestResult &test)
    {
        if (!test.id.empty())
        {
            return test.id;
        }
        return test.className.empty() ? test.name
                                      : test.className + "::" + test.name;
    }

    const TestHistory *
    History::Find(const std::string &key) const
    {
        const auto found = tests_.find(key);
        return found == tests_.end() ? nullptr : &found->second;
    }

    Expected<History>
    History::Load(const std::filesystem::path &file)
    {
        std::error_code error;
        if (!std::filesystem::exists(file, error))
        {
            return History{};
        }
        auto text = ReadFile(file);
        if (!text)
        {
            return text.GetError();
        }
        auto json = ParseJson(*text);
        if (!json)
        {
            return json.GetError().Context(file.string());
        }
        return FromJson(*json);
    }

    Expected<History>
    History::Commit(const std::filesystem::path &file,
                    const RunResult &run,
                    const RunRecord &record,
                    const std::map<std::string, std::string> &fingerprints,
                    const QuarantinePolicy &policy,
                    std::chrono::milliseconds lockWait)
    {
        const FileLock lock{ file, lockWait };
        if (!lock.Held())
        {
            // Writing without the lock could replace a run that another
            // process is adding at the same time.
            return Error{ "another run holds the lock of " + file.string() };
        }
        auto history = Load(file);
        if (!history)
        {
            // A damaged file is set aside rather than lost.
            std::error_code error;
            std::filesystem::rename(file,
                                    file.string() + ".damaged-" + UniqueName(),
                                    error);
            history = History{};
        }
        history->Apply(run, record, fingerprints, policy);
        if (auto status
            = WriteFileAtomically(file, history->ToJson().ToPrettyString());
            !status)
        {
            return status.GetError();
        }
        return history;
    }

    void
    History::Apply(const RunResult &run,
                   RunRecord record,
                   const std::map<std::string, std::string> &fingerprints,
                   const QuarantinePolicy &policy)
    {
        record.id = nextRun_++;
        runs_.push_back(record);
        if (runs_.size() > kRunsKept)
        {
            runs_.erase(
                runs_.begin(),
                runs_.begin()
                    + static_cast<std::ptrdiff_t>(runs_.size() - kRunsKept));
        }
        for (const auto &test : run.tests)
        {
            if (test.outcome == Outcome::kNotRun)
            {
                continue;
            }
            const std::string key = KeyOf(test);
            TestHistory &history = tests_[key];
            TestRecord result;
            result.run = record.id;
            result.outcome = test.outcome;
            result.category = test.category;
            result.durationMicroseconds
                = test.attempts.empty()
                      ? 0
                      : test.attempts.back().durationMicroseconds;
            result.attempts = test.attempts.size();
            result.flaky = test.flaky;
            result.signature = test.signature;
            if (const auto found = fingerprints.find(key);
                found != fingerprints.end())
            {
                result.fingerprint = found->second;
            }
            history.records.push_back(result);
            if (history.records.size() > kRecordsPerTest)
            {
                history.records.erase(history.records.begin());
            }
            const auto failed
                = std::find_if(test.attempts.begin(),
                               test.attempts.end(),
                               [](const Attempt &attempt) {
                                   return IsFailure(attempt.outcome);
                               });
            if ((IsFailure(test.outcome) || test.flaky)
                && failed != test.attempts.end())
            {
                // The first failure, as it happened: a flaky failure is
                // worth replaying too.
                ReplayRecord replay;
                replay.run = record.id;
                replay.seed = failed->seed;
                replay.precededBy = failed->precededBy;
                replay.freshProcess = failed->freshProcess;
                history.lastFailure = replay;
            }
            else if (test.outcome == Outcome::kPassed)
            {
                history.lastFailure.reset();
            }
            // Quarantine: in after repeated flakiness, out after a streak of
            // clean passes.
            if (!history.quarantine
                && history.FlakyEvents(policy.window)
                       >= policy.flakyEventsToEnter)
            {
                history.quarantine = QuarantineRecord{
                    record.id,
                    std::to_string(history.FlakyEvents(policy.window))
                        + " flaky results in the last "
                        + std::to_string(policy.window) + " runs"
                };
            }
            else if (history.quarantine
                     && history.PassStreak() >= policy.passesToLeave)
            {
                history.quarantine.reset();
            }
        }
    }

    JsonValue
    History::ToJson() const
    {
        JsonValue value = JsonValue::Object();
        value.Set("format", "catch3-history");
        value.Set("version", kHistoryVersion);
        value.Set("nextRun", nextRun_);
        JsonValue runs = JsonValue::Array();
        for (const auto &run : runs_)
        {
            JsonValue item = JsonValue::Object();
            item.Set("id", run.id);
            item.Set("time", run.unixMilliseconds);
            item.Set("commit", run.commit);
            item.Set("binary", run.binaryFingerprint);
            item.Set("seed", run.seed);
            item.Set("tests", run.tests);
            item.Set("failed", run.failed);
            item.Set("wall", run.wallMicroseconds);
            runs.Append(std::move(item));
        }
        value.Set("runs", std::move(runs));
        JsonValue tests = JsonValue::Object();
        for (const auto &[key, history] : tests_)
        {
            JsonValue item = JsonValue::Object();
            JsonValue records = JsonValue::Array();
            for (const auto &record : history.records)
            {
                records.Append(RecordToJson(record));
            }
            item.Set("results", std::move(records));
            if (history.quarantine)
            {
                JsonValue quarantine = JsonValue::Object();
                quarantine.Set("since", history.quarantine->since);
                quarantine.Set("reason", history.quarantine->reason);
                item.Set("quarantine", std::move(quarantine));
            }
            if (history.lastFailure)
            {
                JsonValue replay = JsonValue::Object();
                replay.Set("run", history.lastFailure->run);
                replay.Set("seed", history.lastFailure->seed);
                JsonValue preceding = JsonValue::Array();
                for (const auto &name : history.lastFailure->precededBy)
                {
                    preceding.Append(name);
                }
                replay.Set("precededBy", std::move(preceding));
                replay.Set("freshProcess", history.lastFailure->freshProcess);
                item.Set("lastFailure", std::move(replay));
            }
            tests.Set(key, std::move(item));
        }
        value.Set("tests", std::move(tests));
        return value;
    }

    Expected<History>
    History::FromJson(const JsonValue &value)
    {
        if (value["format"].AsString() != "catch3-history")
        {
            return Error{ "not a Catch3 history" };
        }
        if (value["version"].AsInteger() > kHistoryVersion)
        {
            return Error{ "the history was written by a newer Catch3" };
        }
        History history;
        history.nextRun_
            = std::max<std::int64_t>(1, value["nextRun"].AsInteger());
        for (const auto &item : value["runs"].Items())
        {
            RunRecord run;
            run.id = item["id"].AsInteger();
            run.unixMilliseconds = item["time"].AsInteger();
            run.commit = item["commit"].AsString();
            run.binaryFingerprint = item["binary"].AsString();
            run.seed = static_cast<std::uint32_t>(item["seed"].AsUnsigned());
            run.tests = static_cast<std::size_t>(item["tests"].AsUnsigned());
            run.failed = static_cast<std::size_t>(item["failed"].AsUnsigned());
            run.wallMicroseconds = item["wall"].AsInteger();
            history.runs_.push_back(run);
        }
        for (const auto &[key, item] : value["tests"].Members())
        {
            TestHistory test;
            for (const auto &record : item["results"].Items())
            {
                test.records.push_back(RecordFromJson(record));
            }
            if (item["quarantine"].IsObject())
            {
                test.quarantine = QuarantineRecord{
                    item["quarantine"]["since"].AsInteger(),
                    item["quarantine"]["reason"].AsString()
                };
            }
            if (item["lastFailure"].IsObject())
            {
                const JsonValue &failure = item["lastFailure"];
                ReplayRecord replay;
                replay.run = failure["run"].AsInteger();
                replay.seed
                    = static_cast<std::uint32_t>(failure["seed"].AsUnsigned());
                for (const auto &name : failure["precededBy"].Items())
                {
                    replay.precededBy.push_back(name.AsString());
                }
                replay.freshProcess = failure["freshProcess"].AsBoolean();
                test.lastFailure = replay;
            }
            history.tests_.emplace(key, std::move(test));
        }
        return history;
    }
} // namespace Progmasoft::Catch3::Runner
