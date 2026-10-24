// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

// What the runner prints and returns: its modes, the console report, the
// exit codes and Bazel's test environment.

#include <Progmasoft/Catch3/Version.hpp>
#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/Json.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "RunnerFixtureClient.hpp"

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using Catch::Matchers::ContainsSubstring;
    using FixtureClient::ExitCode;
    using FixtureClient::RunFixture;
    using FixtureClient::RunPlain;
    using FixtureClient::Scratch;

    std::int64_t
    CodeOf(const Runner::CapturedRun &run)
    {
        INFO(run.output << run.errors);
        REQUIRE(run.status.kind == Runner::ExitStatus::Kind::kExited);
        return run.status.code;
    }
} // namespace

TEST_CASE("The runner tells its version and how to use it",
          "[progmasoft][runner][output]")
{
    const auto version = RunPlain({ "--version" });
    CHECK(CodeOf(version) == 0);
    CHECK_THAT(version.output,
               ContainsSubstring(std::string{ "Catch3 " }
                                 + Progmasoft::Catch3::kVersion));

    const auto help = RunPlain({ "--help" });
    CHECK(CodeOf(help) == 0);
    CHECK_THAT(help.output, ContainsSubstring("usage: "));
    CHECK_THAT(help.output, ContainsSubstring("--shard=INDEX/COUNT"));
}

TEST_CASE("Tests and tags are listed", "[progmasoft][runner][output]")
{
    const auto text = RunPlain({ "--list", "[deps]" });
    CHECK(CodeOf(text) == 0);
    CHECK_THAT(text.output, ContainsSubstring("fixture dependent"));
    CHECK_THAT(text.output, ContainsSubstring("RunnerFixture.cpp:"));
    CHECK_THAT(text.output, ContainsSubstring("[!depends:fixture base]"));
    CHECK_THAT(text.output, ContainsSubstring("2 matching test(s)"));

    const auto names = RunPlain({ "--list=names", "[deps]" });
    CHECK_THAT(names.output, ContainsSubstring("fixture base"));
    CHECK(names.output.find("matching") == std::string::npos);

    const auto tags = RunPlain({ "--list-tags" });
    CHECK(CodeOf(tags) == 0);
    CHECK_THAT(tags.output, ContainsSubstring("[fixture]"));
    CHECK_THAT(tags.output, ContainsSubstring("tag(s)"));
}

TEST_CASE("Mistakes on the command line stop the run",
          "[progmasoft][runner][output]")
{
    const auto unknown = RunPlain({ "--jbos=2" });
    CHECK(CodeOf(unknown) == 1);
    CHECK_THAT(unknown.errors, ContainsSubstring("error: "));
    CHECK_THAT(unknown.errors, ContainsSubstring("did you mean --jobs?"));
}

TEST_CASE("Exit codes say what kind of run it was",
          "[progmasoft][runner][output]")
{
    const Scratch scratch;
    SECTION("nothing matched")
    {
        const auto run
            = RunFixture(scratch, { "--no-history", "no such test" });
        CHECK(ExitCode(run) == 2);
        // Catch2 quotes a spec that has spaces.
        CHECK_THAT(
            run.output,
            ContainsSubstring("warning: no test matches '\"no such test\"'"));
    }
    SECTION("nothing matched, and that was a warning")
    {
        const auto run = RunFixture(scratch,
                                    { "--no-history",
                                      "--warn=unmatched-specs",
                                      "[pass]",
                                      "no such test" });
        CHECK(ExitCode(run) == 3);
    }
    SECTION("nothing matched, and that was allowed")
    {
        const auto run = RunFixture(
            scratch,
            { "--no-history", "--allow-no-tests", "no such test" });
        CHECK(ExitCode(run) == 0);
    }
    SECTION("everything skipped")
    {
        const auto run = RunFixture(scratch, { "--no-history", "[skip]" });
        CHECK(ExitCode(run) == 4);
    }
    SECTION("a report that cannot be written")
    {
        // A directory where the report file should go.
        const auto blocked = scratch.Path() / "blocked.xml";
        std::filesystem::create_directories(blocked / "inside");
        const auto run = RunFixture(
            scratch,
            { "--no-history", "[pass]", "--junit=" + blocked.string() });
        CHECK(ExitCode(run) == 1);
        CHECK_THAT(run.output, ContainsSubstring("error: "));
    }
}

TEST_CASE("The console report follows the verbosity",
          "[progmasoft][runner][output]")
{
    const Scratch scratch;
    // Passing tests get a line, and their output, only when asked for.
    const auto normal = RunFixture(scratch, { "--no-history", "[pass]" });
    CHECK_THAT(normal.output, ContainsSubstring("Catch3: 2 test(s)"));
    CHECK(normal.output.find("PASS fixture prints") == std::string::npos);
    CHECK(normal.output.find("said through std::cout") == std::string::npos);

    const auto verbose
        = RunFixture(scratch, { "--no-history", "-v", "[pass]" });
    CHECK_THAT(verbose.output, ContainsSubstring("PASS fixture prints"));
    CHECK_THAT(verbose.output, ContainsSubstring("said through std::cout"));

    const auto quiet = RunFixture(scratch, { "--no-history", "-q", "[pass]" });
    CHECK(ExitCode(quiet) == 0);
    CHECK(quiet.output.find("Catch3: ") == std::string::npos);

    const auto slowest
        = RunFixture(scratch, { "--no-history", "--slowest=2", "[slow]" });
    CHECK_THAT(slowest.output, ContainsSubstring("slowest tests:"));

    const auto coloured
        = RunPlain({ "--no-history", "--color=always", "[pass]" });
    CHECK_THAT(coloured.output, ContainsSubstring("\x1b["));
    const auto plain = RunPlain({ "--no-history", "--color=never", "[pass]" });
    CHECK(plain.output.find("\x1b[") == std::string::npos);
}

TEST_CASE("The history is summarised on request",
          "[progmasoft][runner][output]")
{
    const Scratch scratch;
    const auto history = "--history=" + scratch.Path().string();
    for (int run = 0; run < 3; ++run)
    {
        static_cast<void>(
            RunFixture(scratch,
                       { history, "--retries=off", "[pass],[alternate]" }));
    }

    const auto text = RunPlain({ history, "--analytics" });
    CHECK(CodeOf(text) == 0);
    CHECK_THAT(text.output, ContainsSubstring("3 run(s)"));
    CHECK_THAT(text.output, ContainsSubstring("flaky (last 20 runs):"));
    CHECK_THAT(text.output, ContainsSubstring("fixture alternates"));
    CHECK_THAT(text.output, ContainsSubstring("slowest (median):"));

    const auto json = RunPlain({ history, "--analytics=json" });
    CHECK(CodeOf(json) == 0);
    auto parsed = Runner::ParseJson(json.output);
    REQUIRE(parsed);
    CHECK((*parsed)["format"].AsString() == "catch3-analytics");
    CHECK((*parsed)["runs"].AsInteger() == 3);
    CHECK_FALSE((*parsed)["tests"].Items().empty());
}

TEST_CASE("Bazel's test environment is honoured",
          "[progmasoft][runner][output]")
{
    const Scratch scratch;
    const auto xml = scratch.Path() / "test.xml";
    const auto outputs = scratch.Path() / "outputs";
    const auto status = scratch.Path() / "shard.status";
    std::filesystem::create_directories(outputs);
    const auto run
        = RunPlain({ "--color=never" },
                   { { "BAZEL_TEST", "1" },
                     { "XML_OUTPUT_FILE", xml.string() },
                     { "TESTBRIDGE_TEST_ONLY", "fixture passes" },
                     { "TEST_SHARD_INDEX", "0" },
                     { "TEST_TOTAL_SHARDS", "1" },
                     { "TEST_SHARD_STATUS_FILE", status.string() },
                     { "TEST_RANDOM_SEED", "1234" },
                     { "TEST_UNDECLARED_OUTPUTS_DIR", outputs.string() },
                     { "CATCH3_FIXTURE_STATE", scratch.Path().string() } });
    CHECK(CodeOf(run) == 0);
    CHECK_THAT(run.output, ContainsSubstring("1 test(s): 1 passed"));
    CHECK(std::filesystem::exists(status));

    auto junit = Runner::ReadFile(xml);
    REQUIRE(junit);
    CHECK_THAT(*junit, ContainsSubstring("fixture passes"));

    auto report = Runner::ReadFile(outputs / "catch3.json");
    REQUIRE(report);
    auto parsed = Runner::ParseJson(*report);
    REQUIRE(parsed);
    CHECK((*parsed)["seed"].AsInteger() == 1234);
    // A sandboxed test keeps no history beside the binary.
    const auto beside
        = std::filesystem::path{ FixtureClient::Fixture() }.parent_path()
          / ".catch3";
    CHECK_FALSE(std::filesystem::exists(beside));
}
