// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// Every option of the runner's command line: how it is spelled, what it
// sets and how a wrong value is explained.

#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/Options.hpp>

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Generators/Generators.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using Catch::Matchers::ContainsSubstring;
    using std::chrono::milliseconds;

    Runner::Options
    Parse(const std::vector<std::string> &arguments)
    {
        auto options = Runner::ParseOptions(arguments);
        INFO((options ? std::string{} : options.GetError().Message()));
        REQUIRE(options);
        return *options;
    }

    std::string
    ParseError(const std::vector<std::string> &arguments)
    {
        auto options = Runner::ParseOptions(arguments);
        REQUIRE_FALSE(options);
        return options.GetError().Message();
    }
} // namespace

TEST_CASE("Options have their documented defaults",
          "[progmasoft][runner][options]")
{
    const auto options = Parse({});
    CHECK(options.mode == Runner::Mode::kRun);
    CHECK(options.selection == Runner::Selection::kAll);
    CHECK(options.order == Runner::Order::kPriority);
    CHECK(options.retryMode == Runner::RetryMode::kAuto);
    CHECK(options.retries == 2);
    CHECK(options.repeat == 1);
    CHECK(options.failFast == 0);
    CHECK(options.jobs == 0);
    CHECK(options.shard.index == 0);
    CHECK(options.shard.count == 1);
    CHECK_FALSE(options.timeoutGiven);
    CHECK(options.adaptiveTimeout);
    CHECK(options.bisectOrder);
    CHECK(options.historyEnabled);
    CHECK(options.quarantine == Runner::QuarantineMode::kAuto);
    CHECK(options.verbosity == Runner::Verbosity::kNormal);
    CHECK(options.color == Runner::ColorMode::kAuto);
    CHECK(options.benchmarkSamples == 100);
    CHECK(options.benchmarkResamples == 100000);
    CHECK(options.benchmarkConfidenceInterval == 0.95);
    CHECK(options.benchmarkWarmupTime == milliseconds{100});
    CHECK(options.testSpecs.empty());
}

TEST_CASE("A value can follow an option in every accepted spelling",
          "[progmasoft][runner][options]")
{
    const std::vector<std::string> arguments = GENERATE(
        std::vector<std::string>{"--jobs=4"},
        std::vector<std::string>{"--jobs", "4"},
        std::vector<std::string>{"-j4"},
        std::vector<std::string>{"-j=4"},
        std::vector<std::string>{"-j", "4"});
    CAPTURE(arguments);
    CHECK(Parse(arguments).jobs == 4);
}

TEST_CASE("Switches set what they say", "[progmasoft][runner][options]")
{
    CHECK(Parse({"--incremental"}).incremental);
    CHECK(Parse({"--filenames-as-tags"}).filenamesAsTags);
    CHECK(Parse({"-#"}).filenamesAsTags);
    CHECK(Parse({"--in-process"}).inProcess);
    CHECK(Parse({"--isolate"}).isolate);
    CHECK_FALSE(Parse({"--no-adaptive-timeout"}).adaptiveTimeout);
    CHECK_FALSE(Parse({"--no-bisect"}).bisectOrder);
    CHECK(Parse({"--success"}).includeSuccessful);
    CHECK(Parse({"-s"}).includeSuccessful);
    CHECK(Parse({"--no-throw"}).noThrow);
    CHECK(Parse({"-e"}).noThrow);
    CHECK(Parse({"--allow-no-tests"}).allowNoTests);
    CHECK(Parse({"--skip-benchmarks"}).skipBenchmarks);
    CHECK(Parse({"--benchmark-no-analysis"}).benchmarkNoAnalysis);
    CHECK_FALSE(Parse({"--no-history"}).historyEnabled);
    CHECK(Parse({"--replay"}).replayFailures);
    CHECK(Parse({"--verbose"}).verbosity == Runner::Verbosity::kVerbose);
    CHECK(Parse({"-q"}).verbosity == Runner::Verbosity::kQuiet);

    // A debugger needs the test in this process.
    const auto debugging = Parse({"-b"});
    CHECK(debugging.breakIntoDebugger);
    CHECK(debugging.inProcess);
}

TEST_CASE("Modes are chosen by their options", "[progmasoft][runner][options]")
{
    CHECK(Parse({"--help"}).mode == Runner::Mode::kHelp);
    CHECK(Parse({"-h"}).mode == Runner::Mode::kHelp);
    CHECK(Parse({"--version"}).mode == Runner::Mode::kVersion);
    CHECK(Parse({"--list-tags"}).mode == Runner::Mode::kListTags);
    CHECK(Parse({"-t"}).mode == Runner::Mode::kListTags);
    CHECK(Parse({"--catch3-worker"}).mode == Runner::Mode::kWorker);

    const auto text = Parse({"-l"});
    CHECK(text.mode == Runner::Mode::kListTests);
    CHECK(text.listFormat == Runner::ListFormat::kText);
    CHECK(Parse({"--list=names"}).listFormat == Runner::ListFormat::kNames);
    CHECK(Parse({"--list=json"}).listFormat == Runner::ListFormat::kJson);

    const auto analytics = Parse({"--analytics"});
    CHECK(analytics.mode == Runner::Mode::kAnalytics);
    CHECK_FALSE(analytics.analyticsJson);
    CHECK(Parse({"--analytics=json"}).analyticsJson);
}

TEST_CASE("Choices accept their names and nothing else",
          "[progmasoft][runner][options]")
{
    CHECK(Parse({"--select=all"}).selection == Runner::Selection::kAll);
    CHECK(Parse({"--select=failed"}).selection == Runner::Selection::kFailed);
    CHECK(Parse({"--select=changed"}).selection == Runner::Selection::kChanged);
    CHECK(Parse({"--select=new"}).selection == Runner::Selection::kNew);
    CHECK(Parse({"--select=smart"}).selection == Runner::Selection::kSmart);
    CHECK(Parse({"--order=name"}).order == Runner::Order::kName);
    CHECK(Parse({"--order=random"}).order == Runner::Order::kRandom);
    CHECK(Parse({"--order=priority"}).order == Runner::Order::kPriority);
    CHECK(Parse({"--quarantine=report"}).quarantine ==
          Runner::QuarantineMode::kReport);
    CHECK(Parse({"--quarantine=off"}).quarantine ==
          Runner::QuarantineMode::kOff);
    CHECK(Parse({"--color=always"}).color == Runner::ColorMode::kAlways);
    CHECK(Parse({"--color=never"}).color == Runner::ColorMode::kNever);

    CHECK_THAT(ParseError({"--select=some"}),
               ContainsSubstring("all, failed, changed, new, smart"));
    CHECK_THAT(ParseError({"--quarantine=maybe"}),
               ContainsSubstring("auto, report, off"));
    CHECK_THAT(ParseError({"--color=blue"}),
               ContainsSubstring("auto, always, never"));
    CHECK_THAT(ParseError({"--list=xml"}),
               ContainsSubstring("text, names, json"));
    CHECK_THAT(ParseError({"--analytics=xml"}), ContainsSubstring("json"));
}

TEST_CASE("Numbers are checked", "[progmasoft][runner][options]")
{
    CHECK(Parse({"--repeat=3"}).repeat == 3);
    CHECK(Parse({"-a"}).failFast == 1);
    CHECK(Parse({"--slowest"}).slowest == 10);
    CHECK(Parse({"--slowest=3"}).slowest == 3);
    CHECK(Parse({"--benchmark-samples=7"}).benchmarkSamples == 7);
    CHECK(Parse({"--benchmark-resamples=9"}).benchmarkResamples == 9);
    CHECK(Parse({"--benchmark-confidence-interval=0.5"})
              .benchmarkConfidenceInterval == 0.5);
    CHECK(Parse({"--benchmark-warmup-time=250"}).benchmarkWarmupTime ==
          milliseconds{250});

    const auto unlimited = Parse({"--timeout=0"});
    CHECK(unlimited.timeoutGiven);
    CHECK(unlimited.timeout == milliseconds{0});

    const auto retried = Parse({"--retries=auto"});
    CHECK(retried.retryMode == Runner::RetryMode::kAuto);
    const auto off = Parse({"--retries=off"});
    CHECK(off.retryMode == Runner::RetryMode::kOff);
    CHECK(off.retries == 0);

    const auto random = Parse({"--seed=5", "--seed=random"});
    CHECK_FALSE(random.seedGiven);

    CHECK_THAT(ParseError({"--repeat=0"}), ContainsSubstring("at least 1"));
    CHECK_THAT(ParseError({"--fail-fast=0"}), ContainsSubstring("at least 1"));
    CHECK_THAT(ParseError({"--slowest=0"}), ContainsSubstring("at least 1"));
    CHECK_THAT(ParseError({"--retries=-1"}), ContainsSubstring("whole number"));
    CHECK_THAT(ParseError({"--timeout=soon"}), ContainsSubstring("seconds"));
    CHECK_THAT(ParseError({"--timeout=-2"}), ContainsSubstring("seconds"));
    CHECK_THAT(ParseError({"--benchmark-confidence-interval=1"}),
               ContainsSubstring("--benchmark-confidence-interval"));
    CHECK_THAT(ParseError({"--benchmark-samples=0"}),
               ContainsSubstring("at least 1"));
    CHECK_THAT(ParseError({"--seed=-1"}), ContainsSubstring("--seed"));
    CHECK_THAT(ParseError({"--generator-index=x"}),
               ContainsSubstring("index or *"));
    CHECK_THAT(ParseError({"--path-filter=g:x"}),
               ContainsSubstring("generator index or *"));
}

TEST_CASE("Shards are an index below a count", "[progmasoft][runner][options]")
{
    const auto last = Parse({"--shard=3/4"});
    CHECK(last.shard.index == 3);
    CHECK(last.shard.count == 4);
    const std::string wrong = GENERATE("4/4", "1", "/2", "1/", "0/0", "a/b");
    CAPTURE(wrong);
    CHECK_THAT(ParseError({"--shard=" + wrong}), ContainsSubstring("INDEX/COUNT"));
}

TEST_CASE("Warnings are named", "[progmasoft][runner][options]")
{
    CHECK(Parse({"--warn=no-assertions"}).warnNoAssertions);
    CHECK(Parse({"-w", "NoAssertions"}).warnNoAssertions);
    CHECK(Parse({"--warn=unmatched-specs"}).warnUnmatchedSpecs);
    CHECK(Parse({"--warn=infinite-generators"}).warnInfiniteGenerators);
    CHECK_THAT(ParseError({"--warn=everything"}),
               ContainsSubstring("no-assertions"));
}

TEST_CASE("Files and directories are taken as written",
          "[progmasoft][runner][options]")
{
    const auto options = Parse({"--junit=out/junit.xml",
                                "--json",
                                "out/run.json",
                                "--trace=out/trace.json",
                                "--history=cache",
                                "--shard-durations=last.json",
                                "--changes-since=main",
                                "--premature-exit-guard-file=guard"});
    CHECK(options.junitPath == "out/junit.xml");
    CHECK(options.jsonPath == "out/run.json");
    CHECK(options.tracePath == "out/trace.json");
    CHECK(options.historyDirectory == "cache");
    CHECK(options.historyEnabled);
    CHECK(options.shardDurations == "last.json");
    CHECK(options.changesSince == "main");
    CHECK(options.exitGuardPath == "guard");
}

TEST_CASE("Test specs are kept apart from options",
          "[progmasoft][runner][options]")
{
    const auto options =
        Parse({"name", "[tag]", "~[slow]", "-", "--", "-j", "--x"});
    CHECK(options.jobs == 0);
    CHECK(options.testSpecs ==
          std::vector<std::string>{"name", "[tag]", "~[slow]", "-", "-j", "--x"});
}

TEST_CASE("An input file names tests exactly", "[progmasoft][runner][options]")
{
    std::error_code error;
    const auto file = std::filesystem::temp_directory_path(error) /
                      ("catch3-input-" + Runner::UniqueName());
    std::ofstream(file) << "# a comment\n"
                           "first test\r\n"
                           "\n"
                           "  \"quoted, with a comma\"  \n"
                           "last*\n";
    const auto options = Parse({"--input-file=" + file.string()});
    CHECK(options.testSpecs == std::vector<std::string>{"\"first test\"",
                                                        ",",
                                                        "\"quoted, with a comma\"",
                                                        ",",
                                                        "\"last*\""});
    std::filesystem::remove(file, error);
    CHECK_THAT(ParseError({"-f", file.string()}), ContainsSubstring("cannot read"));
}

TEST_CASE("Unknown options are refused with a suggestion",
          "[progmasoft][runner][options]")
{
    CHECK_THAT(ParseError({"--sucess"}), ContainsSubstring("did you mean --success?"));
    CHECK_THAT(ParseError({"--histroy=x"}), ContainsSubstring("did you mean --history?"));
    CHECK_THAT(ParseError({"-z"}), ContainsSubstring("'-z' is not an option"));
    // Catch2's reporter options are gone with its reporters.
    CHECK_THAT(ParseError({"--reporter=junit"}), ContainsSubstring("is not an option"));
    CHECK_THAT(ParseError({"--isolate=yes"}), ContainsSubstring("takes no value"));
    CHECK_THAT(ParseError({"--junit"}), ContainsSubstring("--junit=FILE"));
}

TEST_CASE("The help lists every option", "[progmasoft][runner][options]")
{
    const std::string help = Runner::HelpText("tests");
    CHECK_THAT(help, ContainsSubstring("usage: tests [options] [test specs...]"));
    for (const char *name : {"--section",       "--generator-index",
                             "--path-filter",   "--input-file",
                             "--select",        "--changes-since",
                             "--incremental",   "--shard",
                             "--shard-durations", "--filenames-as-tags",
                             "--jobs",          "--in-process",
                             "--isolate",       "--timeout",
                             "--no-adaptive-timeout", "--retries",
                             "--repeat",        "--fail-fast",
                             "--order",         "--seed",
                             "--no-bisect",     "--success",
                             "--break",         "--no-throw",
                             "--warn",          "--allow-no-tests",
                             "--skip-benchmarks", "--benchmark-samples",
                             "--benchmark-resamples",
                             "--benchmark-confidence-interval",
                             "--benchmark-no-analysis",
                             "--benchmark-warmup-time",
                             "--history",       "--no-history",
                             "--quarantine",    "--replay",
                             "--analytics",     "--verbose",
                             "--quiet",         "--color",
                             "--junit",         "--json",
                             "--trace",         "--premature-exit-guard-file",
                             "--slowest",       "--list",
                             "--list-tags",     "--help",
                             "--version"})
    {
        CAPTURE(name);
        CHECK_THAT(help, ContainsSubstring(name));
    }
    // The worker option is internal.
    CHECK_THAT(help, !ContainsSubstring("--catch3-worker"));
}

TEST_CASE("A worker gets every option that changes how tests behave",
          "[progmasoft][runner][options]")
{
    const auto options = Parse({"--section=outer",
                                "--section=inner",
                                "-p",
                                "g:1",
                                "-#",
                                "-s",
                                "-e",
                                "--warn=no-assertions",
                                "--warn=infinite-generators",
                                "--skip-benchmarks",
                                "--benchmark-no-analysis",
                                "--benchmark-samples=3",
                                "--benchmark-resamples=4",
                                "--benchmark-warmup-time=5",
                                "--benchmark-confidence-interval=0.9",
                                "--seed=6",
                                // Runner-side only:
                                "--jobs=9",
                                "--junit=x.xml",
                                "[tag]"});
    const auto worker = Parse(Runner::WorkerArguments(options));
    CHECK(worker.mode == Runner::Mode::kWorker);
    CHECK(worker.sections == options.sections);
    CHECK(worker.paths == options.paths);
    CHECK(worker.filenamesAsTags);
    CHECK(worker.includeSuccessful);
    CHECK(worker.noThrow);
    CHECK(worker.warnNoAssertions);
    CHECK(worker.warnInfiniteGenerators);
    CHECK(worker.skipBenchmarks);
    CHECK(worker.benchmarkNoAnalysis);
    CHECK(worker.benchmarkSamples == 3);
    CHECK(worker.benchmarkResamples == 4);
    CHECK(worker.benchmarkWarmupTime == milliseconds{5});
    CHECK(worker.benchmarkConfidenceInterval == 0.9);
    CHECK(worker.seed == 6);
    CHECK(worker.jobs == 0);
    CHECK(worker.junitPath.empty());
    CHECK(worker.testSpecs.empty());
}
