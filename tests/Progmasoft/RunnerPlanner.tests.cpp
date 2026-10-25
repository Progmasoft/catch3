// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

// The planner decides which tests run, where, in which order and under
// which limits, from the options, the history and the changed files. These
// tests give it a catalog of their own, so no test of this binary is
// involved.

#include <Progmasoft/Runner/Catalog.hpp>
#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/History.hpp>
#include <Progmasoft/Runner/Planner.hpp>

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using Catch::Matchers::ContainsSubstring;
    using Progmasoft::Catch3::Attempt;
    using Progmasoft::Catch3::Outcome;
    using Progmasoft::Catch3::RunResult;
    using Progmasoft::Catch3::TestResult;
    using std::chrono::milliseconds;

    /// A directory of its own, removed at the end of the test.
    class Scratch final
    {
    public:
        Scratch()
        {
            std::error_code error;
            path_ = std::filesystem::temp_directory_path(error) /
                    ("catch3-planner-" + Runner::UniqueName());
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

    Runner::TestEntry
    Entry(const std::string &name, const std::string &file = "src/a.cpp")
    {
        Runner::TestEntry entry;
        entry.name = name;
        entry.file = file;
        entry.line = 1;
        return entry;
    }

    std::vector<std::size_t>
    All(const Runner::Catalog &catalog)
    {
        std::vector<std::size_t> indices(catalog.Tests().size());
        for (std::size_t index = 0; index < indices.size(); ++index)
        {
            indices[index] = index;
        }
        return indices;
    }

    Runner::Plan
    PlanOf(const Runner::Options &options, const Runner::PlanInputs &inputs)
    {
        auto plan = Runner::MakePlan(options, inputs);
        INFO((plan ? std::string{} : plan.GetError().Message()));
        REQUIRE(plan);
        return *plan;
    }

    std::string
    PlanError(const Runner::Options &options, const Runner::PlanInputs &inputs)
    {
        auto plan = Runner::MakePlan(options, inputs);
        REQUIRE_FALSE(plan);
        return plan.GetError().Message();
    }

    std::vector<std::string>
    Names(const Runner::Plan &plan)
    {
        std::vector<std::string> names;
        for (const auto &test : plan.tests)
        {
            names.push_back(test.entry->name);
        }
        return names;
    }

    const Runner::PlannedTest &
    Planned(const Runner::Plan &plan, const std::string &name)
    {
        const auto found =
            std::find_if(plan.tests.begin(),
                         plan.tests.end(),
                         [&](const Runner::PlannedTest &test)
                         { return test.entry->name == name; });
        REQUIRE(found != plan.tests.end());
        return *found;
    }

    /// One test's result in a run committed to the history.
    struct Result final
    {
        std::string name;
        Outcome outcome = Outcome::kPassed;
        std::int64_t microseconds = 1000;
        bool flaky = false;
        std::vector<std::string> precededBy;
        std::uint32_t seed = 0;
    };

    /// Adds one run to the history in `file` and returns the history.
    Runner::History
    Commit(const std::filesystem::path &file,
           const std::vector<Result> &results,
           const std::map<std::string, std::string> &fingerprints = {})
    {
        RunResult run;
        for (const auto &result : results)
        {
            TestResult test;
            test.id = result.name;
            test.name = result.name;
            test.outcome = result.outcome;
            test.flaky = result.flaky;
            Attempt attempt;
            attempt.outcome = result.flaky ? Outcome::kFailed : result.outcome;
            attempt.durationMicroseconds = result.microseconds;
            attempt.precededBy = result.precededBy;
            attempt.seed = result.seed;
            test.attempts.push_back(attempt);
            run.tests.push_back(test);
        }
        auto history =
            Runner::History::Commit(file, run, {}, fingerprints, {});
        INFO((history ? std::string{} : history.GetError().Message()));
        REQUIRE(history);
        return *history;
    }
} // namespace

TEST_CASE("The planner selects tests by their history and by change",
          "[progmasoft][runner][planner]")
{
    const Scratch scratch;
    const auto file = Runner::HistoryFile(scratch.Path(), "selection");
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("steady"),
         Entry("broken"),
         Entry("shaky"),
         Entry("edited", "src/edited.cpp"),
         Entry("unseen")});
    const auto history = Commit(file,
                                {{"steady"},
                                 {"broken", Outcome::kFailed},
                                 {"shaky", Outcome::kPassed, 1000, true},
                                 {"edited"}});
    Runner::Changes changes;
    changes.root = scratch.Path();
    changes.files.push_back(scratch.Path() / "src" / "edited.cpp");

    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    inputs.history = &history;
    inputs.changes = &changes;
    Runner::Options options;
    options.order = Runner::Order::kDeclared;

    SECTION("all")
    {
        CHECK(PlanOf(options, inputs).tests.size() == 5);
    }
    SECTION("failed: failures and flaky passes of the last run")
    {
        options.selection = Runner::Selection::kFailed;
        const auto plan = PlanOf(options, inputs);
        CHECK(Names(plan) == std::vector<std::string>{"broken", "shaky"});
        CHECK(Planned(plan, "broken").selectionReason == "failed last run");
        CHECK(Planned(plan, "shaky").selectionReason == "flaky last run");
    }
    SECTION("new: tests without a result")
    {
        options.selection = Runner::Selection::kNew;
        const auto plan = PlanOf(options, inputs);
        CHECK(Names(plan) == std::vector<std::string>{"unseen"});
        CHECK(Planned(plan, "unseen").selectionReason == "no earlier result");
    }
    SECTION("changed: tests in a changed file")
    {
        options.selection = Runner::Selection::kChanged;
        const auto plan = PlanOf(options, inputs);
        CHECK(Names(plan) == std::vector<std::string>{"edited"});
        CHECK(Planned(plan, "edited").selectionReason ==
              "its source file changed");
    }
    SECTION("smart: everything but the steady test")
    {
        options.selection = Runner::Selection::kSmart;
        const auto plan = PlanOf(options, inputs);
        CHECK(Names(plan) == std::vector<std::string>{
                                 "broken", "shaky", "edited", "unseen"});
    }
    SECTION("changed, when Git did not answer, selects nothing and says so")
    {
        options.selection = Runner::Selection::kChanged;
        inputs.changes = nullptr;
        const auto plan = PlanOf(options, inputs);
        CHECK(plan.tests.empty());
        REQUIRE(plan.notes.size() == 1);
        CHECK_THAT(plan.notes.front(), ContainsSubstring("Git"));
    }
    SECTION("only the matched tests are candidates")
    {
        inputs.matched = {0, 4};
        const auto plan = PlanOf(options, inputs);
        CHECK(plan.candidates == 2);
        CHECK(Names(plan) == std::vector<std::string>{"steady", "unseen"});
    }
}

TEST_CASE("Dependencies come along and run first",
          "[progmasoft][runner][planner]")
{
    auto user = Entry("user");
    user.dependsOn = {"setup"};
    auto setup = Entry("setup");
    setup.dependsOn = {"base"};
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("base"), setup, user, Entry("other")});
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = {2};
    Runner::Options options;
    options.order = Runner::Order::kName;

    const auto plan = PlanOf(options, inputs);
    CHECK(Names(plan) == std::vector<std::string>{"base", "setup", "user"});
    CHECK(Planned(plan, "setup").selectionReason == "needed by 'user'");
    CHECK(Planned(plan, "base").selectionReason == "needed by 'setup'");
    // Dependencies are positions in the plan, wherever the order put them.
    REQUIRE(Planned(plan, "user").dependencies.size() == 1);
    CHECK(plan.tests[Planned(plan, "user").dependencies.front()]
              .entry->name == "setup");
    CHECK(Planned(plan, "base").dependencies.empty());
}

TEST_CASE("Broken dependencies stop the plan with an explanation",
          "[progmasoft][runner][planner]")
{
    Runner::Options options;
    Runner::PlanInputs inputs;

    SECTION("a dependency that is not a test")
    {
        auto lonely = Entry("lonely");
        lonely.dependsOn = {"nowhere"};
        const auto catalog = Runner::Catalog::FromEntries({lonely});
        inputs.catalog = &catalog;
        inputs.matched = All(catalog);
        CHECK_THAT(PlanError(options, inputs),
                   ContainsSubstring("'lonely' depends on 'nowhere'"));
    }
    SECTION("a cycle")
    {
        auto first = Entry("first");
        first.dependsOn = {"second"};
        auto second = Entry("second");
        second.dependsOn = {"third"};
        auto third = Entry("third");
        third.dependsOn = {"first"};
        const auto catalog =
            Runner::Catalog::FromEntries({first, second, third});
        inputs.catalog = &catalog;
        inputs.matched = {0};
        const auto message = PlanError(options, inputs);
        CHECK_THAT(message, ContainsSubstring("cycle"));
        CHECK_THAT(message,
                   ContainsSubstring("'first' -> 'second' -> 'third' -> "
                                     "'first'"));
    }
}

TEST_CASE("Hash shards cover every test once and keep dependencies together",
          "[progmasoft][runner][planner]")
{
    std::vector<Runner::TestEntry> entries;
    for (int index = 0; index < 40; ++index)
    {
        entries.push_back(Entry("test " + std::to_string(index)));
    }
    // A chain of three tests must land in one shard.
    entries[10].dependsOn = {"test 20"};
    entries[20].dependsOn = {"test 30"};
    const auto catalog = Runner::Catalog::FromEntries(entries);
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    Runner::Options options;
    options.shard.count = 3;

    std::multiset<std::string> seen;
    std::set<unsigned> shardsOfChain;
    for (unsigned shard = 0; shard < 3; ++shard)
    {
        options.shard.index = shard;
        const auto plan = PlanOf(options, inputs);
        CHECK_FALSE(plan.tests.empty());
        for (const auto &name : Names(plan))
        {
            seen.insert(name);
            if (name == "test 10" || name == "test 20" || name == "test 30")
            {
                shardsOfChain.insert(shard);
            }
        }
    }
    CHECK(seen.size() == 40);
    CHECK(std::set<std::string>(seen.begin(), seen.end()).size() == 40);
    CHECK(shardsOfChain.size() == 1);
}

TEST_CASE("Duration shards balance the recorded time",
          "[progmasoft][runner][planner]")
{
    const Scratch scratch;
    const auto report = scratch.Path() / "report.json";
    std::ofstream(report)
        << R"({"format": "catch3-run", "version": 1, "tests": [)"
        << R"({"id": "long", "durationMicroseconds": 9000000},)"
        << R"({"id": "medium", "durationMicroseconds": 5000000},)"
        << R"({"id": "short", "durationMicroseconds": 4000000}]})";
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("short"), Entry("medium"), Entry("long")});
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    Runner::Options options;
    options.order = Runner::Order::kName;
    options.shard.count = 2;
    options.shardDurations = report.string();

    // Longest first into the emptier shard: 9 s alone, 5 s + 4 s together.
    options.shard.index = 0;
    CHECK(Names(PlanOf(options, inputs)) == std::vector<std::string>{"long"});
    options.shard.index = 1;
    CHECK(Names(PlanOf(options, inputs)) ==
          std::vector<std::string>{"medium", "short"});

    SECTION("a file that is no report is an error")
    {
        std::ofstream(report, std::ios::trunc) << R"({"format": "other"})";
        CHECK_THAT(PlanError(options, inputs),
                   ContainsSubstring("neither a Catch3 history"));
    }
}

TEST_CASE("An incremental run keeps passes with the same inputs",
          "[progmasoft][runner][planner]")
{
    const Scratch scratch;
    const auto file = Runner::HistoryFile(scratch.Path(), "incremental");
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("passes"), Entry("fails"), Entry("flakes")});
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    inputs.binaryFingerprint = "build 1";
    Runner::Options options;
    options.order = Runner::Order::kDeclared;

    const auto first = PlanOf(options, inputs);
    CHECK(first.fingerprints.size() == 3);
    const auto history =
        Commit(file,
               {{"passes"},
                {"fails", Outcome::kFailed},
                {"flakes", Outcome::kPassed, 1000, true}},
               first.fingerprints);
    inputs.history = &history;
    options.incremental = true;

    const auto again = PlanOf(options, inputs);
    CHECK(Names(again) == std::vector<std::string>{"fails", "flakes"});
    REQUIRE(again.kept.size() == 1);
    CHECK(again.kept.front().name == "passes");
    CHECK(again.kept.front().outcome == Outcome::kUpToDate);

    // Another build changes every fingerprint.
    inputs.binaryFingerprint = "build 2";
    CHECK(PlanOf(options, inputs).tests.size() == 3);
}

TEST_CASE("Time limits are learned from the durations",
          "[progmasoft][runner][planner]")
{
    const Scratch scratch;
    const auto file = Runner::HistoryFile(scratch.Path(), "timeouts");
    auto tagged = Entry("tagged");
    tagged.timeout = milliseconds{1500};
    auto benchmark = Entry("benchmark");
    benchmark.benchmark = true;
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("quick"), Entry("slow"), tagged, benchmark, Entry("unseen")});
    Runner::History history;
    for (int run = 0; run < 5; ++run)
    {
        history = Commit(file,
                         {{"quick", Outcome::kPassed, 100000},
                          {"slow", Outcome::kPassed, 2000000},
                          {"tagged", Outcome::kPassed, 100000},
                          {"benchmark", Outcome::kPassed, 2000000}});
    }
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    inputs.history = &history;
    Runner::Options options;

    auto plan = PlanOf(options, inputs);
    // max(p95 x 4, median x 10) + 2 s, at least 5 s.
    CHECK(Planned(plan, "quick").timeout == milliseconds{5000});
    CHECK(Planned(plan, "quick").timeoutLearned);
    CHECK(Planned(plan, "slow").timeout == milliseconds{22000});
    CHECK(Planned(plan, "slow").predictedMicroseconds == 2000000);
    // A tag wins; benchmarks and unknown tests get the global limit.
    CHECK(Planned(plan, "tagged").timeout == milliseconds{1500});
    CHECK_FALSE(Planned(plan, "tagged").timeoutLearned);
    CHECK(Planned(plan, "benchmark").timeout == milliseconds{300000});
    CHECK(Planned(plan, "unseen").timeout == milliseconds{300000});
    CHECK(Planned(plan, "unseen").predictedMicroseconds == 500000);

    SECTION("--timeout caps what was learned")
    {
        options.timeout = milliseconds{10000};
        options.timeoutGiven = true;
        plan = PlanOf(options, inputs);
        CHECK(Planned(plan, "slow").timeout == milliseconds{10000});
        CHECK(Planned(plan, "unseen").timeout == milliseconds{10000});
    }
    SECTION("--no-adaptive-timeout uses the global limit")
    {
        options.adaptiveTimeout = false;
        plan = PlanOf(options, inputs);
        CHECK(Planned(plan, "quick").timeout == milliseconds{300000});
        CHECK_FALSE(Planned(plan, "quick").timeoutLearned);
    }
}

TEST_CASE("Likely failures go first, then the longest tests",
          "[progmasoft][runner][planner]")
{
    const Scratch scratch;
    const auto file = Runner::HistoryFile(scratch.Path(), "priority");
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("long pass"),
         Entry("short pass"),
         Entry("slow failure"),
         Entry("quick failure"),
         Entry("unseen")});
    const auto history = Commit(file,
                                {{"long pass", Outcome::kPassed, 3000000},
                                 {"short pass", Outcome::kPassed, 10000},
                                 {"slow failure", Outcome::kFailed, 2000000},
                                 {"quick failure", Outcome::kFailed, 10000}});
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    inputs.history = &history;
    Runner::Options options;

    SECTION("priority")
    {
        const auto plan = PlanOf(options, inputs);
        CHECK(Planned(plan, "quick failure").failureProbability >= 0.5);
        CHECK(Names(plan) == std::vector<std::string>{"quick failure",
                                                      "slow failure",
                                                      "long pass",
                                                      "unseen",
                                                      "short pass"});
    }
    SECTION("declared")
    {
        options.order = Runner::Order::kDeclared;
        CHECK(Names(PlanOf(options, inputs)) ==
              std::vector<std::string>{"long pass",
                                       "short pass",
                                       "slow failure",
                                       "quick failure",
                                       "unseen"});
    }
    SECTION("random, the same for the same seed")
    {
        options.order = Runner::Order::kRandom;
        options.seed = 7;
        const auto once = Names(PlanOf(options, inputs));
        CHECK(Names(PlanOf(options, inputs)) == once);
        auto sorted = once;
        std::sort(sorted.begin(), sorted.end());
        CHECK(sorted.size() == 5);
    }
}

TEST_CASE("Retries and quarantine come from tags, options and history",
          "[progmasoft][runner][planner]")
{
    auto retried = Entry("retried");
    retried.retries = 5U;
    auto quarantined = Entry("quarantined");
    quarantined.quarantined = true;
    const auto catalog =
        Runner::Catalog::FromEntries({Entry("plain"), retried, quarantined});
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    Runner::Options options;

    auto plan = PlanOf(options, inputs);
    CHECK(Planned(plan, "plain").retryLimit == 2);
    CHECK(Planned(plan, "retried").retryLimit == 5);
    CHECK_FALSE(Planned(plan, "plain").quarantined);
    CHECK(Planned(plan, "quarantined").quarantined);
    CHECK(Planned(plan, "quarantined").quarantineReason ==
          "tagged [!quarantine]");

    options.retryMode = Runner::RetryMode::kOff;
    options.quarantine = Runner::QuarantineMode::kOff;
    plan = PlanOf(options, inputs);
    CHECK(Planned(plan, "plain").retryLimit == 0);
    // A tag still asks for its retries.
    CHECK(Planned(plan, "retried").retryLimit == 5);
    CHECK_FALSE(Planned(plan, "quarantined").quarantined);
}

TEST_CASE("Replay runs the last failures the way they happened",
          "[progmasoft][runner][planner]")
{
    const Scratch scratch;
    const auto file = Runner::HistoryFile(scratch.Path(), "replay");
    const auto catalog = Runner::Catalog::FromEntries(
        {Entry("polluter"), Entry("victim"), Entry("bystander")});
    Runner::PlanInputs inputs;
    inputs.catalog = &catalog;
    inputs.matched = All(catalog);
    Runner::Options options;
    options.replayFailures = true;

    SECTION("with a failure")
    {
        Result victim{"victim", Outcome::kFailed};
        victim.precededBy = {"polluter"};
        victim.seed = 42;
        const auto history =
            Commit(file, {{"polluter"}, victim, {"bystander"}});
        inputs.history = &history;
        const auto plan = PlanOf(options, inputs);
        REQUIRE(Names(plan) == std::vector<std::string>{"victim"});
        const auto &test = plan.tests.front();
        CHECK(test.replaySeed == 42U);
        REQUIRE(test.replaySequence.has_value());
        CHECK(*test.replaySequence == std::vector<std::size_t>{0});
        CHECK_THAT(test.selectionReason, ContainsSubstring("failed in run"));
    }
    SECTION("without one")
    {
        const auto history = Commit(file, {{"polluter"}, {"victim"}});
        inputs.history = &history;
        const auto plan = PlanOf(options, inputs);
        CHECK(plan.tests.empty());
        REQUIRE(plan.notes.size() == 1);
        CHECK_THAT(plan.notes.front(), ContainsSubstring("no failure"));
    }
}
