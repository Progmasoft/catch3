// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// Reading the reports of other test frameworks, and what `--import` does with
// them.

#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Runner/History.hpp>
#include <Progmasoft/Runner/Options.hpp>
#include <Progmasoft/Runner/ResultImport.hpp>
#include <Progmasoft/Runner/ResultsJson.hpp>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "RunnerFixtureClient.hpp"

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using namespace Progmasoft::Catch3;
    using Catch::Matchers::ContainsSubstring;
    using FixtureClient::ExitCode;
    using FixtureClient::RunFixture;
    using FixtureClient::Scratch;

    std::vector<TestResult>
    Import(std::string_view text,
           Runner::ImportFormat format = Runner::ImportFormat::kAuto)
    {
        auto tests = Runner::ParseImport(text, format);
        INFO((tests ? std::string{} : tests.GetError().Message()));
        REQUIRE(tests);
        return Runner::MergeImported(std::move(*tests));
    }

    const TestResult &
    Named(const std::vector<TestResult> &tests, const std::string &name)
    {
        for (const auto &test : tests)
        {
            if (test.name == name)
            {
                return test;
            }
        }
        FAIL("no result for '" << name << "'");
        return tests.front();
    }

    constexpr std::string_view kJUnit = R"(<?xml version="1.0"?>
<!-- written by some tool -->
<testsuites>
  <testsuite name="outer" file="outer.py">
    <testsuite name="inner">
      <testcase classname="dates" name="parses" time="0.25"/>
      <testcase classname="dates" name="rounds" time="1,000.5" line="7">
        <failure message="1.5 != 2" type="AssertionError">at rounds:7</failure>
        <system-out>said something</system-out>
      </testcase>
      <testcase classname="dates" name="throws">
        <error type="ValueError"><![CDATA[Traceback
ValueError: no]]></error>
      </testcase>
      <testcase classname="dates" name="ignored">
        <skipped message="not today"/>
      </testcase>
      <testcase classname="dates" name="disabled" status="notrun"/>
      <testcase classname="dates" name="retried" time="2">
        <flakyFailure message="first try" type="E" time="1">
          <stackTrace>at retried:1</stackTrace>
        </flakyFailure>
      </testcase>
      <testcase classname="dates" name="always">
        <failure message="last"/>
        <rerunFailure message="again"/>
      </testcase>
    </testsuite>
  </testsuite>
</testsuites>)";

    constexpr std::string_view kXUnit = R"(<assemblies>
  <assembly name="Tests.dll">
    <errors>
      <error type="fixture" name="Dates.Setup">
        <failure><message>no database</message></failure>
      </error>
    </errors>
    <collection name="Dates">
      <test name="Dates.Parses" type="Dates" method="Parses" time="0.5" result="Pass">
        <traits><trait name="kind" value="unit"/></traits>
      </test>
      <test name="Dates.Rounds" type="Dates" time="0.1" result="Fail"
            source-file="Dates.cs" source-line="12">
        <failure exception-type="Xunit.EqualException">
          <message>not equal</message>
          <stack-trace>at Dates.Rounds</stack-trace>
        </failure>
      </test>
      <test name="Dates.Later" type="Dates" result="Skip"><reason>later</reason></test>
    </collection>
  </assembly>
</assemblies>)";

    constexpr std::string_view kTrx
        = R"xml(<?xml version="1.0" encoding="utf-8"?>
<TestRun xmlns="http://microsoft.com/schemas/VisualStudio/TeamTest/2010">
  <TestDefinitions>
    <UnitTest id="1"><TestMethod className="Ns.Dates, Tests, Version=1.0" name="Parses"/></UnitTest>
    <UnitTest id="2"><TestMethod className="Ns.Dates" name="Rounds"/></UnitTest>
    <UnitTest id="3"><TestMethod className="Ns.Dates" name="Rows"/></UnitTest>
  </TestDefinitions>
  <Results>
    <UnitTestResult testId="1" testName="Parses" outcome="Passed" duration="00:01:02.5000000"/>
    <UnitTestResult testId="2" testName="Rounds" outcome="Failed" duration="1.00:00:00">
      <Output>
        <StdOut>said something</StdOut>
        <ErrorInfo><Message>Assert.Equal</Message><StackTrace>at Rounds</StackTrace></ErrorInfo>
      </Output>
    </UnitTestResult>
    <UnitTestResult testId="2" testName="Rounds" outcome="Passed" duration="00:00:01"/>
    <UnitTestResult testId="3" testName="Rows" outcome="Failed">
      <InnerResults>
        <UnitTestResult testId="3" testName="Rows (1)" outcome="Passed"/>
        <UnitTestResult testId="3" testName="Rows (2)" outcome="Timeout"/>
        <UnitTestResult testId="3" testName="Rows (3)" outcome="Aborted"/>
        <UnitTestResult testId="3" testName="Rows (4)" outcome="NotExecuted"/>
      </InnerResults>
    </UnitTestResult>
  </Results>
</TestRun>)xml";

    constexpr std::string_view kTap = R"(TAP version 14
1..6
ok 1 - parses
not ok 2 - rounds
  ---
  message: "1.5 != 2"
  duration_ms: 1500
  ...
# it was close
ok 3 - later # SKIP not today
not ok 4 known bug # TODO fix it
ok 5
# Subtest: group
    ok 1 - parses
    1..1
ok 6 - parses
)";

    constexpr std::string_view kCtrf = R"({
  "results": {
    "tool": { "name": "vitest" },
    "tests": [
      { "name": "parses", "status": "passed", "duration": 250, "suite": ["dates", "parse"] },
      { "name": "rounds", "status": "failed", "duration": 10, "message": "1.5 != 2",
        "trace": "at rounds", "filePath": "dates.test.ts", "line": 7, "tags": ["@unit"],
        "stdout": ["one", "two"] },
      { "name": "later", "status": "pending" },
      { "name": "retried", "status": "passed", "flaky": true, "retries": 1,
        "retryAttempts": [ { "attempt": 1, "status": "failed", "message": "first" } ] },
      { "name": "repeated", "status": "failed", "message": "second", "retries": 1,
        "retryAttempts": [ { "attempt": 1, "status": "failed", "message": "first" } ] }
    ]
  }
})";

    constexpr std::string_view kLines
        = R"({"format": "catch3-results", "version": 1}
{"name": "parses", "className": "Dates", "outcome": "passed", "durationMicroseconds": 1200}

{"name": "rounds", "outcome": "failed", "message": "1.5 != 2", "details": "at rounds"}
{"id": "custom-id", "name": "crashes", "outcome": "crashed"}
)";
} // namespace

TEST_CASE("The format of a report shows in its contents",
          "[progmasoft][runner][import]")
{
    using Runner::DetectImportFormat;
    using Runner::ImportFormat;
    CHECK(DetectImportFormat(kJUnit) == ImportFormat::kJUnit);
    CHECK(DetectImportFormat("\xEF\xBB\xBF  <testsuite/>")
          == ImportFormat::kJUnit);
    CHECK(DetectImportFormat(kXUnit) == ImportFormat::kXUnit);
    CHECK(DetectImportFormat(kTrx) == ImportFormat::kTrx);
    CHECK(DetectImportFormat(kTap) == ImportFormat::kTap);
    CHECK(DetectImportFormat("# comment\nnot ok 1 - x\n")
          == ImportFormat::kTap);
    CHECK(DetectImportFormat("1..0 # SKIP nothing\n") == ImportFormat::kTap);
    CHECK(DetectImportFormat(kCtrf) == ImportFormat::kCtrf);
    CHECK(DetectImportFormat(kLines) == ImportFormat::kCatch3);
    CHECK(DetectImportFormat(R"({"format": "catch3-run", "tests": []})")
          == ImportFormat::kCatch3);

    CHECK_FALSE(DetectImportFormat(""));
    CHECK_FALSE(DetectImportFormat("<html></html>"));
    CHECK_FALSE(DetectImportFormat("{\"other\": 1}"));
    CHECK_FALSE(DetectImportFormat("okay then"));
    CHECK_FALSE(DetectImportFormat("{ not json"));
}

TEST_CASE("Reports that cannot be read say why", "[progmasoft][runner][import]")
{
    using Runner::ImportFormat;
    const auto error = [](std::string_view text, ImportFormat format) {
        auto tests = Runner::ParseImport(text, format);
        REQUIRE_FALSE(tests);
        return tests.GetError().Message();
    };
    CHECK_THAT(error("hello", ImportFormat::kAuto),
               ContainsSubstring("--import-format"));
    CHECK_THAT(error("<testsuite>", ImportFormat::kJUnit),
               ContainsSubstring("not valid XML"));
    CHECK_THAT(error("<html/>", ImportFormat::kJUnit),
               ContainsSubstring("html"));
    CHECK_THAT(error("<html/>", ImportFormat::kXUnit),
               ContainsSubstring("html"));
    CHECK_THAT(error("<html/>", ImportFormat::kTrx), ContainsSubstring("html"));
    CHECK_THAT(error("hello\n", ImportFormat::kTap),
               ContainsSubstring("no TAP"));
    CHECK_THAT(error("{}", ImportFormat::kCtrf),
               ContainsSubstring("results.tests"));
    CHECK_THAT(error(R"({"results": {"tests": [{"status": "passed"}]}})",
                     ImportFormat::kCtrf),
               ContainsSubstring("no name"));
    CHECK_THAT(error("{\"name\": \"x\"}\n", ImportFormat::kCatch3),
               ContainsSubstring("catch3-results"));
    CHECK_THAT(error("{\"format\": \"catch3-results\", \"version\": 99}\n",
                     ImportFormat::kCatch3),
               ContainsSubstring("newer"));
    CHECK_THAT(
        error("{\"format\": \"catch3-results\"}\n{\"outcome\": \"passed\"}\n",
              ImportFormat::kCatch3),
        ContainsSubstring("line 2"));
    CHECK_THAT(error("{\"format\": \"catch3-results\"}\n"
                     "{\"name\": \"x\", \"outcome\": \"green\"}\n",
                     ImportFormat::kCatch3),
               ContainsSubstring("green"));
    CHECK_THAT(error("{\"format\": \"catch3-results\"}\n{ broken\n",
                     ImportFormat::kCatch3),
               ContainsSubstring("line 2"));
}

TEST_CASE("JUnit reports are imported", "[progmasoft][runner][import]")
{
    const auto tests = Import(kJUnit);
    REQUIRE(tests.size() == 7);

    const auto &parses = Named(tests, "parses");
    CHECK(parses.id == "dates::parses");
    CHECK(parses.className == "dates");
    CHECK(parses.outcome == Outcome::kPassed);
    CHECK(parses.file == "outer.py");
    CHECK(parses.attempts.front().durationMicroseconds == 250000);

    const auto &rounds = Named(tests, "rounds");
    CHECK(rounds.outcome == Outcome::kFailed);
    CHECK(rounds.category == FailureCategory::kAssertion);
    CHECK(rounds.line == 7);
    const auto &attempt = rounds.attempts.front();
    CHECK(attempt.durationMicroseconds == 1000500000);
    CHECK(attempt.standardOutput == "said something");
    REQUIRE(attempt.assertions.size() == 1);
    CHECK(attempt.assertions.front().message == "1.5 != 2");
    CHECK(attempt.assertions.front().macro == "AssertionError");
    CHECK(attempt.assertions.front().expansion == "at rounds:7");

    const auto &throws = Named(tests, "throws");
    CHECK(throws.category == FailureCategory::kUnexpectedException);
    CHECK(throws.attempts.front().assertions.front().message == "Traceback");

    CHECK(Named(tests, "ignored").outcome == Outcome::kSkipped);
    CHECK(Named(tests, "ignored").attempts.front().standardOutput
          == "not today");
    CHECK(Named(tests, "disabled").outcome == Outcome::kSkipped);

    const auto &retried = Named(tests, "retried");
    CHECK(retried.outcome == Outcome::kPassed);
    CHECK(retried.flaky);
    REQUIRE(retried.attempts.size() == 2);
    CHECK(retried.attempts.front().outcome == Outcome::kFailed);
    CHECK(retried.attempts.front().number == 1);
    CHECK(retried.attempts.back().number == 2);
    CHECK(retried.attempts.back().durationMicroseconds == 2000000);

    const auto &always = Named(tests, "always");
    CHECK(always.outcome == Outcome::kFailed);
    CHECK_FALSE(always.flaky);
    CHECK(always.attempts.size() == 2);
}

TEST_CASE("Several failures of one JUnit test are one attempt",
          "[progmasoft][runner][import]")
{
    const auto tests = Import(R"(<testsuite><testcase name="both">
        <failure message="one"/><error message="two"/>
    </testcase></testsuite>)");
    REQUIRE(tests.size() == 1);
    CHECK(tests.front().id == "both");
    REQUIRE(tests.front().attempts.size() == 1);
    CHECK(tests.front().attempts.front().assertions.size() == 2);
    CHECK(tests.front().attempts.front().assertionsFailed == 2);
}

TEST_CASE("xUnit reports are imported", "[progmasoft][runner][import]")
{
    const auto tests = Import(kXUnit);
    REQUIRE(tests.size() == 4);

    const auto &parses = Named(tests, "Dates.Parses");
    CHECK(parses.outcome == Outcome::kPassed);
    CHECK(parses.className == "Dates");
    CHECK(parses.tags == std::vector<std::string>{ "kind=unit" });

    const auto &rounds = Named(tests, "Dates.Rounds");
    CHECK(rounds.outcome == Outcome::kFailed);
    CHECK(rounds.file == "Dates.cs");
    CHECK(rounds.line == 12);
    const auto &assertion = rounds.attempts.front().assertions.front();
    CHECK(assertion.message == "not equal");
    CHECK(assertion.macro == "Xunit.EqualException");

    CHECK(Named(tests, "Dates.Later").outcome == Outcome::kSkipped);
    CHECK(Named(tests, "fixture Dates.Setup").category
          == FailureCategory::kInfrastructure);
}

TEST_CASE("One assembly is an xUnit report too", "[progmasoft][runner][import]")
{
    const auto tests = Import(
        R"(<assembly><test name="a" result="Pass"/><test name="b" result="NotRun"/></assembly>)");
    REQUIRE(tests.size() == 2);
    CHECK(Named(tests, "b").outcome == Outcome::kNotRun);
}

TEST_CASE("TRX reports are imported", "[progmasoft][runner][import]")
{
    const auto tests = Import(kTrx);
    REQUIRE(tests.size() == 6);

    const auto &parses = Named(tests, "Parses");
    CHECK(parses.className == "Ns.Dates");
    CHECK(parses.attempts.front().durationMicroseconds == 62500000);

    // The same test twice is a retry.
    const auto &rounds = Named(tests, "Rounds");
    CHECK(rounds.outcome == Outcome::kPassed);
    CHECK(rounds.flaky);
    REQUIRE(rounds.attempts.size() == 2);
    CHECK(rounds.attempts.front().durationMicroseconds == 86400000000);
    CHECK(rounds.attempts.front().standardOutput == "said something");
    CHECK(rounds.attempts.front().assertions.front().message == "Assert.Equal");

    CHECK(Named(tests, "Rows (1)").outcome == Outcome::kPassed);
    CHECK(Named(tests, "Rows (2)").outcome == Outcome::kTimedOut);
    CHECK(Named(tests, "Rows (2)").category == FailureCategory::kTimeout);
    CHECK(Named(tests, "Rows (3)").outcome == Outcome::kCrashed);
    CHECK(Named(tests, "Rows (4)").outcome == Outcome::kSkipped);
}

TEST_CASE("TAP streams are imported", "[progmasoft][runner][import]")
{
    const auto tests = Import(kTap);
    REQUIRE(tests.size() == 7);

    CHECK(Named(tests, "parses").outcome == Outcome::kPassed);

    const auto &rounds = Named(tests, "rounds");
    CHECK(rounds.outcome == Outcome::kFailed);
    CHECK(rounds.attempts.front().durationMicroseconds == 1500000);
    const auto &assertion = rounds.attempts.front().assertions.front();
    CHECK(assertion.message == "1.5 != 2");
    CHECK_THAT(assertion.expansion, ContainsSubstring("duration_ms"));
    CHECK_THAT(assertion.expansion, ContainsSubstring("it was close"));

    CHECK(Named(tests, "later").outcome == Outcome::kSkipped);
    CHECK(Named(tests, "known bug").outcome == Outcome::kFailedAsExpected);
    CHECK_FALSE(Named(tests, "known bug").FailsRun());
    CHECK(Named(tests, "test 1").outcome == Outcome::kPassed);

    // A name in a subtest is its own, and one that comes back is another test.
    bool nested = false;
    for (const auto &test : tests)
    {
        nested = nested || test.id == "group::parses";
    }
    CHECK(nested);
    CHECK(Named(tests, "parses #2").outcome == Outcome::kPassed);
}

TEST_CASE("TAP keeps escaped hashes in descriptions",
          "[progmasoft][runner][import]")
{
    const auto tests = Import("ok 1 - issue \\#12 # skip\n");
    REQUIRE(tests.size() == 1);
    CHECK(tests.front().name == "issue #12");
    CHECK(tests.front().outcome == Outcome::kSkipped);
}

TEST_CASE("TAP directives are whole words", "[progmasoft][runner][import]")
{
    CHECK(Import("not ok 1 - a # TODOING\n").front().outcome
          == Outcome::kFailed);
    CHECK(Import("not ok 1 - a # skipper\n").front().outcome
          == Outcome::kFailed);
    CHECK(Import("not ok 1 - a # TODO: later\n").front().outcome
          == Outcome::kFailedAsExpected);
    CHECK(Import("not ok 1 - a # SKIPPED on Windows\n").front().outcome
          == Outcome::kSkipped);
    CHECK(Import("ok 1 - a # skipping\n").front().outcome == Outcome::kSkipped);
    CHECK(Import("ok 1 - a # Skip: not today\n")
              .front()
              .attempts.front()
              .standardOutput
          == "not today");
}

TEST_CASE("TAP tests belong to their stream", "[progmasoft][runner][import]")
{
    auto first = Runner::ParseImport("not ok 1\n",
                                     Runner::ImportFormat::kTap,
                                     "linux.tap");
    auto second = Runner::ParseImport("ok 1\n",
                                      Runner::ImportFormat::kTap,
                                      "windows.tap");
    REQUIRE(first);
    REQUIRE(second);
    std::vector<TestResult> all = *first;
    all.insert(all.end(), second->begin(), second->end());

    const auto tests = Runner::MergeImported(std::move(all));
    REQUIRE(tests.size() == 2);
    CHECK(tests.front().id == "linux.tap::test 1");
    CHECK(tests.front().outcome == Outcome::kFailed);
    CHECK_FALSE(tests.front().flaky);
}

TEST_CASE("A TAP stream that ends early fails", "[progmasoft][runner][import]")
{
    const auto bailed = Import("1..2\nok 1 - a\nBail out! no database\n");
    CHECK(Named(bailed, "the TAP stream").outcome == Outcome::kFailed);
    CHECK(Named(bailed, "the TAP stream").category
          == FailureCategory::kInfrastructure);
    CHECK(Named(bailed, "the TAP stream")
              .attempts.front()
              .assertions.front()
              .message
          == "Bail out! no database");

    const auto shorter = Import("1..3\nok 1 - a\n");
    CHECK_THAT(Named(shorter, "the TAP stream")
                   .attempts.front()
                   .assertions.front()
                   .message,
               ContainsSubstring("planned 3"));

    CHECK(Import("1..0 # SKIP nothing here\n").empty());
    CHECK(Import("ok 1 - a\nok 2 - b\n").size() == 2);
}

TEST_CASE("CTRF reports are imported", "[progmasoft][runner][import]")
{
    const auto tests = Import(kCtrf);
    REQUIRE(tests.size() == 5);

    const auto &parses = Named(tests, "parses");
    CHECK(parses.className == "dates > parse");
    CHECK(parses.attempts.front().durationMicroseconds == 250000);

    const auto &rounds = Named(tests, "rounds");
    CHECK(rounds.outcome == Outcome::kFailed);
    CHECK(rounds.file == "dates.test.ts");
    CHECK(rounds.line == 7);
    CHECK(rounds.tags == std::vector<std::string>{ "@unit" });
    CHECK(rounds.attempts.front().standardOutput == "one\ntwo\n");
    CHECK(rounds.attempts.front().assertions.front().expansion == "at rounds");

    CHECK(Named(tests, "later").outcome == Outcome::kSkipped);

    const auto &retried = Named(tests, "retried");
    CHECK(retried.flaky);
    CHECK(retried.outcome == Outcome::kPassed);
    CHECK(retried.attempts.size() == 2);

    // The retries end before the last attempt, whatever its outcome.
    const auto &repeated = Named(tests, "repeated");
    CHECK(repeated.outcome == Outcome::kFailed);
    REQUIRE(repeated.attempts.size() == 2);
    CHECK(repeated.attempts.back().assertions.front().message == "second");
}

TEST_CASE("catch3-results lines are imported", "[progmasoft][runner][import]")
{
    const auto tests = Import(kLines);
    REQUIRE(tests.size() == 3);
    CHECK(Named(tests, "parses").id == "Dates::parses");
    CHECK(Named(tests, "parses").attempts.front().durationMicroseconds == 1200);
    const auto &rounds = Named(tests, "rounds");
    CHECK(rounds.category == FailureCategory::kAssertion);
    CHECK(rounds.attempts.front().assertions.front().expansion == "at rounds");
    CHECK(Named(tests, "crashes").id == "custom-id");
    CHECK(Named(tests, "crashes").category == FailureCategory::kCrash);
}

TEST_CASE("Catch3's own report is imported without its old conclusions",
          "[progmasoft][runner][import]")
{
    RunResult run;
    TestResult test;
    test.id = "a";
    test.name = "a";
    test.outcome = Outcome::kFailed;
    test.quarantined = true;
    test.signature = "sig-old";
    test.analysis = { "old" };
    test.replayCommand = "tests a";
    test.attempts.push_back(Attempt{});
    test.attempts.back().outcome = Outcome::kFailed;
    run.tests.push_back(test);

    const auto tests = Import(Runner::ToJson(run).ToString());
    REQUIRE(tests.size() == 1);
    CHECK(tests.front().outcome == Outcome::kFailed);
    CHECK_FALSE(tests.front().quarantined);
    CHECK(tests.front().signature.empty());
    CHECK(tests.front().analysis.empty());
    CHECK(tests.front().replayCommand.empty());
}

TEST_CASE("Results that share an identifier are attempts",
          "[progmasoft][runner][import]")
{
    const auto one = Runner::ParseImport(
        "<testsuite><testcase name=\"a\"><failure/></testcase>"
        "<testcase name=\"b\"/></testsuite>",
        Runner::ImportFormat::kJUnit);
    const auto two
        = Runner::ParseImport("<testsuite><testcase name=\"a\"/></testsuite>",
                              Runner::ImportFormat::kJUnit);
    REQUIRE(one);
    REQUIRE(two);
    std::vector<TestResult> all = *one;
    all.insert(all.end(), two->begin(), two->end());

    const auto tests = Runner::MergeImported(std::move(all));
    REQUIRE(tests.size() == 2);
    CHECK(tests.front().name == "a");
    CHECK(tests.front().flaky);
    CHECK(tests.front().outcome == Outcome::kPassed);
    CHECK(tests.front().category == FailureCategory::kNone);
    CHECK_THAT(tests.front().analysis.front(),
               ContainsSubstring("failed 1 of 2"));
    CHECK_FALSE(tests.back().flaky);
}

TEST_CASE("A failure is not lost to a later skip",
          "[progmasoft][runner][import]")
{
    const auto tests = Import(R"(<testsuite>
        <testcase name="a"><failure message="no"/></testcase>
        <testcase name="a"><skipped/></testcase>
        <testcase name="b" status="notrun"/>
        <testcase name="b"><error message="no"/></testcase>
        <testcase name="b" status="notrun"/>
    </testsuite>)");
    REQUIRE(tests.size() == 2);
    CHECK(tests.front().outcome == Outcome::kFailed);
    CHECK(tests.front().category == FailureCategory::kAssertion);
    CHECK(tests.back().category == FailureCategory::kUnexpectedException);
    CHECK_FALSE(tests.front().flaky);
}

TEST_CASE("Durations that make no sense count as zero",
          "[progmasoft][runner][import]")
{
    CHECK(Runner::ImportedMicroseconds(1.5) == 1500000);
    CHECK(Runner::ImportedMicroseconds(-1.0) == 0);
    CHECK(Runner::ImportedMicroseconds(1e30) == 0);
    CHECK(Runner::ImportedMicroseconds(std::numeric_limits<double>::quiet_NaN())
          == 0);
    CHECK(Runner::ImportedAttempt(Outcome::kPassed, -5).durationMicroseconds
          == 0);
}

TEST_CASE("Importing goes through the history and the reports of a run",
          "[progmasoft][runner][import]")
{
    const Scratch scratch;
    const auto write = [&](const std::string &name, std::string_view text) {
        const auto path = scratch.Path() / name;
        REQUIRE(Runner::WriteFileAtomically(path, text));
        return path.string();
    };
    const auto report
        = write("failing.xml",
                "<testsuite><testcase classname=\"c\" name=\"t\">"
                "<failure message=\"no\"/></testcase></testsuite>");
    const auto retry = write("passing.tap", "1..1\nok 1 - c::x\n");
    const std::string history = "--history=" + (scratch.Path() / "h").string();

    const auto first = RunFixture(scratch,
                                  { "--import=" + report,
                                    "--import=" + retry,
                                    "--import-name=web",
                                    "--import-revision=one",
                                    history });
    INFO(first.output);
    CHECK(ExitCode(first) == 42);
    CHECK_THAT(first.output, ContainsSubstring("imported from 2 report(s)"));
    CHECK(first.report["binary"].AsString() == "web");
    CHECK(first.report["tests"].Items().size() == 2);
    CHECK(first.Test("t")["replayCommand"].AsString().empty());

    // The same test passes on the same revision: that is flakiness.
    const auto second
        = RunFixture(scratch,
                     { "--import="
                           + write("fixed.xml",
                                   "<testsuite><testcase classname=\"c\" "
                                   "name=\"t\"/></testsuite>"),
                       "--import-name=web",
                       "--import-revision=one",
                       history });
    INFO(second.output);
    CHECK(ExitCode(second) == 0);

    const auto third = RunFixture(scratch,
                                  { "--import=" + report,
                                    "--import-name=web",
                                    "--import-revision=one",
                                    history });
    INFO(third.output);
    CHECK_THAT(third.Analysis("t"), ContainsSubstring("intermittent"));
}

TEST_CASE("Verdicts count as flaky only on a named revision",
          "[progmasoft][runner][import]")
{
    const Scratch scratch;
    const auto failing = scratch.Path() / "failing.xml";
    const auto passing = scratch.Path() / "passing.xml";
    REQUIRE(Runner::WriteFileAtomically(
        failing,
        "<testsuite><testcase name=\"t\"><failure/></testcase></testsuite>"));
    REQUIRE(Runner::WriteFileAtomically(
        passing,
        "<testsuite><testcase name=\"t\"/></testsuite>"));
    const std::string history = "--history=" + (scratch.Path() / "h").string();
    for (const auto &report : { failing, passing })
    {
        const auto run
            = RunFixture(scratch, { "--import=" + report.string(), history });
        INFO(run.output);
    }
    const auto last
        = RunFixture(scratch, { "--import=" + failing.string(), history });
    INFO(last.output);
    CHECK_THAT(last.Analysis("t"), !ContainsSubstring("intermittent"));
}

TEST_CASE("Two TAP streams with the same numbers stay apart",
          "[progmasoft][runner][import]")
{
    const Scratch scratch;
    const auto failing = scratch.Path() / "failing.tap";
    const auto passing = scratch.Path() / "passing.tap";
    REQUIRE(Runner::WriteFileAtomically(failing, "1..1\nnot ok 1\n"));
    REQUIRE(Runner::WriteFileAtomically(passing, "1..1\nok 1\n"));
    const auto run = RunFixture(scratch,
                                { "--import=" + failing.string(),
                                  "--import=" + passing.string(),
                                  "--no-history" });
    INFO(run.output);
    CHECK(ExitCode(run) == 42);
    CHECK(run.report["tests"].Items().size() == 2);
}

TEST_CASE("A run of tests keeps its own history beside an import name",
          "[progmasoft][runner][import]")
{
    const Scratch scratch;
    const auto directory = scratch.Path() / "h";
    const auto run = RunFixture(
        scratch,
        { "--history=" + directory.string(), "--import-name=web", "[pass]" });
    INFO(run.output);
    CHECK(ExitCode(run) == 0);
    CHECK_FALSE(std::filesystem::exists(directory / "web.history.json"));
}

TEST_CASE("Importing a file that is not a report fails",
          "[progmasoft][runner][import]")
{
    const Scratch scratch;
    const auto path = scratch.Path() / "notes.txt";
    REQUIRE(Runner::WriteFileAtomically(path, "hello"));
    const auto run
        = RunFixture(scratch, { "--import=" + path.string(), "--no-history" });
    CHECK_FALSE(ExitCode(run) == 0);
    CHECK_THAT(run.output, ContainsSubstring("notes.txt"));

    const auto missing
        = RunFixture(scratch,
                     { "--import=" + (scratch.Path() / "none.xml").string(),
                       "--no-history" });
    CHECK_FALSE(ExitCode(missing) == 0);
}
