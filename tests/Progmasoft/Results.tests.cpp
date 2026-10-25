// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Results.hpp>
#include <Progmasoft/Catch3/XmlWriter.hpp>

#include <Progmasoft/Catch3/TestMacros.hpp>

#include <sstream>
#include <string>

namespace
{
    using namespace Progmasoft::Catch3;

    Attempt
    MakeAttempt(Outcome outcome,
                std::int64_t microseconds,
                std::size_t number = 1)
    {
        Attempt attempt;
        attempt.outcome = outcome;
        attempt.number = number;
        attempt.durationMicroseconds = microseconds;
        if (IsFailure(outcome))
        {
            attempt.category = outcome == Outcome::kCrashed
                                   ? FailureCategory::kCrash
                               : outcome == Outcome::kTimedOut
                                   ? FailureCategory::kTimeout
                                   : FailureCategory::kAssertion;
        }
        return attempt;
    }

    TestResult
    MakeTest(const std::string &name, Outcome outcome, std::int64_t microseconds)
    {
        TestResult test;
        test.id = name;
        test.name = name;
        test.file = "tests/parser.cpp";
        test.line = 12;
        test.outcome = outcome;
        test.attempts.push_back(MakeAttempt(outcome, microseconds));
        test.category = test.attempts.back().category;
        return test;
    }
} // namespace

TEST_CASE("A run summary counts every verdict once",
          "[progmasoft][results]")
{
    RunResult run;
    run.binary = "parser-tests";
    run.wallMicroseconds = 2000000;
    run.tests.push_back(MakeTest("parses", Outcome::kPassed, 125000));
    run.tests.push_back(MakeTest("reports", Outcome::kFailed, 250000));
    run.tests.push_back(MakeTest("loads", Outcome::kCrashed, 500000));
    run.tests.push_back(MakeTest("waits", Outcome::kTimedOut, 1000000));
    auto flaky = MakeTest("retries", Outcome::kPassed, 0);
    flaky.flaky = true;
    flaky.attempts.insert(flaky.attempts.begin(),
                          MakeAttempt(Outcome::kFailed, 10000));
    run.tests.push_back(flaky);
    auto quarantined = MakeTest("quarantined", Outcome::kFailed, 1);
    quarantined.quarantined = true;
    run.tests.push_back(quarantined);

    const RunSummary summary = Summarize(run);
    CHECK(summary.tests == 6);
    CHECK(summary.passed == 2);
    CHECK(summary.failed == 2);
    CHECK(summary.crashed == 1);
    CHECK(summary.timedOut == 1);
    CHECK(summary.flaky == 1);
    CHECK(summary.quarantined == 1);
    CHECK(summary.attempts == 7);
    CHECK(summary.testMicroseconds == 1885001);

    CHECK(run.tests[1].FailsRun());
    CHECK_FALSE(run.tests[4].FailsRun());
    CHECK_FALSE(run.tests[5].FailsRun()); // In quarantine.
}

TEST_CASE("Outcome and category names read back", "[progmasoft][results]")
{
    for (const Outcome outcome : {Outcome::kPassed,
                                  Outcome::kFailed,
                                  Outcome::kFailedAsExpected,
                                  Outcome::kSkipped,
                                  Outcome::kCrashed,
                                  Outcome::kTimedOut,
                                  Outcome::kBlocked,
                                  Outcome::kNotRun,
                                  Outcome::kUpToDate})
    {
        CHECK(OutcomeFromName(OutcomeName(outcome)) == outcome);
    }
    CHECK(CategoryFromName(CategoryName(FailureCategory::kTimeout)) ==
          FailureCategory::kTimeout);
    CHECK_FALSE(OutcomeFromName("exploded").has_value());
}

TEST_CASE("A failure is described with its expansion, messages and sections",
          "[progmasoft][results]")
{
    Attempt attempt = MakeAttempt(Outcome::kFailed, 1000);
    AssertionRecord assertion;
    assertion.file = "parser.cpp";
    assertion.line = 40;
    assertion.macro = "REQUIRE";
    assertion.expression = "tokens.size() == 3";
    assertion.expansion = "2 == 3";
    assertion.message = "the comma was lost";
    assertion.context = {"input := a,b"};
    assertion.sections = {"lists", "with commas"};
    attempt.assertions.push_back(assertion);

    const std::string text = DescribeFailure(attempt);
    CHECK(text.find("parser.cpp:40: FAILED:") != std::string::npos);
    CHECK(text.find("REQUIRE( tokens.size() == 3 )") != std::string::npos);
    CHECK(text.find("with expansion:\n  2 == 3") != std::string::npos);
    CHECK(text.find("with message:\n  the comma was lost") != std::string::npos);
    CHECK(text.find("with context:\n  input := a,b") != std::string::npos);
    CHECK(text.find("in section lists / with commas") != std::string::npos);
    CHECK(SummarizeFailure(attempt) == "parser.cpp:40: FAILED:");
}

TEST_CASE("A crash and a timeout say where they stopped",
          "[progmasoft][results]")
{
    Attempt crash = MakeAttempt(Outcome::kCrashed, 1000);
    crash.exitDescription = "signal 11 (SIGSEGV)";
    crash.fatalMessage = "SIGSEGV - Segmentation violation signal";
    crash.lastLocation = "parser.cpp:51";
    SectionRecord open;
    open.name = "deep input";
    open.durationMicroseconds = -1;
    crash.sections.push_back(open);
    const std::string crashText = DescribeFailure(crash);
    CHECK(crashText.find("crashed with signal 11 (SIGSEGV)") !=
          std::string::npos);
    CHECK(crashText.find("Segmentation violation") != std::string::npos);
    CHECK(crashText.find("last assertion started at parser.cpp:51") !=
          std::string::npos);
    CHECK(crashText.find("in section deep input") != std::string::npos);

    const Attempt timeout = MakeAttempt(Outcome::kTimedOut, 2500000);
    CHECK(DescribeFailure(timeout).find("timed out after 2.5 s") !=
          std::string::npos);
}

TEST_CASE("JUnit XML keeps every attempt and marks flaky tests",
          "[progmasoft][results][xml]")
{
    RunResult run;
    run.binary = "parser-tests";
    run.seed = 42;
    run.workers = 4;
    run.wallMicroseconds = 263500;

    auto failed = MakeTest("reports <errors>", Outcome::kFailed, 263500);
    failed.className = "ParserTests";
    failed.attempts.back().standardOutput = "trace <start>&finish";
    AssertionRecord assertion;
    assertion.file = "parser.cpp";
    assertion.line = 7;
    assertion.expression = "ok";
    failed.attempts.back().assertions.push_back(assertion);
    failed.analysis.push_back("new failure after 3 passing run(s)");
    failed.replayCommand = "parser-tests \"reports <errors>\" --seed=42";
    run.tests.push_back(failed);

    auto crashed = MakeTest("loads", Outcome::kCrashed, 1);
    crashed.attempts.back().exitDescription = "signal 6 (SIGABRT)";
    run.tests.push_back(crashed);

    auto flaky = MakeTest("retries", Outcome::kPassed, 10);
    flaky.flaky = true;
    flaky.attempts.insert(flaky.attempts.begin(),
                          MakeAttempt(Outcome::kTimedOut, 5000000));
    run.tests.push_back(flaky);

    auto blocked = MakeTest("needs the parser", Outcome::kBlocked, 0);
    blocked.attempts.clear();
    blocked.analysis.push_back("depends on 'reports <errors>', which failed");
    run.tests.push_back(blocked);

    const std::string xml = JUnitXmlWriter::ToString(run);
    CHECK(xml.rfind("<?xml version=\"1.0\" encoding=\"UTF-8\"?>", 0) == 0);
    CHECK(xml.find("tests=\"4\" failures=\"1\" errors=\"1\" skipped=\"1\"") !=
          std::string::npos);
    CHECK(xml.find("name=\"reports &lt;errors>\"") != std::string::npos);
    CHECK(xml.find("classname=\"ParserTests\"") != std::string::npos);
    CHECK(xml.find("<failure message=\"parser.cpp:7: FAILED:\" "
                   "type=\"assertion\">") != std::string::npos);
    CHECK(xml.find("analysis: new failure after 3 passing run(s)") !=
          std::string::npos);
    CHECK(xml.find("<system-out>trace &lt;start&gt;&amp;finish</system-out>") !=
          std::string::npos);
    CHECK(xml.find("<error message=\"crashed with signal 6 (SIGABRT)\" "
                   "type=\"crash\">") != std::string::npos);
    CHECK(xml.find("<flakyFailure message=\"timed out after 5.0 s\" "
                   "type=\"timeout\">") != std::string::npos);
    CHECK(xml.find("<skipped message=\"depends on 'reports &lt;errors>', "
                   "which failed\" />") != std::string::npos);
    CHECK(xml.find("<property name=\"random-seed\" value=\"42\" />") !=
          std::string::npos);
    CHECK(xml.find("time=\"0.263500\"") != std::string::npos);
    CHECK(xml.ends_with("</testsuites>\n"));
}

TEST_CASE("JUnit XML replaces malformed UTF-8 and forbidden XML controls",
          "[progmasoft][results][xml]")
{
    RunResult run;
    run.binary = std::string("suite\0name", 10);
    auto test = MakeTest("valid", Outcome::kPassed, 0);
    test.name = std::string("bad\x01utf8\xF0\x28\x8C\x28", 12);
    run.tests.push_back(test);

    const std::string xml = JUnitXmlWriter::ToString(run);
    // U+FFFD, written as the character itself.
    CHECK(xml.find("name=\"bad\xEF\xBF\xBDutf8\xEF\xBF\xBD(\xEF\xBF\xBD(\"") !=
          std::string::npos);
    CHECK(xml.find("name=\"suite\xEF\xBF\xBDname\"") != std::string::npos);
}

TEST_CASE("The JUnit writer reports a failed stream", "[progmasoft][results][xml]")
{
    const RunResult empty;
    std::ostringstream output;
    CHECK(JUnitXmlWriter::Write(empty, output));
    CHECK(output.str().find("tests=\"0\" failures=\"0\" errors=\"0\" ") !=
          std::string::npos);

    std::ostringstream failed;
    failed.setstate(std::ios::badbit);
    CHECK_FALSE(JUnitXmlWriter::Write(empty, failed));
}
