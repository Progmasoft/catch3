// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include "Git.hpp"
#include "History.hpp"
#include "Planner.hpp"

#include <Progmasoft/Catch3/Results.hpp>

#include <string>

namespace Progmasoft::Catch3::Runner
{
    struct AnalysisInputs final
    {
        /// The history before this run.
        const History *history = nullptr;
        const Changes *changes = nullptr;
        const Plan *plan = nullptr;
        /// The binary, for the replay command.
        std::string program;
        std::string changesSince;
    };

    /// Classifies every failure of the run and writes down what the runner
    /// can tell about its cause: a signature shared with other failures,
    /// the failure's history, the change it follows, where a crash or a
    /// hang happened, and a command that replays it.
    void
    Analyze(RunResult &run, const AnalysisInputs &inputs);

    /// The signature of a failed attempt: its category, the location of the
    /// first failure and its message with numbers and addresses replaced.
    [[nodiscard]] std::string
    FailureSignature(const Attempt &attempt);

    /// A test spec that matches exactly this name.
    [[nodiscard]] std::string
    QuoteTestSpec(const std::string &name);
} // namespace Progmasoft::Catch3::Runner
