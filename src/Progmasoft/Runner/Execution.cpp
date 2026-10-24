// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <atomic>
#include <catch2/catch_assertion_result.hpp>
#include <catch2/catch_config.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/interfaces/catch_interfaces_registry_hub.hpp>
#include <catch2/interfaces/catch_interfaces_reporter.hpp>
#include <catch2/interfaces/catch_interfaces_reporter_factory.hpp>
#include <catch2/internal/catch_context.hpp>
#include <catch2/internal/catch_reporter_registry.hpp>
#include <catch2/internal/catch_run_context.hpp>
#include <catch2/internal/catch_string_manip.hpp>
#include <catch2/reporters/catch_reporter_multi.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <optional>

#include "Execution.hpp"
#include "Process.hpp"
#include "Protocol.hpp"
#include "ResultsJson.hpp"
#include "TemporaryDirectory.hpp"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        std::int64_t
        MicrosecondsBetween(Clock::time_point start, Clock::time_point end)
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                       end - start)
                .count();
        }

        FailureCategory
        CategoryOf(Catch::ResultWas::OfType type)
        {
            switch (type)
            {
                case Catch::ResultWas::ExpressionFailed:
                    return FailureCategory::kAssertion;
                case Catch::ResultWas::ExplicitFailure:
                    return FailureCategory::kExplicitFailure;
                case Catch::ResultWas::ThrewException:
                case Catch::ResultWas::Exception:
                    return FailureCategory::kUnexpectedException;
                case Catch::ResultWas::DidntThrowException:
                    return FailureCategory::kMissingException;
                case Catch::ResultWas::FatalErrorCondition:
                    return FailureCategory::kCrash;
                default:
                    return FailureCategory::kNone;
            }
        }

        std::string
        Location(const Catch::SourceLineInfo &info)
        {
            return std::string{ info.file } + ":" + std::to_string(info.line);
        }

        /// Turns the events of Catch2's engine into protocol messages.
        class ProtocolListener final: public Catch::IEventListener
        {
        public:
            ProtocolListener(const Catch::IConfig *config,
                             EventSink sink,
                             bool includeSuccessful)
                : IEventListener{ config }
                , sink_{ std::move(sink) }
                , includeSuccessful_{ includeSuccessful }
            {
                // Captured output belongs to the test that printed it.
                m_preferences.shouldRedirectStdOut = true;
                m_preferences.shouldReportAllAssertions = includeSuccessful;
                // The start of every assertion, for the location of a crash
                // or a hang.
                m_preferences.shouldReportAllAssertionStarts = true;
            }

            void
            Begin(std::size_t index, std::size_t attempt)
            {
                index_ = index;
                attempt_ = attempt;
                depth_ = 0;
                sections_.clear();
                sectionStarts_.clear();
                firstCategory_ = FailureCategory::kNone;
                lastLocation_.clear();
                lastSentLocation_ = Clock::time_point{};
            }

            void
            noMatchingTestCases(Catch::StringRef) override
            {}

            void
            reportInvalidTestSpec(Catch::StringRef) override
            {}

            void
            testRunStarting(const Catch::TestRunInfo &) override
            {}

            void
            testCaseStarting(const Catch::TestCaseInfo &) override
            {
                start_ = Clock::now();
                JsonValue message = Message("start");
                message.Set("index", index_);
                message.Set("attempt", attempt_);
                sink_(message);
            }

            void
            testCasePartialStarting(const Catch::TestCaseInfo &,
                                    std::uint64_t) override
            {
                // Every run through the test case enters the sections
                // again; depth starts over.
                depth_ = 0;
                sections_.clear();
                sectionStarts_.clear();
            }

            void
            sectionStarting(const Catch::SectionInfo &info) override
            {
                // Catch2 reports the test case itself as the outermost
                // section; the runner counts real sections only.
                if (depth_++ == 0)
                {
                    return;
                }
                sections_.push_back(info.name);
                sectionStarts_.push_back(Clock::now());
                JsonValue message = Message("section");
                message.Set("begin", true);
                message.Set("name", info.name);
                message.Set("file", std::string{ info.lineInfo.file });
                message.Set("line", info.lineInfo.line);
                message.Set("depth", sections_.size() - 1);
                message.Set("at", MicrosecondsBetween(start_, Clock::now()));
                sink_(message);
            }

            void
            sectionEnded(const Catch::SectionStats &stats) override
            {
                if (stats.missingAssertions)
                {
                    // --warn=no-assertions: say why the test failed.
                    AssertionRecord record;
                    record.file
                        = std::string{ stats.sectionInfo.lineInfo.file };
                    record.line = stats.sectionInfo.lineInfo.line;
                    record.message = "no assertion ran in "
                                     + stats.sectionInfo.name
                                     + " (--warn=no-assertions)";
                    record.sections = sections_;
                    record.category = FailureCategory::kAssertion;
                    if (firstCategory_ == FailureCategory::kNone)
                    {
                        firstCategory_ = record.category;
                    }
                    JsonValue message = ToJson(record);
                    message.Set("t", "assertion");
                    sink_(message);
                }
                if (depth_ == 0)
                {
                    return;
                }
                if (--depth_ == 0 || sections_.empty())
                {
                    return;
                }
                const auto now = Clock::now();
                JsonValue message = Message("section");
                message.Set("begin", false);
                message.Set("depth", sections_.size() - 1);
                message.Set("at", MicrosecondsBetween(start_, now));
                message.Set("duration",
                            MicrosecondsBetween(sectionStarts_.back(), now));
                sink_(message);
                sections_.pop_back();
                sectionStarts_.pop_back();
            }

            void
            assertionStarting(const Catch::AssertionInfo &info) override
            {
                lastLocation_ = Location(info.lineInfo);
                // A hang is only noticed from outside, so the location goes
                // out now and then rather than with every assertion.
                const auto now = Clock::now();
                if (now - lastSentLocation_ > std::chrono::milliseconds(200))
                {
                    lastSentLocation_ = now;
                    JsonValue message = Message("at");
                    message.Set("location", lastLocation_);
                    sink_(message);
                }
            }

            void
            assertionEnded(const Catch::AssertionStats &stats) override
            {
                const auto &result = stats.assertionResult;
                const bool skipped
                    = result.getResultType() == Catch::ResultWas::ExplicitSkip;
                const bool passed = result.succeeded() || skipped;
                if (passed && !includeSuccessful_ && !skipped)
                {
                    return;
                }
                AssertionRecord record;
                record.file = std::string{ result.getSourceInfo().file };
                record.line = result.getSourceInfo().line;
                record.macro = std::string{ result.getTestMacroName() };
                if (result.hasExpression())
                {
                    record.expression = result.getExpression();
                    record.expansion = result.getExpandedExpression();
                }
                record.message = std::string{ result.getMessage() };
                for (const auto &info : stats.infoMessages)
                {
                    if (info.type == Catch::ResultWas::Info
                        || info.type == Catch::ResultWas::Warning)
                    {
                        record.context.push_back(info.message);
                    }
                }
                record.sections = sections_;
                record.passed = passed;
                record.category = skipped ? FailureCategory::kNone
                                          : CategoryOf(result.getResultType());
                record.allowed = !passed && result.isOk();
                if (!passed && !record.allowed
                    && firstCategory_ == FailureCategory::kNone)
                {
                    firstCategory_ = record.category;
                }
                JsonValue message = ToJson(record);
                message.Set("t", "assertion");
                sink_(message);
            }

            void
            benchmarkPreparing(Catch::StringRef) override
            {}

            void
            benchmarkStarting(const Catch::BenchmarkInfo &) override
            {}

            void
            benchmarkEnded(const Catch::BenchmarkStats<> &stats) override
            {
                BenchmarkRecord record;
                record.name = stats.info.name;
                record.samples = stats.samples.size();
                record.iterations
                    = static_cast<std::size_t>(stats.info.iterations);
                record.meanNanoseconds = stats.mean.point.count();
                record.lowMeanNanoseconds = stats.mean.lower_bound.count();
                record.highMeanNanoseconds = stats.mean.upper_bound.count();
                record.standardDeviationNanoseconds
                    = stats.standardDeviation.point.count();
                record.outlierVariance = stats.outlierVariance;
                JsonValue message = ToJson(record);
                message.Set("t", "benchmark");
                sink_(message);
            }

            void
            benchmarkFailed(Catch::StringRef) override
            {}

            void
            testCasePartialEnded(const Catch::TestCaseStats &,
                                 std::uint64_t) override
            {}

            void
            testCaseEnded(const Catch::TestCaseStats &stats) override
            {
                const auto &counts = stats.totals.testCases;
                Outcome outcome = Outcome::kPassed;
                if (counts.failed > 0)
                {
                    outcome = Outcome::kFailed;
                }
                else if (counts.failedButOk > 0)
                {
                    outcome = Outcome::kFailedAsExpected;
                }
                else if (counts.skipped > 0)
                {
                    outcome = Outcome::kSkipped;
                }
                FailureCategory category = firstCategory_;
                if (outcome == Outcome::kFailed
                    && category == FailureCategory::kNone)
                {
                    // A [!shouldfail] test that passed, or a test without
                    // assertions under --warn=no-assertions.
                    category = FailureCategory::kAssertion;
                }
                JsonValue message = Message("end");
                message.Set("index", index_);
                message.Set("attempt", attempt_);
                message.Set("outcome", OutcomeName(outcome));
                if (outcome == Outcome::kFailed)
                {
                    message.Set("category", CategoryName(category));
                }
                message.Set("passed", stats.totals.assertions.passed);
                // Failures [!mayfail] and [!shouldfail] allow are failures
                // too; the verdict says whether they count.
                message.Set("failed",
                            stats.totals.assertions.failed
                                + stats.totals.assertions.failedButOk);
                message.Set("duration",
                            MicrosecondsBetween(start_, Clock::now()));
                if (!stats.stdOut.empty())
                {
                    message.Set("stdout", stats.stdOut);
                }
                if (!stats.stdErr.empty())
                {
                    message.Set("stderr", stats.stdErr);
                }
                sink_(message);
            }

            void
            testRunEnded(const Catch::TestRunStats &) override
            {}

            void
            fatalErrorEncountered(Catch::StringRef error) override
            {
                JsonValue message = Message("fatal");
                message.Set("message", std::string{ error });
                message.Set("location", lastLocation_);
                sink_(message);
            }

        private:
            EventSink sink_;
            bool includeSuccessful_;
            std::size_t index_ = 0;
            std::size_t attempt_ = 0;
            std::size_t depth_ = 0;
            std::vector<std::string> sections_;
            std::vector<Clock::time_point> sectionStarts_;
            Clock::time_point start_{};
            FailureCategory firstCategory_ = FailureCategory::kNone;
            std::string lastLocation_;
            Clock::time_point lastSentLocation_{};
        };
    } // namespace

    Catch::ConfigData
    ToConfigData(const Options &options, const std::string &programName)
    {
        Catch::ConfigData data;
        data.showSuccessfulTests = options.includeSuccessful;
        data.shouldDebugBreak = options.breakIntoDebugger;
        data.noThrow = options.noThrow;
        data.filenamesAsTags = options.filenamesAsTags;
        data.allowZeroTests = options.allowNoTests;
        data.rngSeed = options.seed;
        data.rngSeedWasFixed = true;
        data.skipBenchmarks = options.skipBenchmarks;
        data.benchmarkNoAnalysis = options.benchmarkNoAnalysis;
        data.benchmarkSamples = options.benchmarkSamples;
        data.benchmarkResamples = options.benchmarkResamples;
        data.benchmarkConfidenceInterval = options.benchmarkConfidenceInterval;
        data.benchmarkWarmupTime = options.benchmarkWarmupTime.count();
        int warnings = Catch::WarnAbout::Nothing;
        if (options.warnNoAssertions)
        {
            warnings |= Catch::WarnAbout::NoAssertions;
        }
        if (options.warnUnmatchedSpecs)
        {
            warnings |= Catch::WarnAbout::UnmatchedTestSpec;
        }
        if (options.warnInfiniteGenerators)
        {
            warnings |= Catch::WarnAbout::InfiniteGenerator;
        }
        data.warnings = static_cast<Catch::WarnAbout::What>(warnings);
        data.runOrder = Catch::TestRunOrder::Declared;
        data.processName = programName;
        data.testsOrTags = options.testSpecs;
        for (const auto &section : options.sections)
        {
            data.pathFilters.emplace_back(Catch::PathFilter::For::Section,
                                          Catch::trim(section));
        }
        for (const auto &path : options.paths)
        {
            data.useNewPathFilteringBehaviour = true;
            data.pathFilters.emplace_back(
                path[0] == 'c' ? Catch::PathFilter::For::Section
                               : Catch::PathFilter::For::Generator,
                Catch::trim(path.substr(2)));
        }
        return data;
    }

    namespace
    {
        /// The executor whose test is running, for the exit handler.
        std::atomic<TestExecutor::State *> runningState{ nullptr };

        /// A test that calls `exit` never ends, and what Catch captured of
        /// its output would be lost with the process: it is written where
        /// the runner reads the process's own output.
        void
        WriteCapturedOutputAtExit();
    } // namespace

    struct TestExecutor::State final
    {
        Catch::Config config;
        const Catalog &catalog;
        ProtocolListener *listener = nullptr;
        std::optional<Catch::RunContext> context;

        State(const Catch::ConfigData &data, const Catalog &knownTests)
            : config{ data }
            , catalog{ knownTests }
        {}
    };

    TestExecutor::TestExecutor(const Options &options,
                               const Catalog &catalog,
                               const std::string &programName,
                               EventSink sink)
        : state_{ std::make_unique<State>(ToConfigData(options, programName),
                                          catalog) }
    {
        Catch::getCurrentMutableContext().setConfig(&state_->config);
        auto multi
            = Catch::Detail::make_unique<Catch::MultiReporter>(&state_->config);
        for (const auto &factory :
             Catch::getRegistryHub().getReporterRegistry().getListeners())
        {
            multi->addListener(factory->create(&state_->config));
        }
        auto listener = Catch::Detail::make_unique<ProtocolListener>(
            &state_->config,
            std::move(sink),
            options.includeSuccessful);
        state_->listener = listener.get();
        multi->addReporter(CATCH_MOVE(listener));
        Catch::seedRng(state_->config);
        state_->context.emplace(&state_->config, CATCH_MOVE(multi));
        static const bool registered
            = std::atexit(&WriteCapturedOutputAtExit) == 0;
        static_cast<void>(registered);
    }

    TestExecutor::~TestExecutor()
    {
        // The run context reports the end of the run to the listeners.
        state_->context.reset();
        Catch::getCurrentMutableContext().setConfig(nullptr);
    }

    void
    TestExecutor::Run(std::size_t index,
                      std::size_t attempt,
                      const std::string &temporaryDirectory)
    {
        if (index >= state_->catalog.Handles().size())
        {
            return;
        }
        const TemporaryDirectoryScope scope{ temporaryDirectory };
        state_->listener->Begin(index, attempt);
        runningState.store(state_.get());
        static_cast<void>(
            state_->context->runTest(state_->catalog.Handles()[index]));
        runningState.store(nullptr);
    }

    namespace
    {
        void
        WriteCapturedOutputAtExit()
        {
            TestExecutor::State *state = runningState.exchange(nullptr);
            if (state == nullptr || !state->context)
            {
                return;
            }
            const std::string output = state->context->endCapture();
            std::fflush(stdout);
            std::fflush(stderr);
            WriteStandardError(output);
        }
    } // namespace

    int
    RunWorker(const Options &options, const std::string &programName)
    {
        QuietCrashReporting();
        auto streams = TakeStandardStreams();
        if (!streams)
        {
            std::fprintf(stderr,
                         "catch3 worker: %s\n",
                         streams.GetError().Message().c_str());
            return 1;
        }
        auto catalog = Catalog::Load();
        if (!catalog)
        {
            std::fprintf(stderr,
                         "catch3 worker: %s\n",
                         catalog.GetError().Message().c_str());
            return 1;
        }
        MessageWriter writer{ streams->events };
        bool connected = true;
        const auto send = [&](const JsonValue &message) {
            if (connected && !writer.Send(message))
            {
                // The runner is gone; there is no one to report to.
                connected = false;
            }
        };
        // Listeners are created and told of the run before the greeting, so
        // what they print then is written before the runner assigns a test
        // and is not mistaken for that test's output.
        TestExecutor executor{ options, *catalog, programName, send };
        std::fflush(stdout);
        std::fflush(stderr);

        JsonValue hello = Message("hello");
        hello.Set("pid", CurrentProcessId());
        hello.Set("identity", catalog->Identity());
        hello.Set("tests", catalog->Tests().size());
        hello.Set("protocol", kWorkerProtocolVersion);
        send(hello);
        LineReader commands{ streams->commands };
        while (connected)
        {
            const auto line = commands.Next();
            if (!line)
            {
                break;
            }
            const auto command = ParseJson(*line);
            if (!command)
            {
                continue;
            }
            const std::string &kind = (*command)["t"].AsString();
            if (kind == "quit")
            {
                break;
            }
            if (kind == "run")
            {
                executor.Run(
                    static_cast<std::size_t>((*command)["index"].AsUnsigned()),
                    static_cast<std::size_t>(
                        (*command)["attempt"].AsUnsigned()),
                    (*command)["temp"].AsString());
            }
        }
        return 0;
    }
} // namespace Progmasoft::Catch3::Runner
