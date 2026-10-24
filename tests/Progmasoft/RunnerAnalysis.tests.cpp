// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

// What the runner writes down about a failure: how it relates to earlier
// runs and to other failures, where it stopped, what changed and how to
// run it again.

#include <Progmasoft/Runner/Analysis.hpp>
#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/History.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using Catch::Matchers::ContainsSubstring;
    using Progmasoft::Catch3::AssertionRecord;
    using Progmasoft::Catch3::Attempt;
    using Progmasoft::Catch3::FailureCategory;
    using Progmasoft::Catch3::FailureHistory;
    using Progmasoft::Catch3::Outcome;
    using Progmasoft::Catch3::RunResult;
    using Progmasoft::Catch3::SectionRecord;
    using Progmasoft::Catch3::TestResult;

    /// A directory of its own, removed at the end of the test.
    class Scratch final
    {
    public:
        Scratch()
        {
            std::error_code error;
            path_ = std::filesystem::temp_directory_path(error) /
                    ("catch3-analysis-" + Runner::UniqueName());
            std::filesystem::create_directories(path_, error);
        }
        Scratch(const Scratch &) = delete;
        Scratch &
        operator=(const Scratch &) = delete;
        ~Scratch()
        {
            std::error_code error;
            std::filesystem::remove_all(path_, error);
        }

        [[nodiscard]] const std::filesystem::path &
        Path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_;
    };

    /// A test that failed one assertion.
    TestResult
    Failed(const std::string &name,
           const std::string &message = "wrong",
           std::size_t line = 10)
    {
        TestResult test;
        test.id = name;
        test.name = name;
        test.file = "src/test.cpp";
        test.line = 1;
        test.outcome = Outcome::kFailed;
        test.category = FailureCategory::kAssertion;
        Attempt attempt;
        attempt.outcome = Outcome::kFailed;
        attempt.category = FailureCategory::kAssertion;
        attempt.seed = 7;
        AssertionRecord assertion;
        assertion.file = "src/test.cpp";
        assertion.line = line;
        assertion.expression = "a == b";
        assertion.message = message;
        attempt.assertions.push_back(assertion);
        test.attempts.push_back(attempt);
        return test;
    }

    TestResult
    Passed(const std::string &name)
    {
        TestResult test;
        test.id = name;
        test.name = name;
        test.outcome = Outcome::kPassed;
        Attempt attempt;
        attempt.outcome = Outcome::kPassed;
        test.attempts.push_back(attempt);
        return test;
    }

    /// Commits runs of `tests` to a history, the last commit last.
    Runner::History
    HistoryOf(const std::filesystem::path &file,
              const std::vector<std::vector<TestResult>> &runs)
    {
        Runner::History history;
        int number = 0;
        for (const auto &tests : runs)
        {
            RunResult run;
            run.tests = tests;
            for (auto &test : run.tests)
            {
                // As the runner does before it commits a run.
                for (const auto &attempt : test.attempts)
                {
                    if (Progmasoft::Catch3::IsFailure(attempt.outcome))
                    {
                        test.signature = Runner::FailureSignature(attempt);
                        break;
                    }
                }
            }
            Runner::RunRecord record;
            record.commit = "commit" + std::to_string(++number) + "0123456789";
            auto committed =
                Runner::History::Commit(file, run, record, {}, {});
            REQUIRE(committed);
            history = *committed;
        }
        return history;
    }

    std::string
    Findings(const TestResult &test)
    {
        std::string text;
        for (const auto &finding : test.analysis)
        {
            text += finding + "\n";
        }
        return text;
    }
} // namespace

TEST_CASE("A failure is placed in the test's history",
          "[progmasoft][runner][analysis]")
{
    const Scratch scratch;
    const auto file = Runner::HistoryFile(scratch.Path(), "classes");
    auto flaky = Passed("flaky");
    flaky.flaky = true;
    flaky.attempts.front().outcome = Outcome::kFailed;
    const auto history = HistoryOf(
        file,
        {{Passed("fresh"), Failed("stuck", "first"), Passed("wobbly")},
         {Passed("fresh"), Failed("stuck", "first"), flaky}});

    RunResult run;
    run.tests = {Failed("unknown"),
                 Failed("fresh", "one"),
                 Failed("stuck", "second", 20),
                 Failed("flaky", "three", 30)};
    Runner::AnalysisInputs inputs;
    inputs.history = &history;
    inputs.program = "tests";
    Runner::Analyze(run, inputs);

    CHECK(run.tests[0].history == FailureHistory::kUnknown);

    CHECK(run.tests[1].history == FailureHistory::kNew);
    CHECK_THAT(Findings(run.tests[1]),
               ContainsSubstring("new failure after 2 passing run(s); it "
                                 "last passed at commit commit201234"));

    CHECK(run.tests[2].history == FailureHistory::kPersistent);
    CHECK_THAT(Findings(run.tests[2]),
               ContainsSubstring("persistent: failed in the last 2 run(s), "
                                 "but differently last time"));

    CHECK(run.tests[3].history == FailureHistory::kIntermittent);
    CHECK_THAT(Findings(run.tests[3]),
               ContainsSubstring("intermittent: 1 flaky result(s)"));
}

TEST_CASE("Failures with one signature are named together",
          "[progmasoft][runner][analysis]")
{
    RunResult run;
    for (const char *name : {"a", "b", "c", "d", "e"})
    {
        // Numbers in the message do not split the group.
        run.tests.push_back(Failed(
            name,
            "connection refused after " +
                std::to_string((run.tests.size() + 1) * 100) + " ms"));
    }
    run.tests.push_back(Failed("elsewhere", "other", 99));
    Runner::AnalysisInputs inputs;
    inputs.program = "tests";
    Runner::Analyze(run, inputs);

    CHECK(run.tests[0].signature == run.tests[4].signature);
    CHECK(run.tests[0].signature != run.tests[5].signature);
    CHECK_THAT(Findings(run.tests[0]),
               ContainsSubstring("fails the same way as 'b', 'c', 'd' and 1 "
                                 "more: probably one cause"));
    CHECK_THAT(Findings(run.tests[5]),
               !ContainsSubstring("fails the same way"));
}

TEST_CASE("A crash and a hang say where they stopped",
          "[progmasoft][runner][analysis]")
{
    SectionRecord outer;
    outer.name = "outer";
    outer.depth = 0;
    outer.durationMicroseconds = -1;
    SectionRecord done;
    done.name = "done";
    done.depth = 1;
    done.durationMicroseconds = 5;
    SectionRecord inner;
    inner.name = "inner";
    inner.depth = 1;
    inner.durationMicroseconds = -1;

    TestResult crash = Failed("crash");
    crash.outcome = Outcome::kCrashed;
    Attempt &crashed = crash.attempts.front();
    crashed.outcome = Outcome::kCrashed;
    crashed.category = FailureCategory::kCrash;
    crashed.assertions.clear();
    crashed.exitDescription = "signal 11 (SIGSEGV)";
    crashed.lastLocation = "src/test.cpp:44";
    crashed.sections = {outer, done, inner};

    TestResult hang = Failed("hang");
    hang.outcome = Outcome::kTimedOut;
    hang.predictedDurationMicroseconds = 200000;
    Attempt &hung = hang.attempts.front();
    hung.outcome = Outcome::kTimedOut;
    hung.category = FailureCategory::kTimeout;
    hung.assertions.clear();
    hung.durationMicroseconds = 5000000;
    hung.lastLocation = "src/test.cpp:50";
    hung.sections = {outer};

    RunResult run;
    run.tests = {crash, hang};
    Runner::AnalysisInputs inputs;
    inputs.program = "tests";
    Runner::Analyze(run, inputs);

    CHECK_THAT(Findings(run.tests[0]),
               ContainsSubstring("crashed (signal 11 (SIGSEGV)) after the "
                                 "assertion at src/test.cpp:44, in section "
                                 "outer / inner"));
    CHECK_THAT(Findings(run.tests[1]),
               ContainsSubstring("stopped after 5.0 s"));
    CHECK_THAT(Findings(run.tests[1]),
               ContainsSubstring("it was in section outer"));
    CHECK_THAT(Findings(run.tests[1]),
               ContainsSubstring("last assertion started at src/test.cpp:50"));
    // Two crashes in different places are different failures.
    CHECK(run.tests[0].signature != run.tests[1].signature);
}

TEST_CASE("A failure names the changed files it touches",
          "[progmasoft][runner][analysis]")
{
    const Scratch scratch;
    Runner::Changes changes;
    changes.root = scratch.Path();
    changes.files = {scratch.Path() / "src" / "test.cpp",
                     scratch.Path() / "src" / "helper.hpp"};
    TestResult test = Failed("touched");
    AssertionRecord inHelper;
    inHelper.file = "src/helper.hpp";
    inHelper.line = 3;
    inHelper.passed = false;
    test.attempts.front().assertions.push_back(inHelper);
    test.attempts.front().temporaryDirectory = "/tmp/kept";

    RunResult run;
    run.tests = {test};
    Runner::AnalysisInputs inputs;
    inputs.changes = &changes;
    inputs.changesSince = "main";
    inputs.program = "tests";
    Runner::Analyze(run, inputs);

    const auto findings = Findings(run.tests[0]);
    CHECK_THAT(findings, ContainsSubstring("changed since main: src/test.cpp"));
    CHECK_THAT(findings, ContainsSubstring("changed since main: src/helper.hpp"));
    CHECK_THAT(findings,
               ContainsSubstring("its temporary files are kept in /tmp/kept"));
}

TEST_CASE("Every failure gets a command that runs it again",
          "[progmasoft][runner][analysis]")
{
    RunResult run;
    run.tests = {Failed("plain, with [specials]"), Failed("after another"),
                 Passed("fine")};
    run.tests[1].attempts.front().precededBy = {"polluter"};
    Runner::AnalysisInputs inputs;
    inputs.program = "build dir/tests";
    Runner::Analyze(run, inputs);

    CHECK_THAT(run.tests[0].replayCommand,
               ContainsSubstring("plain\\\\, with \\\\[specials\\\\]"));
    CHECK_THAT(run.tests[0].replayCommand, ContainsSubstring("--seed=7"));
    CHECK_THAT(run.tests[0].replayCommand, ContainsSubstring("build dir/tests"));
    // A failure that needed other tests first is replayed with them.
    CHECK_THAT(run.tests[1].replayCommand, ContainsSubstring("--replay"));
    CHECK(run.tests[2].replayCommand.empty());
    CHECK(run.tests[2].analysis.empty());
}
