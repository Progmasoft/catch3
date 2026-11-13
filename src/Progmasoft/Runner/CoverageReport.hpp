// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Results.hpp>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Expected.hpp"

namespace Progmasoft::Catch3::Runner
{
    /// What one test, or the whole run, reached in one source file.
    struct FileCoverage final
    {
        /// Execution count of each instrumented line.
        std::map<std::size_t, std::uint64_t> lines;
        /// Count of each branch, by line and by its position on the line.
        std::map<std::pair<std::size_t, std::size_t>, std::uint64_t> branches;
    };

    /// Coverage by source file.
    using CoverageMap = std::map<std::string, FileCoverage>;

    /// Reads an LCOV tracefile: `SF`, `DA` and `BRDA` records.
    [[nodiscard]] CoverageMap
    ParseLcov(const std::string &text);

    /// Reads the JSON that `gcov --json-format` writes, adding to `map`.
    void
    AddGcovJson(const std::string &text, CoverageMap &map);

    /// Writes `map` as an LCOV tracefile.
    [[nodiscard]] std::string
    ToLcov(const CoverageMap &map);

    /// Prepares the directory of a coverage run: the profiles of an earlier
    /// run below it are removed, because gcov adds to the .gcda files it
    /// finds instead of replacing them.
    Status
    PrepareCoverageDirectory(const std::filesystem::path &directory);

    /// Turns the profiles a coverage run left in `directory` into
    /// `directory/coverage.json`, the lines and branches each test reached,
    /// and `directory/coverage.lcov`, the run combined. LLVM profiles are
    /// read with `llvm-profdata` and `llvm-cov`, gcov data with `gcov`,
    /// found on PATH or named by CATCH3_LLVM_PROFDATA, CATCH3_LLVM_COV and
    /// CATCH3_GCOV. Returns what the run should be told: how many profiles
    /// there were and where the reports are, or why there are none.
    [[nodiscard]] std::vector<std::string>
    WriteCoverageReports(const std::filesystem::path &directory,
                         const RunResult &run,
                         const std::string &program);
} // namespace Progmasoft::Catch3::Runner
