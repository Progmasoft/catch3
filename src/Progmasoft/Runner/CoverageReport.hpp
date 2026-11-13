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

    /// Prepares the directory of a coverage run: the profiles and reports of
    /// an earlier run below it are removed, because gcov adds to the .gcda
    /// files it finds instead of replacing them, and because a run that
    /// writes no reports must not leave the earlier ones to be read as its
    /// own.
    Status
    PrepareCoverageDirectory(const std::filesystem::path &directory);

    /// The last attempt of `test` whose profile, its base name followed by
    /// `suffix` (`.profraw` or `.gcov`), exists: an attempt that crashed or
    /// timed out before writing one is passed over for an earlier one.
    [[nodiscard]] const Attempt *
    ProfiledAttempt(const TestResult &test, const std::string &suffix);

    /// What writing the coverage reports gave.
    struct CoverageReports final
    {
        /// What the run should be told: how many profiles there were and
        /// where the reports are, or why there are none.
        std::vector<std::string> notes;
        /// What failed: a profile that could not be read, a report that
        /// could not be written. A run with failures is missing a report.
        std::vector<std::string> failures;
    };

    /// Turns the profiles a coverage run left in `directory` into
    /// `directory/coverage.json`, the lines and branches each test reached,
    /// and `directory/coverage.lcov`, the run combined. LLVM profiles are
    /// read with `llvm-profdata` and `llvm-cov`, gcov data with `gcov`,
    /// found on PATH or named by CATCH3_LLVM_PROFDATA, CATCH3_LLVM_COV and
    /// CATCH3_GCOV; the profiles of the tests are read up to `jobs` at a
    /// time. A run whose tests wrote no profiles, or whose tools are
    /// missing, gets a note and no reports, not a failure.
    [[nodiscard]] CoverageReports
    WriteCoverageReports(const std::filesystem::path &directory,
                         const RunResult &run,
                         const std::string &program,
                         std::size_t jobs);
} // namespace Progmasoft::Catch3::Runner
