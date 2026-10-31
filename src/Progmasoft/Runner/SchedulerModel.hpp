// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

// The state the scheduler keeps of jobs, workers and tests.

#include <Progmasoft/Catch3/Results.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "Process.hpp"

namespace Progmasoft::Catch3::Runner::Scheduling
{
    using Clock = std::chrono::steady_clock;

    inline constexpr std::size_t kMaximumRecordedAssertions = 200;
    inline constexpr std::size_t kMaximumProcessOutput = 64 * 1024;
    inline constexpr std::size_t kMaximumProbes = 12;
    inline constexpr std::size_t kStartupFailuresToGiveUp = 3;
    inline constexpr std::chrono::milliseconds kStartupTimeout{ 60000 };

    inline std::int64_t
    Microseconds(Clock::duration duration)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(duration)
            .count();
    }

    inline void
    AppendBounded(std::string &target, std::string_view text)
    {
        target.append(text);
        if (target.size() > kMaximumProcessOutput)
        {
            // The end of the output says most about a crash.
            target.erase(0, target.size() - kMaximumProcessOutput);
        }
    }

    inline bool
    Satisfies(Outcome outcome)
    {
        return outcome == Outcome::kPassed
               || outcome == Outcome::kFailedAsExpected
               || outcome == Outcome::kUpToDate;
    }

    inline bool
    Passing(Outcome outcome)
    {
        return outcome == Outcome::kPassed
               || outcome == Outcome::kFailedAsExpected
               || outcome == Outcome::kSkipped;
    }

    /// What a worker is asked to do: run tests in order, the last one
    /// being the one the job is about.
    struct Job final
    {
        enum class Kind : std::uint8_t
        {
            /// An attempt of a planned test.
            kAttempt,
            /// A run that tells whether a failure depends on the tests
            /// before it; its result is not an attempt.
            kProbe,
        };

        Kind kind = Kind::kAttempt;
        std::size_t test = 0;
        /// Catalog indices to run first, in the same process.
        std::vector<std::size_t> before;
        bool fresh = false;
        std::optional<std::uint32_t> seed;
        /// For a probe: the tests that ran before it.
        std::vector<std::size_t> probeSequence;
    };

    /// One test being run in a worker.
    struct Running final
    {
        std::size_t catalogIndex = 0;
        bool target = false;
        Attempt attempt;
        Clock::time_point start;
        Clock::time_point deadline = Clock::time_point::max();
        bool fatal = false;
    };

    struct Worker final
    {
        std::size_t id = 0;
        std::unique_ptr<ChildProcess> process;
        std::thread outputReader;
        std::thread errorReader;
        bool ready = false;
        bool dedicated = false;
        bool quitting = false;
        bool outputClosed = false;
        bool errorsClosed = false;
        bool killedForTimeout = false;
        std::optional<std::uint32_t> seed;
        Clock::time_point started;
        std::optional<Job> job;
        /// Index into `job->before`, or its size for the target.
        std::size_t step = 0;
        std::optional<Running> running;
        /// Catalog indices this process has run, in order.
        std::vector<std::size_t> history;
        std::string startupOutput;
    };

    struct Event final
    {
        enum class Kind : std::uint8_t
        {
            kLine,
            kErrors,
            kOutputClosed,
            kErrorsClosed,
        };
        std::size_t worker = 0;
        Kind kind = Kind::kLine;
        std::string text;
    };

    /// The search for the test that leaves state behind.
    struct Bisection final
    {
        enum class Stage : std::uint8_t
        {
            kNone,
            kConfirming,
            kSearching,
            kDone,
        };
        Stage stage = Stage::kNone;
        std::vector<std::size_t> candidates;
        std::vector<std::size_t> probing;
        std::size_t probes = 0;
        std::optional<std::size_t> culprit;
        bool reproduced = false;
    };

    struct TestState final
    {
        TestResult result;
        bool queued = false;
        bool finished = false;
        std::size_t pendingJobs = 0;
        std::size_t retriesUsed = 0;
        std::size_t repeatsLeft = 0;
        Bisection bisection;
    };
} // namespace Progmasoft::Catch3::Runner::Scheduling
