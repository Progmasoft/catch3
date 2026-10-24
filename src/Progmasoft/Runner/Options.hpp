// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include "Expected.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Progmasoft::Catch3::Runner
{
    /// What the command line asks for.
    enum class Mode : std::uint8_t
    {
        kRun,
        kListTests,
        kListTags,
        kAnalytics,
        kHelp,
        kVersion,
        /// Internal: run tests on behalf of a runner, over the protocol.
        kWorker,
    };

    enum class ListFormat : std::uint8_t
    {
        kText,
        kNames,
        kJson,
    };

    /// Which of the matching tests to run.
    enum class Selection : std::uint8_t
    {
        /// Every matching test.
        kAll,
        /// Those that failed, crashed, timed out or were flaky last time.
        kFailed,
        /// Those defined in, or covering, files changed since a revision.
        kChanged,
        /// Those without a recorded result.
        kNew,
        /// Changed, failed and new tests, and those most likely to fail.
        kSmart,
    };

    enum class Order : std::uint8_t
    {
        /// Most likely to fail per second of running time first.
        kPriority,
        kDeclared,
        kName,
        kRandom,
    };

    enum class RetryMode : std::uint8_t
    {
        kOff,
        /// Retry only what may pass on a second try: crashes, timeouts,
        /// tests with a flaky history and failures that do not repeat.
        kAuto,
        /// Retry every failure up to the limit.
        kAlways,
    };

    enum class QuarantineMode : std::uint8_t
    {
        /// Tests that keep flaking go into quarantine on their own; a failure
        /// in quarantine is reported but does not fail the run.
        kAuto,
        /// Quarantine is reported, but failures still fail the run.
        kReport,
        kOff,
    };

    enum class ColorMode : std::uint8_t
    {
        kAuto,
        kAlways,
        kNever,
    };

    enum class Verbosity : std::uint8_t
    {
        kQuiet,
        kNormal,
        kVerbose,
    };

    /// One shard of a run: `index` counts from zero.
    struct Shard final
    {
        unsigned index = 0;
        unsigned count = 1;
    };

    /// Everything the command line can say, already checked and typed.
    struct Options final
    {
        Mode mode = Mode::kRun;
        ListFormat listFormat = ListFormat::kText;
        std::string programName;

        // Selecting tests.
        std::vector<std::string> testSpecs;
        std::vector<std::string> sections;
        /// Path filters: "c:SECTION" or "g:INDEX" (a generator value, or *).
        std::vector<std::string> paths;
        Selection selection = Selection::kAll;
        std::string changesSince;
        bool incremental = false;
        Shard shard;
        std::string shardDurations;
        bool filenamesAsTags = false;

        // Running.
        unsigned jobs = 0;
        bool inProcess = false;
        bool isolate = false;
        /// The per-test limit; zero disables it.
        std::chrono::milliseconds timeout{0};
        bool timeoutGiven = false;
        bool adaptiveTimeout = true;
        RetryMode retryMode = RetryMode::kAuto;
        unsigned retries = 2;
        unsigned repeat = 1;
        unsigned failFast = 0;
        Order order = Order::kPriority;
        std::uint32_t seed = 0;
        bool seedGiven = false;
        bool bisectOrder = true;

        // How tests behave.
        bool includeSuccessful = false;
        bool breakIntoDebugger = false;
        bool noThrow = false;
        bool warnNoAssertions = false;
        bool warnUnmatchedSpecs = false;
        bool warnInfiniteGenerators = false;
        bool allowNoTests = false;
        bool skipBenchmarks = false;
        bool benchmarkNoAnalysis = false;
        unsigned benchmarkSamples = 100;
        unsigned benchmarkResamples = 100000;
        double benchmarkConfidenceInterval = 0.95;
        std::chrono::milliseconds benchmarkWarmupTime{100};

        // History and analysis.
        std::string historyDirectory;
        bool historyEnabled = true;
        QuarantineMode quarantine = QuarantineMode::kAuto;
        bool replayFailures = false;
        bool analyticsJson = false;

        // Output.
        Verbosity verbosity = Verbosity::kNormal;
        ColorMode color = ColorMode::kAuto;
        std::string junitPath;
        std::string jsonPath;
        std::string tracePath;
        unsigned slowest = 0;
        /// Created when the run starts and removed when it ends normally, so
        /// a file left behind shows that the run ended early.
        std::string exitGuardPath;
    };

    /// Reads a command line. `arguments` does not include the program.
    [[nodiscard]] Expected<Options>
    ParseOptions(const std::vector<std::string> &arguments);

    /// The help text, for `--help`.
    [[nodiscard]] std::string
    HelpText(const std::string &programName);

    /// The arguments a worker is started with to behave like `options`.
    [[nodiscard]] std::vector<std::string>
    WorkerArguments(const Options &options);
} // namespace Progmasoft::Catch3::Runner
