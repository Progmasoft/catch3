// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Progmasoft::Catch3 {

    enum class TestCaseStatus : unsigned char {
        kPassed,
        kFailed,
        kErrored,
        kSkipped,
    };

    /// Framework-neutral result for one completed test case.
    struct TestCaseResult final {
        std::string Name;
        std::string ClassName;
        std::string SourceFile;
        TestCaseStatus Status = TestCaseStatus::kPassed;
        double DurationSeconds = 0.0;
        std::string Message;
        std::string Details;
        std::string StandardOutput;
        std::string StandardError;
    };

    struct TestSuiteResult final {
        std::string Name;
        std::vector<TestCaseResult> TestCases;
    };

    struct TestRunResult final {
        std::string Name;
        std::vector<TestSuiteResult> TestSuites;
    };

    struct TestRunSummary final {
        std::size_t Tests = 0;
        std::size_t Failures = 0;
        std::size_t Errors = 0;
        std::size_t Skipped = 0;
        double DurationSeconds = 0.0;

        [[nodiscard]] std::size_t Passed() const noexcept;
    };

    /// Validates durations and statuses while aggregating nested test results.
    [[nodiscard]] TestRunSummary
    SummarizeTestSuite( const TestSuiteResult& result );
    [[nodiscard]] TestRunSummary
    SummarizeTestRun( const TestRunResult& result );

} // namespace Progmasoft::Catch3
