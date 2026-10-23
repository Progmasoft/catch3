// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Progmasoft::Catch3
{
    /// How one attempt at a test ended.
    enum class Outcome : std::uint8_t
    {
        kPassed,
        /// An assertion failed, or the test threw.
        kFailed,
        /// It failed, but `[!shouldfail]` or `[!mayfail]` allows that.
        kFailedAsExpected,
        /// `SKIP` was called.
        kSkipped,
        /// The process running it died: a signal, a structured exception or
        /// an exit in the middle of the test.
        kCrashed,
        /// It ran past its time limit and was stopped.
        kTimedOut,
        /// A test it depends on did not pass, so it was not run.
        kBlocked,
        /// The run stopped before reaching it.
        kNotRun,
        /// It passed last time and nothing it depends on changed, so the
        /// incremental run kept that result.
        kUpToDate,
    };

    /// Why a failure happened, as far as the runner can tell.
    enum class FailureCategory : std::uint8_t
    {
        kNone,
        /// A REQUIRE or CHECK expression was false.
        kAssertion,
        /// An exception escaped the test or an assertion.
        kUnexpectedException,
        /// REQUIRE_THROWS and its kind did not see the expected exception.
        kMissingException,
        /// FAIL or FAIL_CHECK.
        kExplicitFailure,
        /// A signal or structured exception inside the test.
        kCrash,
        kTimeout,
        /// The worker could not run the test at all, for example because it
        /// failed to start.
        kInfrastructure,
        /// A test it depends on failed.
        kDependency,
    };

    /// How a failure relates to earlier runs.
    enum class FailureHistory : std::uint8_t
    {
        /// There is no history to compare with.
        kUnknown,
        /// The test passed in the previous run.
        kNew,
        /// The test failed in the previous run too.
        kPersistent,
        /// The test passes and fails with the same inputs.
        kIntermittent,
    };

    /// One failed or reported assertion.
    struct AssertionRecord final
    {
        std::string file;
        std::size_t line = 0;
        /// The macro, such as REQUIRE.
        std::string macro;
        /// The expression as written.
        std::string expression;
        /// The expression with its operands' values.
        std::string expansion;
        std::string message;
        /// INFO, CAPTURE and UNSCOPED_INFO messages in force.
        std::vector<std::string> context;
        /// The sections the assertion was in, outermost first.
        std::vector<std::string> sections;
        FailureCategory category = FailureCategory::kAssertion;
        bool passed = false;
        /// The failure is allowed by `[!mayfail]` or `[!shouldfail]`.
        bool allowed = false;
    };

    /// The statistics of one BENCHMARK.
    struct BenchmarkRecord final
    {
        std::string name;
        std::size_t samples = 0;
        std::size_t iterations = 0;
        double meanNanoseconds = 0.0;
        double lowMeanNanoseconds = 0.0;
        double highMeanNanoseconds = 0.0;
        double standardDeviationNanoseconds = 0.0;
        double outlierVariance = 0.0;
    };

    /// One section entered while a test ran, with its timing.
    struct SectionRecord final
    {
        std::string name;
        std::string file;
        std::size_t line = 0;
        /// Microseconds since the start of the attempt.
        std::int64_t startMicroseconds = 0;
        /// -1 for a section that never ended: the attempt crashed or timed
        /// out inside it.
        std::int64_t durationMicroseconds = 0;
        /// 0 for a section directly in the test case.
        std::size_t depth = 0;
    };

    /// One run of a test in a worker. A test may be tried several times:
    /// retries, repeats and the probes that look for an order dependency.
    struct Attempt final
    {
        Outcome outcome = Outcome::kNotRun;
        FailureCategory category = FailureCategory::kNone;
        std::size_t number = 1;
        /// The worker that ran it, and the tests it ran before, in order.
        std::size_t worker = 0;
        std::vector<std::string> precededBy;
        bool freshProcess = false;
        std::uint32_t seed = 0;
        std::int64_t startMicroseconds = 0;
        std::int64_t durationMicroseconds = 0;
        std::size_t assertionsPassed = 0;
        std::size_t assertionsFailed = 0;
        std::vector<AssertionRecord> assertions;
        std::vector<BenchmarkRecord> benchmarks;
        std::vector<SectionRecord> sections;
        /// What the test wrote to std::cout and std::cerr, captured.
        std::string standardOutput;
        std::string standardError;
        /// What reached the worker's own streams while the test ran, such as
        /// printf output or a sanitizer report.
        std::string processOutput;
        /// The fatal condition the worker reported before it died.
        std::string fatalMessage;
        /// How the worker ended, for a crash.
        std::string exitDescription;
        /// The last assertion started before a crash or a timeout.
        std::string lastLocation;
        /// The directory the test had for its temporary files; kept when the
        /// attempt failed, so its contents can be inspected.
        std::string temporaryDirectory;
    };

    /// One test of the run and everything the runner learned about it.
    struct TestResult final
    {
        /// Identifies the test across runs and builds: its name, with its
        /// class and, when names repeat, its tags. History is kept by it.
        std::string id;
        std::string name;
        std::string className;
        std::string file;
        std::size_t line = 0;
        std::vector<std::string> tags;
        /// The verdict over all attempts.
        Outcome outcome = Outcome::kNotRun;
        FailureCategory category = FailureCategory::kNone;
        FailureHistory history = FailureHistory::kUnknown;
        /// It failed and then passed with the same inputs.
        bool flaky = false;
        /// It is in quarantine, so its failure does not fail the run.
        bool quarantined = false;
        /// Why it was selected, when not every test runs: "failed last
        /// run", "covers a changed file", "dependency of ...".
        std::string selectionReason;
        /// A stable identifier of the failure: its category, location and
        /// message with numbers removed. Tests that share it probably share a
        /// cause.
        std::string signature;
        /// What the runner found out about the failure, one finding per
        /// entry, most useful first.
        std::vector<std::string> analysis;
        /// A command line that runs the failure again the way it happened.
        std::string replayCommand;
        std::vector<Attempt> attempts;
        std::int64_t predictedDurationMicroseconds = 0;
        double predictedFailureProbability = 0.0;

        /// The sum of the attempts' durations.
        [[nodiscard]] std::int64_t
        DurationMicroseconds() const noexcept;

        /// Whether the result fails the run.
        [[nodiscard]] bool
        FailsRun() const noexcept;
    };

    /// The totals of a run, by verdict.
    struct RunSummary final
    {
        std::size_t tests = 0;
        std::size_t passed = 0;
        std::size_t failed = 0;
        std::size_t failedAsExpected = 0;
        std::size_t skipped = 0;
        std::size_t crashed = 0;
        std::size_t timedOut = 0;
        std::size_t blocked = 0;
        std::size_t notRun = 0;
        std::size_t upToDate = 0;
        std::size_t flaky = 0;
        std::size_t quarantined = 0;
        std::size_t attempts = 0;
        std::size_t assertionsPassed = 0;
        std::size_t assertionsFailed = 0;
        /// Wall-clock time of the run, and the time its tests took added up;
        /// their ratio is the speed-up of running in parallel.
        std::int64_t wallMicroseconds = 0;
        std::int64_t testMicroseconds = 0;
    };

    /// A whole run of one test binary.
    struct RunResult final
    {
        std::string binary;
        std::uint32_t seed = 0;
        std::size_t workers = 0;
        /// The shard, when the run was one of several.
        unsigned shardIndex = 0;
        unsigned shardCount = 1;
        /// The tests that matched before selection narrowed them.
        std::size_t candidates = 0;
        std::vector<TestResult> tests;
        /// Test specs that matched nothing.
        std::vector<std::string> unmatchedSpecs;
        /// Messages about the run itself, such as a history that could not
        /// be read.
        std::vector<std::string> notes;
        std::int64_t startUnixMilliseconds = 0;
        std::int64_t wallMicroseconds = 0;
    };

    [[nodiscard]] RunSummary
    Summarize(const RunResult &run);

    /// The name an outcome has in reports: "passed", "timed-out", ...
    [[nodiscard]] const char *
    OutcomeName(Outcome outcome) noexcept;

    [[nodiscard]] std::optional<Outcome>
    OutcomeFromName(const std::string &name) noexcept;

    [[nodiscard]] const char *
    CategoryName(FailureCategory category) noexcept;

    [[nodiscard]] std::optional<FailureCategory>
    CategoryFromName(const std::string &name) noexcept;

    [[nodiscard]] const char *
    HistoryName(FailureHistory history) noexcept;

    /// The failure of an attempt as text: each failed assertion with its
    /// expansion, messages and sections, or what is known about a crash or a
    /// timeout. Empty for an attempt that did not fail.
    [[nodiscard]] std::string
    DescribeFailure(const Attempt &attempt);

    /// The first line of `DescribeFailure`, for a summary.
    [[nodiscard]] std::string
    SummarizeFailure(const Attempt &attempt);

    /// Whether an outcome counts as a failure of the attempt.
    [[nodiscard]] bool
    IsFailure(Outcome outcome) noexcept;
} // namespace Progmasoft::Catch3
