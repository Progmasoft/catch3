// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Runner.hpp>
#include <Progmasoft/Catch3/Version.hpp>
#include <Progmasoft/Catch3/XmlWriter.hpp>
#include <algorithm>
#include <Progmasoft/Catch3/Config.hpp>
#include <Progmasoft/Catch3/Session.hpp>
#include <Progmasoft/Catch3/UserConfig.hpp>
#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>
#include <Progmasoft/Catch3/Interfaces/ITestCaseRegistry.hpp>
#include <Progmasoft/Catch3/Internal/CompilerCapabilities.hpp>
#include <Progmasoft/Catch3/Internal/Context.hpp>
#include <Progmasoft/Catch3/Internal/StartupExceptionRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TestCaseRegistryImpl.hpp>
#include <chrono>
#include <cstdio>
#include <exception>
#include <map>
#include <random>

#include "Analysis.hpp"
#include "Catalog.hpp"
#include "Console.hpp"
#include "Execution.hpp"
#include "Files.hpp"
#include "Git.hpp"
#include "Hash.hpp"
#include "History.hpp"
#include "Json.hpp"
#include "Options.hpp"
#include "Planner.hpp"
#include "Process.hpp"
#include "ResultsJson.hpp"
#include "Scheduler.hpp"

namespace Progmasoft::Catch3
{
    namespace
    {
        using Runner::JsonValue;

        void
        PrintError(const std::string &message)
        {
            std::fprintf(stderr, "error: %s\n", message.c_str());
        }

        /// Problems found while the tests registered, such as two tests of
        /// the same name; reported before anything runs.
        bool
        ReportStartupErrors()
        {
#if !defined(CATCH_CONFIG_DISABLE_EXCEPTIONS)
            const auto &exceptions = Catch::getRegistryHub()
                                         .getStartupExceptionRegistry()
                                         .getExceptions();
            if (exceptions.empty())
            {
                return false;
            }
            std::fprintf(stderr, "error: the tests could not be registered:\n");
            for (const auto &exception : exceptions)
            {
                try
                {
                    std::rethrow_exception(exception);
                }
                catch (const std::exception &error)
                {
                    std::fprintf(stderr, "  %s\n", error.what());
                }
                catch (...)
                {
                    std::fprintf(stderr, "  an unknown exception\n");
                }
            }
            return true;
#else
            return false;
#endif
        }

        bool
        BazelTest()
        {
#if defined(CATCH_CONFIG_BAZEL_SUPPORT)
            return true;
#else
            return Runner::HasEnvironment("BAZEL_TEST");
#endif
        }

        /// Bazel passes what it wants through the environment; it wins over
        /// the command line, as `bazel test --test_filter` expects.
        void
        ApplyBazel(Runner::Options &options, std::vector<std::string> &notes)
        {
            const std::string xml = Runner::ReadEnvironment("XML_OUTPUT_FILE");
            if (!xml.empty() && options.junitPath.empty())
            {
                options.junitPath = xml;
            }
            const std::string filter
                = Runner::ReadEnvironment("TESTBRIDGE_TEST_ONLY");
            if (!filter.empty())
            {
                options.testSpecs = { filter };
            }
            const std::string index
                = Runner::ReadEnvironment("TEST_SHARD_INDEX");
            const std::string total
                = Runner::ReadEnvironment("TEST_TOTAL_SHARDS");
            const std::string status
                = Runner::ReadEnvironment("TEST_SHARD_STATUS_FILE");
            if (!index.empty() && !total.empty())
            {
                auto parsed = Runner::ParseOptions(
                    { "--shard=" + index + "/" + total });
                if (parsed)
                {
                    options.shard = parsed->shard;
                    if (!status.empty())
                    {
                        // Bazel needs to see that the shard was read.
                        static_cast<void>(
                            Runner::WriteFileAtomically(status, ""));
                    }
                }
                else
                {
                    notes.push_back("ignoring Bazel's shard " + index + "/"
                                    + total + ": "
                                    + parsed.GetError().Message());
                }
            }
            const std::string seed
                = Runner::ReadEnvironment("TEST_RANDOM_SEED");
            if (!seed.empty())
            {
                auto parsed = Runner::ParseOptions({ "--seed=" + seed });
                if (parsed)
                {
                    options.seed = parsed->seed;
                    options.seedGiven = true;
                }
            }
            const std::string outputs
                = Runner::ReadEnvironment("TEST_UNDECLARED_OUTPUTS_DIR");
            if (!outputs.empty() && options.jsonPath.empty())
            {
                options.jsonPath
                    = (std::filesystem::path{ outputs } / "catch3.json")
                          .string();
            }
            if (options.historyDirectory.empty()
                && !Runner::HasEnvironment("CATCH3_HISTORY"))
            {
                // A sandboxed test has nowhere lasting to keep history.
                options.historyEnabled = false;
            }
        }

        std::filesystem::path
        HistoryDirectory(const Runner::Options &options,
                         const std::filesystem::path &program)
        {
            if (!options.historyDirectory.empty())
            {
                return options.historyDirectory;
            }
            const std::string fromEnvironment
                = Runner::ReadEnvironment("CATCH3_HISTORY");
            if (!fromEnvironment.empty())
            {
                return fromEnvironment;
            }
            return program.parent_path() / ".catch3";
        }

        JsonValue
        Strings(const std::vector<std::string> &values)
        {
            JsonValue array = JsonValue::Array();
            for (const auto &value : values)
            {
                array.Append(value);
            }
            return array;
        }

        /// The discovery protocol: what `--list=json` prints. Its shape is
        /// fixed for a given version; IDE adapters and CMake read it.
        std::string
        ListJson(const Runner::Catalog &catalog,
                 const std::vector<std::size_t> &matched,
                 const std::string &programName)
        {
            JsonValue list = JsonValue::Object();
            list.Set("format", "catch3-tests");
            list.Set("version", 1);
            list.Set("binary", programName);
            list.Set("catch3", kVersion);
            JsonValue tests = JsonValue::Array();
            for (const std::size_t index : matched)
            {
                const auto &entry = catalog.Tests()[index];
                JsonValue test = JsonValue::Object();
                test.Set("id", entry.key);
                test.Set("name", entry.name);
                test.Set("className", entry.className);
                test.Set("file", entry.file);
                test.Set("line", entry.line);
                test.Set("tags", Strings(entry.tags));
                test.Set("hidden", entry.hidden);
                test.Set("serial", entry.serial);
                test.Set("isolated", entry.isolated);
                test.Set("quarantined", entry.quarantined);
                test.Set("resources", Strings(entry.resources));
                test.Set("dependsOn", Strings(entry.dependsOn));
                if (entry.timeout)
                {
                    test.Set("timeoutSeconds",
                             static_cast<double>(entry.timeout->count())
                                 / 1000.0);
                }
                if (entry.retries)
                {
                    test.Set("retries", *entry.retries);
                }
                tests.Append(std::move(test));
            }
            list.Set("tests", std::move(tests));
            return list.ToPrettyString();
        }

        int
        List(const Runner::Options &options,
             const Runner::Catalog &catalog,
             const std::vector<std::size_t> &matched,
             const std::string &programName)
        {
            if (options.mode == Runner::Mode::kListTags)
            {
                std::map<std::string, std::size_t> counts;
                for (const std::size_t index : matched)
                {
                    for (const auto &tag : catalog.Tests()[index].tags)
                    {
                        ++counts[tag];
                    }
                }
                for (const auto &[tag, count] : counts)
                {
                    std::printf("%6zu  [%s]\n", count, tag.c_str());
                }
                std::printf("%zu tag(s)\n", counts.size());
                return 0;
            }
            switch (options.listFormat)
            {
                case Runner::ListFormat::kJson:
                    std::fputs(ListJson(catalog, matched, programName).c_str(),
                               stdout);
                    break;
                case Runner::ListFormat::kNames:
                    for (const std::size_t index : matched)
                    {
                        std::printf("%s\n",
                                    catalog.Tests()[index].name.c_str());
                    }
                    break;
                case Runner::ListFormat::kText:
                    for (const std::size_t index : matched)
                    {
                        const auto &entry = catalog.Tests()[index];
                        std::string tags;
                        for (const auto &tag : entry.tags)
                        {
                            tags += "[" + tag + "]";
                        }
                        std::printf("%s\n    %s:%zu\n",
                                    entry.name.c_str(),
                                    entry.file.c_str(),
                                    entry.line);
                        if (!tags.empty())
                        {
                            std::printf("    %s\n", tags.c_str());
                        }
                    }
                    std::printf("%zu matching test(s)\n", matched.size());
                    break;
            }
            return 0;
        }

        std::string
        FormatMicroseconds(std::int64_t microseconds)
        {
            char buffer[32];
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%.3f s",
                          static_cast<double>(microseconds) / 1e6);
            return buffer;
        }

        /// What the history says about the tests: `--analytics`.
        int
        Analytics(const Runner::Options &options,
                  const Runner::History &history,
                  const std::filesystem::path &file)
        {
            struct Row
            {
                std::string key;
                double failureRate = 0.0;
                std::size_t runs = 0;
                std::size_t flakyEvents = 0;
                std::int64_t median = 0;
                std::int64_t recentMedian = 0;
                std::int64_t olderMedian = 0;
                std::string quarantine;
            };
            std::vector<Row> rows;
            for (const auto &[key, test] : history.Tests())
            {
                Row row;
                row.key = key;
                std::size_t failures = 0;
                std::vector<std::int64_t> recent;
                std::vector<std::int64_t> older;
                std::size_t position = 0;
                for (auto record = test.records.rbegin();
                     record != test.records.rend();
                     ++record, ++position)
                {
                    ++row.runs;
                    failures += IsFailure(record->outcome) ? 1 : 0;
                    if (record->outcome == Outcome::kPassed)
                    {
                        (position < 5 ? recent : older)
                            .push_back(record->durationMicroseconds);
                    }
                }
                const auto median = [](std::vector<std::int64_t> values) {
                    if (values.empty())
                    {
                        return std::int64_t{ 0 };
                    }
                    std::sort(values.begin(), values.end());
                    return values[values.size() / 2];
                };
                row.failureRate = row.runs == 0
                                      ? 0.0
                                      : static_cast<double>(failures)
                                            / static_cast<double>(row.runs);
                row.flakyEvents = test.FlakyEvents(20);
                row.median = test.DurationPercentile(0.5).value_or(0);
                row.recentMedian = median(recent);
                row.olderMedian = median(older);
                if (test.quarantine)
                {
                    row.quarantine = test.quarantine->reason;
                }
                rows.push_back(std::move(row));
            }
            const auto top = [&](auto key, std::size_t count) {
                std::vector<const Row *> ordered;
                for (const auto &row : rows)
                {
                    ordered.push_back(&row);
                }
                std::stable_sort(ordered.begin(),
                                 ordered.end(),
                                 [&](const Row *left, const Row *right) {
                                     return key(*left) > key(*right);
                                 });
                ordered.resize(std::min(count, ordered.size()));
                return ordered;
            };
            const auto flaky = top(
                [](const Row &row) {
                    return static_cast<double>(row.flakyEvents);
                },
                10);
            const auto failing = top(
                [](const Row &row) {
                    return row.failureRate;
                },
                10);
            const auto slowest = top(
                [](const Row &row) {
                    return static_cast<double>(row.median);
                },
                10);
            const auto slower = top(
                [](const Row &row) {
                    return row.olderMedian > 0
                               ? static_cast<double>(row.recentMedian)
                                     / static_cast<double>(row.olderMedian)
                               : 0.0;
                },
                10);

            if (options.analyticsJson)
            {
                JsonValue report = JsonValue::Object();
                report.Set("format", "catch3-analytics");
                report.Set("version", 1);
                report.Set("runs", history.Runs().size());
                report.Set("tests", rows.size());
                JsonValue tests = JsonValue::Array();
                for (const auto &row : rows)
                {
                    JsonValue item = JsonValue::Object();
                    item.Set("id", row.key);
                    item.Set("runs", row.runs);
                    item.Set("failureRate", row.failureRate);
                    item.Set("flakyEvents", row.flakyEvents);
                    item.Set("medianMicroseconds", row.median);
                    item.Set("recentMedianMicroseconds", row.recentMedian);
                    item.Set("olderMedianMicroseconds", row.olderMedian);
                    if (!row.quarantine.empty())
                    {
                        item.Set("quarantine", row.quarantine);
                    }
                    tests.Append(std::move(item));
                }
                report.Set("tests", std::move(tests));
                std::fputs(report.ToPrettyString().c_str(), stdout);
                return 0;
            }
            std::printf("history: %s\n%zu run(s), %zu test(s)\n",
                        file.string().c_str(),
                        history.Runs().size(),
                        rows.size());
            std::printf("\nflaky (last 20 runs):\n");
            for (const Row *row : flaky)
            {
                if (row->flakyEvents > 0)
                {
                    std::printf("  %3zu  %s\n",
                                row->flakyEvents,
                                row->key.c_str());
                }
            }
            std::printf("\nin quarantine:\n");
            for (const auto &row : rows)
            {
                if (!row.quarantine.empty())
                {
                    std::printf("  %s: %s\n",
                                row.key.c_str(),
                                row.quarantine.c_str());
                }
            }
            std::printf("\nfailing most often:\n");
            for (const Row *row : failing)
            {
                if (row->failureRate > 0.0)
                {
                    std::printf("  %3.0f%%  %s\n",
                                row->failureRate * 100.0,
                                row->key.c_str());
                }
            }
            std::printf("\nslowest (median):\n");
            for (const Row *row : slowest)
            {
                std::printf("  %10s  %s\n",
                            FormatMicroseconds(row->median).c_str(),
                            row->key.c_str());
            }
            std::printf(
                "\ngetting slower (last 5 runs against the 15 before):\n");
            for (const Row *row : slower)
            {
                if (row->olderMedian > 0
                    && row->recentMedian > row->olderMedian * 3 / 2)
                {
                    std::printf("  %10s -> %-10s  %s\n",
                                FormatMicroseconds(row->olderMedian).c_str(),
                                FormatMicroseconds(row->recentMedian).c_str(),
                                row->key.c_str());
                }
            }
            return 0;
        }

        std::filesystem::path
        SourceDirectory(const Runner::Catalog &catalog)
        {
            for (const auto &entry : catalog.Tests())
            {
                const std::filesystem::path file{ entry.file };
                if (file.is_absolute())
                {
                    return file.parent_path();
                }
            }
            std::error_code error;
            return std::filesystem::current_path(error);
        }

        std::int64_t
        UnixMilliseconds()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
        }

        int
        Run(const std::vector<std::string> &arguments)
        {
            const std::filesystem::path program = Runner::CurrentExecutable(
                arguments.empty() ? nullptr : arguments.front().c_str());
            const std::string programName
                = program.empty() ? "tests" : program.stem().string();
            std::vector<std::string> rest;
            if (arguments.size() > 1)
            {
                rest.assign(arguments.begin() + 1, arguments.end());
            }
            auto parsed = Runner::ParseOptions(rest);
            if (!parsed)
            {
                PrintError(parsed.GetError().Message());
                return Catch::UnspecifiedErrorExitCode;
            }
            Runner::Options options = std::move(*parsed);
            options.programName = programName;

            if (options.mode == Runner::Mode::kHelp)
            {
                PrintHelp(programName);
                return 0;
            }
            if (options.mode == Runner::Mode::kVersion)
            {
                std::printf("Catch3 %s\n", kVersion);
                return 0;
            }
            if (ReportStartupErrors())
            {
                return Catch::UnspecifiedErrorExitCode;
            }
            if (options.filenamesAsTags)
            {
                Catch::getMutableRegistryHub()
                    .getMutableTestCaseRegistry()
                    .enableFilenameTags();
            }
            if (options.mode == Runner::Mode::kWorker)
            {
                return Runner::RunWorker(options, programName);
            }

            std::vector<std::string> notes;
            if (BazelTest())
            {
                ApplyBazel(options, notes);
            }
            if (!options.seedGiven)
            {
                std::random_device device;
                options.seed = device();
            }

            auto catalog = Runner::Catalog::Load();
            if (!catalog)
            {
                PrintError(catalog.GetError().Message());
                return Catch::UnspecifiedErrorExitCode;
            }
            const Catch::Config config{ Runner::ToConfigData(options,
                                                             programName) };
            const auto &invalid = config.testSpec().getInvalidSpecs();
            if (!invalid.empty())
            {
                for (const auto &spec : invalid)
                {
                    PrintError("invalid test spec: " + spec);
                }
                return Catch::InvalidTestSpecExitCode;
            }
            std::vector<std::string> unmatched;
            auto matched = catalog->Match(config, unmatched);
            if (options.mode == Runner::Mode::kListTests
                || options.mode == Runner::Mode::kListTags)
            {
                if (options.shard.count > 1)
                {
                    // A shard lists the tests it would run, dependencies
                    // included, in registration order.
                    Runner::PlanInputs inputs;
                    inputs.catalog = &*catalog;
                    inputs.matched = matched;
                    Runner::Options listing = options;
                    listing.order = Runner::Order::kDeclared;
                    auto plan = Runner::MakePlan(listing, inputs);
                    if (!plan)
                    {
                        PrintError(plan.GetError().Message());
                        return Catch::UnspecifiedErrorExitCode;
                    }
                    matched.clear();
                    for (const auto &test : plan->tests)
                    {
                        matched.push_back(test.entry->index);
                    }
                    std::sort(matched.begin(), matched.end());
                }
                return List(options, *catalog, matched, programName);
            }

            const std::filesystem::path historyFile
                = Runner::HistoryFile(HistoryDirectory(options, program),
                                      programName);
            Runner::History history;
            if (options.historyEnabled
                || options.mode == Runner::Mode::kAnalytics)
            {
                auto loaded = Runner::History::Load(historyFile);
                if (loaded)
                {
                    history = std::move(*loaded);
                }
                else
                {
                    notes.push_back("the history could not be read: "
                                    + loaded.GetError().Message());
                }
            }
            if (options.mode == Runner::Mode::kAnalytics)
            {
                return Analytics(options, history, historyFile);
            }

            std::string binaryFingerprint;
            if (options.historyEnabled || options.incremental)
            {
                auto digest = Runner::Sha256File(program);
                binaryFingerprint = digest ? *digest : std::string{};
            }

            std::optional<Runner::Changes> changes;
            const auto readChanges = [&] {
                if (changes)
                {
                    return;
                }
                auto read = Runner::ReadChanges(SourceDirectory(*catalog),
                                                options.changesSince);
                if (read)
                {
                    changes = std::move(*read);
                }
                else if (options.selection == Runner::Selection::kChanged
                         || options.selection == Runner::Selection::kSmart)
                {
                    notes.push_back(read.GetError().Message());
                }
            };
            if (options.selection == Runner::Selection::kChanged
                || options.selection == Runner::Selection::kSmart)
            {
                readChanges();
            }

            Runner::PlanInputs inputs;
            inputs.catalog = &*catalog;
            inputs.matched = matched;
            inputs.unmatchedSpecs = unmatched;
            inputs.history = options.historyEnabled ? &history : nullptr;
            inputs.changes = changes ? &*changes : nullptr;
            inputs.binaryFingerprint = binaryFingerprint;
            auto plan = Runner::MakePlan(options, inputs);
            if (!plan)
            {
                PrintError(plan.GetError().Message());
                return Catch::UnspecifiedErrorExitCode;
            }
            notes.insert(notes.end(), plan->notes.begin(), plan->notes.end());

            Runner::ConsoleReport console{ options, stdout };
            const std::size_t workers
                = options.inProcess
                      ? 1
                      : std::max<std::size_t>(
                            1,
                            std::min<std::size_t>(
                                options.jobs == 0 ? Runner::HardwareThreads()
                                                  : options.jobs,
                                plan->tests.size()));
            console.RunStarting(plan->tests.size(),
                                workers,
                                options.seed,
                                notes);

            std::string guard = options.exitGuardPath;
            if (BazelTest() && guard.empty())
            {
                guard = Runner::ReadEnvironment("TEST_PREMATURE_EXIT_FILE");
            }
            if (!guard.empty())
            {
                if (auto status = Runner::CreatePrivateFile(guard); !status)
                {
                    PrintError("the exit guard file: "
                               + status.GetError().Message());
                    return Catch::UnspecifiedErrorExitCode;
                }
            }

            const auto started = std::chrono::steady_clock::now();
            RunResult run;
            run.binary = programName;
            run.seed = options.seed;
            run.shardIndex = options.shard.index;
            run.shardCount = options.shard.count;
            run.candidates = plan->candidates;
            run.unmatchedSpecs = plan->unmatchedSpecs;
            run.startUnixMilliseconds = UnixMilliseconds();
            Runner::SchedulerEvents events;
            events.testFinished = [&](const TestResult &test) {
                console.TestFinished(test);
            };
            events.retrying
                = [&](const TestResult &test, const Attempt &attempt) {
                      console.Retrying(test, attempt);
                  };
            auto outcome = Runner::RunPlan(options,
                                           *plan,
                                           *catalog,
                                           program.string(),
                                           programName,
                                           events);
            run.wallMicroseconds
                = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now() - started)
                      .count();
            run.workers = outcome.workers;
            run.tests = std::move(outcome.tests);
            for (auto &kept : plan->kept)
            {
                run.tests.push_back(std::move(kept));
            }
            run.notes = std::move(outcome.notes);

            const bool anyFailure
                = std::any_of(run.tests.begin(),
                              run.tests.end(),
                              [](const TestResult &test) {
                                  return IsFailure(test.outcome) || test.flaky;
                              });
            if (anyFailure)
            {
                readChanges();
            }
            Runner::AnalysisInputs analysis;
            analysis.history = options.historyEnabled ? &history : nullptr;
            analysis.changes = changes ? &*changes : nullptr;
            analysis.plan = &*plan;
            analysis.program = program.string();
            analysis.changesSince = options.changesSince;
            Runner::Analyze(run, analysis);
            console.RunEnded(run);

            // A report that was asked for and is missing would let a build
            // server miss the run, so it fails the run.
            bool reportMissing = false;
            if (!options.junitPath.empty())
            {
                if (auto status = Runner::WriteFileAtomically(
                        options.junitPath,
                        JUnitXmlWriter::ToString(run));
                    !status)
                {
                    PrintError(status.GetError().Message());
                    reportMissing = true;
                }
            }
            if (!options.jsonPath.empty())
            {
                if (auto status = Runner::WriteFileAtomically(
                        options.jsonPath,
                        Runner::ToJson(run).ToPrettyString());
                    !status)
                {
                    PrintError(status.GetError().Message());
                    reportMissing = true;
                }
            }
            if (!options.tracePath.empty())
            {
                if (auto status
                    = Runner::WriteFileAtomically(options.tracePath,
                                                  Runner::TraceJson(run));
                    !status)
                {
                    PrintError(status.GetError().Message());
                    reportMissing = true;
                }
            }
            if (options.historyEnabled && !run.tests.empty())
            {
                Runner::RunRecord record;
                record.unixMilliseconds = run.startUnixMilliseconds;
                record.commit = changes ? changes->commit : std::string{};
                record.binaryFingerprint = binaryFingerprint;
                record.seed = run.seed;
                record.tests = run.tests.size();
                record.wallMicroseconds = run.wallMicroseconds;
                for (const auto &test : run.tests)
                {
                    record.failed += test.FailsRun() ? 1 : 0;
                }
                auto committed = Runner::History::Commit(historyFile,
                                                         run,
                                                         record,
                                                         plan->fingerprints,
                                                         {});
                if (!committed)
                {
                    std::fprintf(stderr,
                                 "warning: the history was not saved: %s\n",
                                 committed.GetError().Message().c_str());
                }
            }
            if (!guard.empty())
            {
                std::error_code error;
                std::filesystem::remove(guard, error);
            }

            if (outcome.infrastructureFailed)
            {
                return Catch::UnspecifiedErrorExitCode;
            }
            if (!run.unmatchedSpecs.empty() && options.warnUnmatchedSpecs)
            {
                return Catch::UnmatchedTestSpecExitCode;
            }
            const bool quarantineCounts
                = options.quarantine == Runner::QuarantineMode::kReport;
            bool failed = false;
            std::size_t ran = 0;
            std::size_t skipped = 0;
            for (const auto &test : run.tests)
            {
                if (test.outcome != Outcome::kUpToDate)
                {
                    ++ran;
                }
                skipped += test.outcome == Outcome::kSkipped ? 1 : 0;
                failed = failed || test.FailsRun()
                         || (quarantineCounts && test.quarantined
                             && IsFailure(test.outcome));
            }
            if (failed)
            {
                return Catch::TestFailureExitCode;
            }
            if (reportMissing)
            {
                return Catch::UnspecifiedErrorExitCode;
            }
            if (run.tests.empty() && !options.allowNoTests)
            {
                return Catch::NoTestsRunExitCode;
            }
            if (ran > 0 && skipped == ran && !options.allowNoTests)
            {
                return Catch::AllTestsSkippedExitCode;
            }
            return 0;
        }
    } // namespace

    int
    RunTests(const std::vector<std::string> &arguments)
    {
        return Run(arguments);
    }

    void
    PrintHelp(const std::string &programName)
    {
        std::fputs(Runner::HelpText(programName).c_str(), stdout);
    }
} // namespace Progmasoft::Catch3
