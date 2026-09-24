// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <cmath>
#include <ios>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    Progmasoft::Catch3::TestCaseResult
    MakeCase( std::string name,
              Progmasoft::Catch3::TestCaseStatus status,
              double duration ) {
        Progmasoft::Catch3::TestCaseResult testCase;
        testCase.Name = std::move( name );
        testCase.Status = status;
        testCase.DurationSeconds = duration;
        return testCase;
    }

} // namespace

TEST_CASE( "Test result summaries count outcomes and duration",
           "[progmasoft][results]" ) {
    using namespace Progmasoft::Catch3;

    TestSuiteResult suite;
    suite.Name = "Parser";
    suite.TestCases = {
        MakeCase( "accepts input", TestCaseStatus::kPassed, 0.125 ),
        MakeCase( "rejects malformed input", TestCaseStatus::kFailed, 0.250 ),
        MakeCase( "handles I/O", TestCaseStatus::kErrored, 0.500 ),
        MakeCase( "optional backend", TestCaseStatus::kSkipped, 0.0 ),
    };
    const TestRunResult run{ "Compiler", { suite } };

    const TestRunSummary summary = SummarizeTestRun( run );
    CHECK( summary.Tests == 4 );
    CHECK( summary.Passed() == 1 );
    CHECK( summary.Failures == 1 );
    CHECK( summary.Errors == 1 );
    CHECK( summary.Skipped == 1 );
    CHECK( summary.DurationSeconds == 0.875 );
}

TEST_CASE( "JUnit XML writer serializes counts, details, and escaped text",
           "[progmasoft][results][xml]" ) {
    using namespace Progmasoft::Catch3;

    TestSuiteResult suite;
    suite.Name = "Parser & resolver";

    auto failed =
        MakeCase( "rejects <input>", TestCaseStatus::kFailed, 0.0125 );
    failed.ClassName = "CompilerTests";
    failed.SourceFile = "tests/parser.cpp";
    failed.Message = "assertion: \"expected\" & actual";
    failed.Details = "expected <node> but got &lt;none>";
    failed.StandardOutput = "trace <start>&finish";
    suite.TestCases.push_back( std::move( failed ) );

    auto errored = MakeCase( "throws", TestCaseStatus::kErrored, 0.25 );
    errored.Message = "runtime error";
    errored.Details = "first line\nsecond line";
    errored.StandardError = "failure detail";
    suite.TestCases.push_back( std::move( errored ) );

    auto skipped = MakeCase( "platform-only", TestCaseStatus::kSkipped, 0.0 );
    skipped.Message = "not available";
    suite.TestCases.push_back( std::move( skipped ) );

    suite.TestCases.push_back(
        MakeCase( "passes", TestCaseStatus::kPassed, 0.001 ) );
    const TestRunResult run{ "Compiler run", { std::move( suite ) } };

    const std::string xml = JUnitXmlWriter::ToString( run );
    CHECK( xml.starts_with( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ) );
    CHECK( xml.find( "<testsuites name=\"Compiler run\" tests=\"4\" " ) !=
           std::string::npos );
    CHECK( xml.find( "failures=\"1\" errors=\"1\" skipped=\"1\"" ) !=
           std::string::npos );
    CHECK( xml.find( "name=\"Parser &amp; resolver\"" ) != std::string::npos );
    CHECK( xml.find( "name=\"rejects &lt;input&gt;\"" ) != std::string::npos );
    CHECK( xml.find(
               "message=\"assertion: &quot;expected&quot; &amp; actual\"" ) !=
           std::string::npos );
    CHECK( xml.find( "expected &lt;node&gt; but got &amp;lt;none&gt;" ) !=
           std::string::npos );
    CHECK(
        xml.find( "<system-out>trace &lt;start&gt;&amp;finish</system-out>" ) !=
        std::string::npos );
    CHECK( xml.find( "<error message=\"runtime error\" type=\"TestError\">" ) !=
           std::string::npos );
    CHECK( xml.find( "<skipped message=\"not available\"/>" ) !=
           std::string::npos );
    CHECK( xml.find( "classname=\"CompilerTests\"" ) != std::string::npos );
    CHECK( xml.find( "file=\"tests/parser.cpp\"" ) != std::string::npos );
    CHECK( xml.find( "time=\"0.263500\"" ) != std::string::npos );
    CHECK( xml.ends_with( "</testsuites>\n" ) );
}

TEST_CASE( "JUnit XML replaces malformed UTF-8 and forbidden XML controls",
           "[progmasoft][results][xml]" ) {
    using namespace Progmasoft::Catch3;

    auto testCase = MakeCase( "valid", TestCaseStatus::kPassed, 0.0 );
    testCase.Name = std::string( "bad\x01utf8\xF0\x28\x8C\x28", 12 );
    const TestRunResult run{ std::string( "suite\0name", 10 ),
                             { { "suite", { testCase } } } };

    const std::string xml = JUnitXmlWriter::ToString( run );
    CHECK( xml.find( "name=\"bad&#xFFFD;utf8&#xFFFD;(&#xFFFD;(\"" ) !=
           std::string::npos );
    CHECK( xml.find( "name=\"suite&#xFFFD;name\"" ) != std::string::npos );
}

TEST_CASE( "Result summaries reject invalid durations and statuses",
           "[progmasoft][results]" ) {
    using namespace Progmasoft::Catch3;

    TestCaseResult invalidDuration =
        MakeCase( "invalid", TestCaseStatus::kPassed, -1.0 );
    CHECK_THROWS_AS(
        SummarizeTestRun( { "run", { { "suite", { invalidDuration } } } } ),
        std::invalid_argument );

    invalidDuration.DurationSeconds = std::numeric_limits<double>::infinity();
    CHECK_THROWS_AS(
        SummarizeTestRun( { "run", { { "suite", { invalidDuration } } } } ),
        std::invalid_argument );

    invalidDuration.DurationSeconds = 0.0;
    invalidDuration.Status = static_cast<TestCaseStatus>( 255 );
    CHECK_THROWS_AS(
        SummarizeTestRun( { "run", { { "suite", { invalidDuration } } } } ),
        std::invalid_argument );
}

TEST_CASE( "JUnit XML writer validates results and output stream state",
           "[progmasoft][results][xml]" ) {
    using namespace Progmasoft::Catch3;

    const TestRunResult emptyRun{ "empty", {} };
    const std::string emptyXml = JUnitXmlWriter::ToString( emptyRun );
    CHECK( emptyXml.find( "tests=\"0\" failures=\"0\" errors=\"0\" " ) !=
           std::string::npos );

    auto invalidCase = MakeCase( "bad", TestCaseStatus::kPassed, 0.0 );
    invalidCase.DurationSeconds = std::numeric_limits<double>::quiet_NaN();
    const TestRunResult invalidRun{ "invalid",
                                    { { "suite", { invalidCase } } } };
    std::ostringstream output;
    CHECK_THROWS_AS( JUnitXmlWriter::Write( invalidRun, output ),
                     std::invalid_argument );

    std::ostringstream failedOutput;
    failedOutput.setstate( std::ios::badbit );
    CHECK_THROWS_AS( JUnitXmlWriter::Write( emptyRun, failedOutput ),
                     std::ios_base::failure );
}
