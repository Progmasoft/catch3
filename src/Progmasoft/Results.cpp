// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Results.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace Progmasoft::Catch3 {

    namespace {

        void AccumulateCases( TestRunSummary& summary,
                              const std::vector<TestCaseResult>& testCases ) {
            for ( const TestCaseResult& testCase : testCases ) {
                if ( !std::isfinite( testCase.DurationSeconds ) ||
                     testCase.DurationSeconds < 0.0 ) {
                    throw std::invalid_argument(
                        "test duration must be finite and non-negative" );
                }
                if ( summary.Tests ==
                     std::numeric_limits<std::size_t>::max() ) {
                    throw std::overflow_error( "test count overflow" );
                }

                ++summary.Tests;
                summary.DurationSeconds += testCase.DurationSeconds;
                if ( !std::isfinite( summary.DurationSeconds ) ) {
                    throw std::overflow_error( "test duration total overflow" );
                }

                switch ( testCase.Status ) {
                case TestCaseStatus::kPassed:
                    break;
                case TestCaseStatus::kFailed:
                    ++summary.Failures;
                    break;
                case TestCaseStatus::kErrored:
                    ++summary.Errors;
                    break;
                case TestCaseStatus::kSkipped:
                    ++summary.Skipped;
                    break;
                default:
                    throw std::invalid_argument( "unknown test case status" );
                }
            }
        }

    } // namespace

    std::size_t TestRunSummary::Passed() const noexcept {
        if ( Failures > Tests || Errors > Tests - Failures ) { return 0; }
        const std::size_t failuresAndErrors = Failures + Errors;
        if ( Skipped > Tests - failuresAndErrors ) { return 0; }
        return Tests - failuresAndErrors - Skipped;
    }

    TestRunSummary SummarizeTestSuite( const TestSuiteResult& result ) {
        TestRunSummary summary;
        AccumulateCases( summary, result.TestCases );
        return summary;
    }

    TestRunSummary SummarizeTestRun( const TestRunResult& result ) {
        TestRunSummary summary;
        for ( const TestSuiteResult& suite : result.TestSuites ) {
            AccumulateCases( summary, suite.TestCases );
        }
        return summary;
    }

} // namespace Progmasoft::Catch3
