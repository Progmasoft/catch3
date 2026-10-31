// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include "Options.hpp"

#include <Progmasoft/Catch3/Results.hpp>

#include <cstddef>
#include <cstdio>
#include <string>

namespace Progmasoft::Catch3::Runner
{
    /// The report on the terminal: progress while the run goes on, then the
    /// failures with their analysis and a summary.
    class ConsoleReport final
    {
    public:
        ConsoleReport(const Options &options, std::FILE *output);

        void
        RunStarting(std::size_t tests,
                    std::size_t workers,
                    std::uint32_t seed,
                    const std::vector<std::string> &notes);

        void
        TestFinished(const TestResult &test);

        void
        Retrying(const TestResult &test, const Attempt &attempt);

        void
        RunEnded(const RunResult &run);

        /// Text in a colour, when colours are on.
        [[nodiscard]] std::string
        Paint(std::string_view text, const char *colour) const;

    private:
        void
        ClearProgress();

        void
        PrintFailure(const TestResult &test);

        const Options &options_;
        std::FILE *output_;
        bool colour_ = false;
        bool terminal_ = false;
        std::size_t total_ = 0;
        std::size_t finished_ = 0;
        std::size_t failed_ = 0;
        bool progressShown_ = false;
    };

    /// Writes a trace of the run in the Trace Event Format that
    /// chrome://tracing and Perfetto open: one track per worker, one slice
    /// per attempt and section.
    [[nodiscard]] std::string
    TraceJson(const RunResult &run);
} // namespace Progmasoft::Catch3::Runner
