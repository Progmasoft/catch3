// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include "Catalog.hpp"
#include "Options.hpp"
#include "Planner.hpp"

#include <Progmasoft/Catch3/Results.hpp>

#include <functional>
#include <string>
#include <vector>

namespace Progmasoft::Catch3::Runner
{
    /// Hooks the console report uses to show progress.
    struct SchedulerEvents final
    {
        /// A test has its verdict.
        std::function<void(const TestResult &)> testFinished;
        /// An attempt ended and another will follow: a retry, a repeat.
        std::function<void(const TestResult &, const Attempt &)> retrying;
    };

    struct ScheduleOutcome final
    {
        /// One result per planned test, in plan order.
        std::vector<TestResult> tests;
        std::size_t workers = 0;
        std::vector<std::string> notes;
        /// The workers could not be started at all.
        bool infrastructureFailed = false;
    };

    /// Runs a plan: in worker processes started from `program`, or in this
    /// process with `--in-process`.
    ///
    /// Tests go to whichever worker is free, most important first. A worker
    /// that crashes or hangs is replaced; the test it was running is
    /// recorded as crashed or timed out. A failure is retried as the plan
    /// allows, first in a fresh process: a test that then passes was either
    /// flaky or broken by a test that ran before it in the same process,
    /// and the scheduler finds that test by bisecting the ones that ran
    /// before.
    [[nodiscard]] ScheduleOutcome
    RunPlan(const Options &options,
            const Plan &plan,
            const Catalog &catalog,
            const std::string &program,
            const std::string &programName,
            const SchedulerEvents &events);
} // namespace Progmasoft::Catch3::Runner
