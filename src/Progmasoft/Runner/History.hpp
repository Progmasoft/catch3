// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Results.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Expected.hpp"
#include "Json.hpp"

namespace Progmasoft::Catch3::Runner
{
    /// One result of one test in one run.
    struct TestRecord final
    {
        std::int64_t run = 0;
        Outcome outcome = Outcome::kNotRun;
        FailureCategory category = FailureCategory::kNone;
        std::int64_t durationMicroseconds = 0;
        std::size_t attempts = 0;
        bool flaky = false;
        std::string signature;
        /// What the test's result depended on; equal fingerprints mean the
        /// same inputs.
        std::string fingerprint;
    };

    /// How to run a failure again the way it happened.
    struct ReplayRecord final
    {
        std::int64_t run = 0;
        std::uint32_t seed = 0;
        /// The tests the worker ran before, in order.
        std::vector<std::string> precededBy;
        bool freshProcess = false;
    };

    struct QuarantineRecord final
    {
        std::int64_t since = 0;
        std::string reason;
    };

    /// Everything remembered about one test.
    struct TestHistory final
    {
        /// The most recent last.
        std::vector<TestRecord> records;
        std::optional<QuarantineRecord> quarantine;
        std::optional<ReplayRecord> lastFailure;

        [[nodiscard]] const TestRecord *
        Last() const noexcept;

        /// A percentile of the durations of recent runs that finished on
        /// their own; nothing without such runs.
        [[nodiscard]] std::optional<std::int64_t>
        DurationPercentile(double fraction) const;

        /// The probability that the next run fails: a failure rate in which
        /// each older run counts less, starting from a prior for tests with
        /// little history.
        [[nodiscard]] double
        FailureProbability() const;

        /// Flaky results, and failures that alternate with passes on the
        /// same fingerprint, among the last `window` runs.
        [[nodiscard]] std::size_t
        FlakyEvents(std::size_t window) const;

        /// Passes in a row, counting back from the latest run.
        [[nodiscard]] std::size_t
        PassStreak() const;
    };

    /// One run, as the history remembers it.
    struct RunRecord final
    {
        std::int64_t id = 0;
        std::int64_t unixMilliseconds = 0;
        std::string commit;
        std::string binaryFingerprint;
        std::uint32_t seed = 0;
        std::size_t tests = 0;
        std::size_t failed = 0;
        std::int64_t wallMicroseconds = 0;
    };

    /// When to put a test into quarantine and let it out.
    struct QuarantinePolicy final
    {
        std::size_t window = 20;
        std::size_t flakyEventsToEnter = 2;
        std::size_t passesToLeave = 10;
    };

    /// The results of earlier runs of one test binary.
    ///
    /// It lives in one JSON file. Several runs of the same binary may finish
    /// at the same time, so `Commit` changes the file under a lock: it reads
    /// it again, adds its run and replaces it in one rename.
    class History final
    {
    public:
        History() = default;

        /// Reads the file; a missing file is an empty history.
        [[nodiscard]] static Expected<History>
        Load(const std::filesystem::path &file);

        /// Adds `run` to the history in `file`, updates quarantine and
        /// returns the history as written.
        [[nodiscard]] static Expected<History>
        Commit(const std::filesystem::path &file,
               const RunResult &run,
               const RunRecord &record,
               const std::map<std::string, std::string> &fingerprints,
               const QuarantinePolicy &policy,
               std::chrono::milliseconds lockWait = std::chrono::seconds(10));

        [[nodiscard]] const TestHistory *
        Find(const std::string &key) const;

        [[nodiscard]] const std::map<std::string, TestHistory> &
        Tests() const noexcept
        {
            return tests_;
        }

        [[nodiscard]] const std::vector<RunRecord> &
        Runs() const noexcept
        {
            return runs_;
        }

        [[nodiscard]] JsonValue
        ToJson() const;

        [[nodiscard]] static Expected<History>
        FromJson(const JsonValue &value);

        /// The key a test result is filed under.
        [[nodiscard]] static std::string
        KeyOf(const TestResult &test);

    private:
        void
        Apply(const RunResult &run,
              RunRecord record,
              const std::map<std::string, std::string> &fingerprints,
              const QuarantinePolicy &policy);

        std::int64_t nextRun_ = 1;
        std::vector<RunRecord> runs_;
        std::map<std::string, TestHistory> tests_;
    };

    /// The file the history of a binary is kept in under `directory`.
    [[nodiscard]] std::filesystem::path
    HistoryFile(const std::filesystem::path &directory,
                const std::string &binaryName);
} // namespace Progmasoft::Catch3::Runner
