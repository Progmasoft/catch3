// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <Progmasoft/Runner/Analysis.hpp>
#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/Git.hpp>
#include <Progmasoft/Runner/Hash.hpp>
#include <Progmasoft/Runner/History.hpp>
#include <Progmasoft/Runner/Json.hpp>
#include <Progmasoft/Runner/Options.hpp>
#include <Progmasoft/Runner/Planner.hpp>
#include <Progmasoft/Runner/ResultsJson.hpp>

#include <Progmasoft/Catch3.hpp>

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>

#include <chrono>
#include <filesystem>
#include <set>
#include <thread>
#include <vector>

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using namespace Progmasoft::Catch3;
    using Catch::Matchers::ContainsSubstring;

    Runner::Options
    Parse(std::vector<std::string> arguments)
    {
        auto options = Runner::ParseOptions(arguments);
        INFO((options ? std::string{} : options.GetError().Message()));
        REQUIRE(options);
        return *options;
    }

    std::string
    ParseError(std::vector<std::string> arguments)
    {
        auto options = Runner::ParseOptions(arguments);
        REQUIRE_FALSE(options);
        return options.GetError().Message();
    }

    Runner::TestRecord
    Record(std::int64_t run, Outcome outcome, bool flaky = false)
    {
        Runner::TestRecord record;
        record.run = run;
        record.outcome = outcome;
        record.flaky = flaky;
        record.durationMicroseconds = 1000 * run;
        record.fingerprint = "same";
        return record;
    }
} // namespace

TEST_CASE("JSON reads back what it writes", "[progmasoft][runner][json]")
{
    Runner::JsonValue value = Runner::JsonValue::Object();
    value.Set("text", "quote \" backslash \\ newline \n tab \t control \x01");
    value.Set("number", 4294967295U);
    value.Set("negative", -12);
    value.Set("fraction", 0.25);
    value.Set("flag", true);
    value.Set("nothing", nullptr);
    Runner::JsonValue list = Runner::JsonValue::Array();
    list.Append("ü and 日本");
    list.Append(Runner::JsonValue::Object());
    value.Set("list", list);

    for (const std::string &text : {value.ToString(), value.ToPrettyString()})
    {
        auto parsed = Runner::ParseJson(text);
        REQUIRE(parsed);
        CHECK((*parsed)["text"].AsString() == value["text"].AsString());
        CHECK((*parsed)["number"].AsUnsigned() == 4294967295U);
        CHECK((*parsed)["negative"].AsInteger() == -12);
        CHECK((*parsed)["fraction"].AsNumber() == 0.25);
        CHECK((*parsed)["flag"].AsBoolean());
        CHECK((*parsed)["nothing"].IsNull());
        CHECK((*parsed)["list"].Items().front().AsString() == "ü and 日本");
    }
    CHECK(value.ToString().find('\n') == std::string::npos);

    auto escaped = Runner::ParseJson(R"("\u00fc\ud83d\ude00")");
    REQUIRE(escaped);
    CHECK(escaped->AsString() == "\xC3\xBC\xF0\x9F\x98\x80");
}

TEST_CASE("JSON refuses what is not JSON", "[progmasoft][runner][json]")
{
    for (const char *text : {"", "{", "[1,]", "{\"a\" 1}", "tru", "1 2",
                             "\"unterminated", "\"\\x\"", "01e"})
    {
        CAPTURE(text);
        CHECK_FALSE(Runner::ParseJson(text));
    }
    std::string deep(1000, '[');
    deep += std::string(1000, ']');
    CHECK_FALSE(Runner::ParseJson(deep));
}

TEST_CASE("Any string survives a trip through JSON", "[progmasoft][runner][json]")
{
    const auto result = ForAll(Gen::Strings(Gen::Integers<char>(),
                                            SizeRange{0, 64}),
                               [](const std::string &text) {
                                   Runner::JsonValue value{text};
                                   auto parsed =
                                       Runner::ParseJson(value.ToString());
                                   return parsed && parsed->AsString() == text;
                               });
    INFO(result.Describe());
    CHECK(result.Status == PropertyStatus::kPassed);
}

TEST_CASE("The command line is typed and checked", "[progmasoft][runner][options]")
{
    const auto options = Parse({"--jobs=8",
                                "-j", "3",
                                "--timeout=1.5",
                                "--retries=4",
                                "--shard=2/5",
                                "--order=declared",
                                "--seed=17",
                                "--select=smart",
                                "-c", "outer",
                                "--path-filter=g:2",
                                "[fast]",
                                "~[slow]",
                                "--",
                                "--looks-like-an-option"});
    CHECK(options.jobs == 3);
    CHECK(options.timeout == std::chrono::milliseconds{1500});
    CHECK(options.timeoutGiven);
    CHECK(options.retryMode == Runner::RetryMode::kAlways);
    CHECK(options.retries == 4);
    CHECK(options.shard.index == 2);
    CHECK(options.shard.count == 5);
    CHECK(options.order == Runner::Order::kDeclared);
    CHECK(options.seed == 17);
    CHECK(options.seedGiven);
    CHECK(options.selection == Runner::Selection::kSmart);
    CHECK(options.sections == std::vector<std::string>{"outer"});
    CHECK(options.paths == std::vector<std::string>{"g:2"});
    CHECK(options.testSpecs ==
          std::vector<std::string>{"[fast]", "~[slow]", "--looks-like-an-option"});

    CHECK(Parse({"--fail-fast"}).failFast == 1);
    CHECK(Parse({"--fail-fast=3"}).failFast == 3);
    CHECK(Parse({"--list=json"}).mode == Runner::Mode::kListTests);
    CHECK(Parse({"--retries=off"}).retryMode == Runner::RetryMode::kOff);
}

TEST_CASE("Command line mistakes are explained", "[progmasoft][runner][options]")
{
    CHECK_THAT(ParseError({"--jbos=2"}), ContainsSubstring("did you mean --jobs?"));
    CHECK_THAT(ParseError({"--jobs"}), ContainsSubstring("needs a value"));
    CHECK_THAT(ParseError({"--jobs=many"}), ContainsSubstring("whole number"));
    CHECK_THAT(ParseError({"--shard=4/4"}), ContainsSubstring("INDEX below"));
    CHECK_THAT(ParseError({"--order=sideways"}),
               ContainsSubstring("priority, declared, name, random"));
    CHECK_THAT(ParseError({"--success=yes"}), ContainsSubstring("takes no value"));
    CHECK_THAT(ParseError({"--path-filter=x:1"}), ContainsSubstring("c:SECTION"));
}

TEST_CASE("A worker is started with what changes how tests behave",
          "[progmasoft][runner][options]")
{
    auto options = Parse({"--success", "--seed=99", "--section=s", "--warn=no-assertions"});
    const auto arguments = Runner::WorkerArguments(options);
    auto worker = Runner::ParseOptions(arguments);
    REQUIRE(worker);
    CHECK(worker->mode == Runner::Mode::kWorker);
    CHECK(worker->includeSuccessful);
    CHECK(worker->seed == 99);
    CHECK(worker->sections == std::vector<std::string>{"s"});
    CHECK(worker->warnNoAssertions);
    CHECK(worker->benchmarkConfidenceInterval == options.benchmarkConfidenceInterval);
}

TEST_CASE("SHA-256 matches the published test vectors", "[progmasoft][runner][hash]")
{
    CHECK(Runner::Sha256Hex("") ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(Runner::Sha256Hex("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(Runner::Sha256Hex(
              "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(Runner::Sha256Hex(std::string(1000000, 'a')) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST_CASE("History predicts failures and finds flakiness", "[progmasoft][runner][history]")
{
    Runner::TestHistory fresh;
    CHECK(fresh.FailureProbability() == 0.3);
    CHECK_FALSE(fresh.DurationPercentile(0.5).has_value());

    Runner::TestHistory steady;
    for (int run = 1; run <= 10; ++run)
    {
        steady.records.push_back(Record(run, Outcome::kPassed));
    }
    CHECK(steady.FailureProbability() < 0.1);
    CHECK(steady.PassStreak() == 10);
    CHECK(steady.FlakyEvents(20) == 0);
    CHECK(steady.DurationPercentile(0.5) == 5000);
    CHECK(steady.DurationPercentile(1.0) == 10000);

    Runner::TestHistory broken = steady;
    broken.records.push_back(Record(11, Outcome::kFailed));
    CHECK(broken.FailureProbability() >= 0.7);
    CHECK(broken.PassStreak() == 0);
    // Pass, fail on the same fingerprint: the verdict flipped by itself.
    CHECK(broken.FlakyEvents(20) == 1);

    Runner::TestHistory flaky;
    flaky.records.push_back(Record(1, Outcome::kPassed, true));
    flaky.records.push_back(Record(2, Outcome::kPassed, true));
    CHECK(flaky.FlakyEvents(20) == 2);
}

TEST_CASE("History quarantines flaky tests and releases them", "[progmasoft][runner][history]")
{
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error) /
                           ("catch3-history-" + Runner::UniqueName());
    const auto file = Runner::HistoryFile(directory, "binary name");
    CHECK(file.filename() == "binary_name.history.json");

    const auto runWith = [&](bool flakyNow) {
        RunResult run;
        TestResult test;
        test.id = "Suite::test";
        test.name = "test";
        test.outcome = Outcome::kPassed;
        test.flaky = flakyNow;
        Attempt attempt;
        attempt.outcome = Outcome::kPassed;
        test.attempts.push_back(attempt);
        run.tests.push_back(test);
        Runner::QuarantinePolicy policy;
        policy.passesToLeave = 3;
        auto history = Runner::History::Commit(file, run, {}, {}, policy);
        REQUIRE(history);
        return *history;
    };
    CHECK_FALSE(runWith(true).Find("Suite::test")->quarantine);
    CHECK(runWith(true).Find("Suite::test")->quarantine);
    CHECK(runWith(false).Find("Suite::test")->quarantine);
    CHECK(runWith(false).Find("Suite::test")->quarantine);
    const auto released = runWith(false);
    CHECK_FALSE(released.Find("Suite::test")->quarantine);
    CHECK(released.Runs().size() == 5);
    CHECK(released.Runs().back().id == 5);

    auto reloaded = Runner::History::Load(file);
    REQUIRE(reloaded);
    CHECK(reloaded->Find("Suite::test")->records.size() == 5);
    std::filesystem::remove_all(directory, error);
}

TEST_CASE("A history held by another run is left alone",
          "[progmasoft][runner][history]")
{
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error) /
                           ("catch3-history-" + Runner::UniqueName());
    const auto file = Runner::HistoryFile(directory, "locked");
    RunResult run;
    TestResult test;
    test.id = "test";
    test.outcome = Outcome::kPassed;
    run.tests.push_back(test);
    {
        const Runner::FileLock held{file, std::chrono::seconds(1)};
        REQUIRE(held.Held());
        auto blocked = Runner::History::Commit(
            file, run, {}, {}, {}, std::chrono::milliseconds(50));
        REQUIRE_FALSE(blocked);
        CHECK_THAT(blocked.GetError().Message(),
                   ContainsSubstring("holds the lock"));
        CHECK_FALSE(std::filesystem::exists(file));
    }
    CHECK(Runner::History::Commit(file, run, {}, {}, {}));
    std::filesystem::remove_all(directory, error);
}

TEST_CASE("Runs that finish together all reach the history",
          "[progmasoft][runner][history]")
{
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error) /
                           ("catch3-history-" + Runner::UniqueName());
    const auto file = Runner::HistoryFile(directory, "parallel");
    std::vector<std::thread> threads;
    for (int index = 0; index < 8; ++index)
    {
        threads.emplace_back([&, index] {
            RunResult run;
            TestResult test;
            test.id = "test " + std::to_string(index);
            test.outcome = Outcome::kPassed;
            run.tests.push_back(test);
            static_cast<void>(Runner::History::Commit(file, run, {}, {}, {}));
        });
    }
    for (auto &thread : threads)
    {
        thread.join();
    }
    auto history = Runner::History::Load(file);
    REQUIRE(history);
    CHECK(history->Runs().size() == 8);
    CHECK(history->Tests().size() == 8);
    std::filesystem::remove_all(directory, error);
}

TEST_CASE("Stable shards do not depend on the order or the machine",
          "[progmasoft][runner][planner]")
{
    CHECK(Runner::StableShard("anything", 1) == 0);
    std::vector<std::size_t> sizes(4, 0);
    for (int index = 0; index < 4000; ++index)
    {
        const auto shard = Runner::StableShard("test " + std::to_string(index), 4);
        REQUIRE(shard < 4);
        ++sizes[shard];
    }
    for (const auto size : sizes)
    {
        CHECK(size > 800);
    }
    CHECK(Runner::StableShard("parser parses", 7) ==
          Runner::StableShard("parser parses", 7));
}

TEST_CASE("Failure signatures ignore numbers and addresses",
          "[progmasoft][runner][analysis]")
{
    Attempt first;
    first.outcome = Outcome::kFailed;
    first.category = FailureCategory::kAssertion;
    AssertionRecord assertion;
    assertion.file = "pool.cpp";
    assertion.line = 33;
    assertion.expression = "pool.size() == limit";
    assertion.message = "slot 17 at 0x7ffd0010 is taken";
    first.assertions.push_back(assertion);
    Attempt second = first;
    second.assertions.front().message = "slot 4 at 0x51a0 is taken";
    CHECK(Runner::FailureSignature(first) == Runner::FailureSignature(second));
    Attempt elsewhere = first;
    elsewhere.assertions.front().line = 34;
    CHECK(Runner::FailureSignature(first) != Runner::FailureSignature(elsewhere));
    CHECK_THAT(Runner::FailureSignature(first), ContainsSubstring("sig-"));

    CHECK(Runner::QuoteTestSpec("a, [b] *c") == "a\\, \\[b\\] \\*c");
}

TEST_CASE("A run report reads back what it wrote", "[progmasoft][runner][json]")
{
    RunResult run;
    run.binary = "tests";
    run.seed = 7;
    run.shardCount = 3;
    run.notes = {"a note"};
    TestResult test;
    test.id = "Fixture::adds";
    test.name = "adds";
    test.className = "Fixture";
    test.tags = {"math", "!timeout:3"};
    test.outcome = Outcome::kTimedOut;
    test.category = FailureCategory::kTimeout;
    test.history = FailureHistory::kNew;
    test.analysis = {"stopped after 3.0 s"};
    Attempt attempt;
    attempt.outcome = Outcome::kTimedOut;
    attempt.category = FailureCategory::kTimeout;
    attempt.precededBy = {"before"};
    attempt.lastLocation = "math.cpp:9";
    SectionRecord section;
    section.name = "slow";
    section.durationMicroseconds = -1;
    attempt.sections.push_back(section);
    BenchmarkRecord benchmark;
    benchmark.name = "sum";
    benchmark.meanNanoseconds = 12.5;
    attempt.benchmarks.push_back(benchmark);
    test.attempts.push_back(attempt);
    run.tests.push_back(test);

    auto json = Runner::ParseJson(Runner::ToJson(run).ToString());
    REQUIRE(json);
    auto read = Runner::RunResultFromJson(*json);
    REQUIRE(read);
    REQUIRE(read->tests.size() == 1);
    const auto &back = read->tests.front();
    CHECK(back.id == test.id);
    CHECK(back.className == "Fixture");
    CHECK(back.tags == test.tags);
    CHECK(back.outcome == Outcome::kTimedOut);
    CHECK(back.history == FailureHistory::kNew);
    CHECK(back.analysis == test.analysis);
    REQUIRE(back.attempts.size() == 1);
    CHECK(back.attempts.front().precededBy == attempt.precededBy);
    CHECK(back.attempts.front().sections.front().durationMicroseconds == -1);
    CHECK(back.attempts.front().benchmarks.front().meanNanoseconds == 12.5);
    CHECK(read->shardCount == 3);
    CHECK((*json)["summary"]["timedOut"].AsInteger() == 1);

    Runner::JsonValue future = *json;
    future.Set("version", 99);
    CHECK_FALSE(Runner::RunResultFromJson(future));
}

TEST_CASE("Source files match across spellings", "[progmasoft][runner][git]")
{
    CHECK(Runner::SameSourceFile("/work/repo/tests/a.cpp", "tests/a.cpp"));
    CHECK(Runner::SameSourceFile("../tests/a.cpp", "/work/repo/tests/a.cpp"));
    CHECK(Runner::SameSourceFile("/work/repo/./tests/../tests/a.cpp",
                                 "/work/repo/tests/a.cpp"));
    CHECK_FALSE(Runner::SameSourceFile("/work/repo/tests/a.cpp", "ests/a.cpp"));
    CHECK_FALSE(Runner::SameSourceFile("/work/repo/tests/a.cpp",
                                       "/work/repo/tests/b.cpp"));
}

TEST_CASE("A file lock lets one holder in at a time", "[progmasoft][runner][files]")
{
    std::error_code error;
    const auto file = std::filesystem::temp_directory_path(error) /
                      ("catch3-lock-" + Runner::UniqueName());
    {
        const Runner::FileLock first{file, std::chrono::milliseconds{100}};
        CHECK(first.Held());
        const Runner::FileLock second{file, std::chrono::milliseconds{50}};
        CHECK_FALSE(second.Held());
    }
    const Runner::FileLock third{file, std::chrono::milliseconds{100}};
    CHECK(third.Held());

    REQUIRE(Runner::WriteFileAtomically(file, "first"));
    REQUIRE(Runner::WriteFileAtomically(file, "second"));
    CHECK(*Runner::ReadFile(file) == "second");
    std::filesystem::remove(file, error);
}
