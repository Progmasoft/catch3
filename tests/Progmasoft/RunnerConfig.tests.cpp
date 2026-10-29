// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

// catch3.json: where it is found, what its keys set, how the command line
// wins over it and how a wrong file is explained.

#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/Options.hpp>
#include <Progmasoft/Runner/ProjectConfig.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    namespace fs = std::filesystem;
    using Catch::Matchers::ContainsSubstring;
    using std::chrono::milliseconds;

    /// A directory of its own under the system's temporary directory,
    /// removed with everything in it at the end of the test.
    class Scratch final
    {
    public:
        Scratch()
        {
            std::error_code error;
            root_ = fs::temp_directory_path(error)
                    / ("catch3-config-" + Runner::UniqueName());
            fs::create_directories(root_, error);
            REQUIRE_FALSE(error);
        }

        Scratch(const Scratch &) = delete;
        Scratch &
        operator=(const Scratch &) = delete;

        ~Scratch()
        {
            std::error_code error;
            fs::remove_all(root_, error);
        }

        [[nodiscard]] const fs::path &
        Root() const noexcept
        {
            return root_;
        }

        fs::path
        Directory(const fs::path &relative) const
        {
            std::error_code error;
            fs::create_directories(root_ / relative, error);
            REQUIRE_FALSE(error);
            return root_ / relative;
        }

        fs::path
        Write(const fs::path &relative, const std::string &text) const
        {
            const auto file = root_ / relative;
            std::error_code error;
            fs::create_directories(file.parent_path(), error);
            std::ofstream stream(file, std::ios::binary);
            stream << text;
            REQUIRE(stream);
            return file;
        }

    private:
        fs::path root_;
    };

    Runner::Options
    Apply(const std::string &text, const fs::path &directory = {})
    {
        auto options
            = Runner::ApplyProjectConfig(text, directory, "catch3.json");
        INFO((options ? std::string{} : options.GetError().Message()));
        REQUIRE(options);
        return *options;
    }

    std::string
    ApplyError(const std::string &text)
    {
        auto options = Runner::ApplyProjectConfig(text, {}, "catch3.json");
        REQUIRE_FALSE(options);
        return options.GetError().Message();
    }

    bool
    InsideRepository(fs::path directory)
    {
        std::error_code error;
        for (;; directory = directory.parent_path())
        {
            if (fs::exists(directory / ".git", error))
            {
                return true;
            }
            if (!directory.has_relative_path())
            {
                return false;
            }
        }
    }
} // namespace

TEST_CASE("catch3.json is found up to the root of the repository",
          "[progmasoft][runner][config]")
{
    const Scratch scratch;
    scratch.Directory(".git");
    const auto deep = scratch.Directory("a/b/c");

    CHECK_FALSE(Runner::FindProjectConfig(deep));

    const auto top = scratch.Write("catch3.json", "{\"version\": 1}");
    CHECK(Runner::FindProjectConfig(deep) == top);
    CHECK(Runner::FindProjectConfig(scratch.Root()) == top);

    // The nearest file wins.
    const auto near = scratch.Write("a/b/catch3.json", "{\"version\": 1}");
    CHECK(Runner::FindProjectConfig(deep) == near);
    CHECK(Runner::FindProjectConfig(scratch.Root() / "a") == top);
}

TEST_CASE("The search stops at the repository it starts in",
          "[progmasoft][runner][config]")
{
    const Scratch scratch;
    scratch.Write("catch3.json", "{\"version\": 1}");
    scratch.Directory("project/.git");
    const auto inner = scratch.Directory("project/tests");
    CHECK_FALSE(Runner::FindProjectConfig(inner));
}

TEST_CASE("Outside a repository only the working directory is searched",
          "[progmasoft][runner][config]")
{
    const Scratch scratch;
    if (InsideRepository(scratch.Root()))
    {
        SKIP("the temporary directory is inside a Git repository");
    }
    scratch.Write("catch3.json", "{\"version\": 1}");
    const auto below = scratch.Directory("below");
    CHECK_FALSE(Runner::FindProjectConfig(below));
    CHECK(Runner::FindProjectConfig(scratch.Root())
          == scratch.Root() / "catch3.json");
}

TEST_CASE("Keys set the option of the same name",
          "[progmasoft][runner][config]")
{
    const auto options = Apply(R"({
        "version": 1,
        "$schema": "https://catch3.progmasoft.com/schema/catch3-v1.json",
        "tests": ["[unit]", "~[slow]"],
        "select": "smart",
        "changes-since": "origin/devel",
        "incremental": true,
        "filenames-as-tags": true,
        "jobs": 6,
        "isolate": true,
        "timeout": 2.5,
        "adaptive-timeout": false,
        "retries": 3,
        "repeat": 2,
        "fail-fast": 4,
        "order": "random",
        "seed": 77,
        "bisect": false,
        "success": true,
        "no-throw": true,
        "warn": ["no-assertions", "infinite-generators"],
        "allow-no-tests": true,
        "skip-benchmarks": true,
        "benchmark-samples": 10,
        "benchmark-resamples": 1000,
        "benchmark-confidence-interval": 0.9,
        "benchmark-no-analysis": true,
        "benchmark-warmup-time": 5,
        "quarantine": "report",
        "verbosity": "quiet",
        "color": "never",
        "slowest": 3
    })");
    CHECK(options.testSpecs == std::vector<std::string>{ "[unit]", "~[slow]" });
    CHECK(options.selection == Runner::Selection::kSmart);
    CHECK(options.changesSince == "origin/devel");
    CHECK(options.incremental);
    CHECK(options.filenamesAsTags);
    CHECK(options.jobs == 6);
    CHECK(options.isolate);
    CHECK(options.timeout == milliseconds{ 2500 });
    CHECK(options.timeoutGiven);
    CHECK_FALSE(options.adaptiveTimeout);
    CHECK(options.retryMode == Runner::RetryMode::kAlways);
    CHECK(options.retries == 3);
    CHECK(options.repeat == 2);
    CHECK(options.failFast == 4);
    CHECK(options.order == Runner::Order::kRandom);
    CHECK(options.seed == 77);
    CHECK(options.seedGiven);
    CHECK_FALSE(options.bisectOrder);
    CHECK(options.includeSuccessful);
    CHECK(options.noThrow);
    CHECK(options.warnNoAssertions);
    CHECK(options.warnInfiniteGenerators);
    CHECK_FALSE(options.warnUnmatchedSpecs);
    CHECK(options.allowNoTests);
    CHECK(options.skipBenchmarks);
    CHECK(options.benchmarkSamples == 10);
    CHECK(options.benchmarkResamples == 1000);
    CHECK(options.benchmarkConfidenceInterval == 0.9);
    CHECK(options.benchmarkNoAnalysis);
    CHECK(options.benchmarkWarmupTime == milliseconds{ 5 });
    CHECK(options.quarantine == Runner::QuarantineMode::kReport);
    CHECK(options.verbosity == Runner::Verbosity::kQuiet);
    CHECK(options.color == Runner::ColorMode::kNever);
    CHECK(options.slowest == 3);
}

TEST_CASE("Words and defaults read like the command line",
          "[progmasoft][runner][config]")
{
    CHECK(Apply(R"({"version": 1, "retries": "off"})").retryMode
          == Runner::RetryMode::kOff);
    CHECK_FALSE(Apply(R"({"version": 1, "seed": "random"})").seedGiven);
    CHECK_FALSE(Apply(R"({"version": 1, "history": false})").historyEnabled);
    CHECK(Apply(R"({"version": 1, "verbosity": "verbose"})").verbosity
          == Runner::Verbosity::kVerbose);
    CHECK(Apply(R"({"version": 1, "verbosity": "normal"})").verbosity
          == Runner::Verbosity::kNormal);

    // false is the default of a switch, and true of a negated one.
    const auto defaults = Apply(R"({"version": 1, "isolate": false,
                                    "adaptive-timeout": true,
                                    "bisect": true})");
    CHECK_FALSE(defaults.isolate);
    CHECK(defaults.adaptiveTimeout);
    CHECK(defaults.bisectOrder);
}

TEST_CASE("Relative paths are read from the directory of the file",
          "[progmasoft][runner][config]")
{
    const fs::path directory = fs::path{ "project" } / "tests";
    const auto options = Apply(R"({
        "version": 1,
        "junit": "out/junit.xml",
        "json": "../report.json",
        "trace": "trace.json",
        "history": ".catch3",
        "shard-durations": "durations.json"
    })",
                               directory);
    CHECK(fs::path{ options.junitPath }
          == (directory / "out/junit.xml").lexically_normal());
    CHECK(fs::path{ options.jsonPath }
          == fs::path{ "project/report.json" }.lexically_normal());
    CHECK(fs::path{ options.tracePath }
          == (directory / "trace.json").lexically_normal());
    CHECK(fs::path{ options.historyDirectory }
          == (directory / ".catch3").lexically_normal());
    CHECK(fs::path{ options.shardDurations }
          == (directory / "durations.json").lexically_normal());

    const auto absolute = (fs::temp_directory_path() / "j.xml").string();
    CHECK(fs::path{ Apply(
                        "{\"version\": 1, \"junit\": " +
                            [&] {
                                std::string quoted;
                                for (const char character : absolute)
                                {
                                    if (character == '\\' || character == '"')
                                    {
                                        quoted += '\\';
                                    }
                                    quoted += character;
                                }
                                return "\"" + quoted + "\"";
                            }()
                            + "}",
                        directory)
                        .junitPath }
          == fs::path{ absolute }.lexically_normal());
}

TEST_CASE("A wrong catch3.json is explained", "[progmasoft][runner][config]")
{
    CHECK_THAT(ApplyError("{}"),
               ContainsSubstring("\"version\": 1 is missing"));
    CHECK_THAT(ApplyError(R"({"version": 2})"),
               ContainsSubstring("this Catch3 reads version 1"));
    CHECK_THAT(ApplyError("[1]"), ContainsSubstring("holds an object"));
    CHECK_THAT(ApplyError(R"({"version": 1,})"),
               ContainsSubstring("catch3.json: "));
    CHECK_THAT(ApplyError(R"({"version": 1, "version": 1})"),
               ContainsSubstring("\"version\" appears twice"));

    CHECK_THAT(ApplyError(R"({"version": 1, "jbos": 4})"),
               ContainsSubstring("did you mean \"jobs\"?"));
    CHECK_THAT(ApplyError(R"({"version": 1, "zzzzzzzz": 4})"),
               ContainsSubstring("\"zzzzzzzz\" is not a key of catch3.json")
                   && !ContainsSubstring("did you mean"));
    CHECK_THAT(ApplyError(R"({"version": 1, "shard": "0/2"})"),
               ContainsSubstring("belongs on the command line"));

    CHECK_THAT(ApplyError(R"({"version": 1, "jobs": "4"})"),
               ContainsSubstring("\"jobs\" takes a whole number"));
    CHECK_THAT(ApplyError(R"({"version": 1, "jobs": -1})"),
               ContainsSubstring("\"jobs\" takes a whole number"));
    CHECK_THAT(ApplyError(R"({"version": 1, "jobs": 1.5})"),
               ContainsSubstring("\"jobs\" takes a whole number"));
    CHECK_THAT(ApplyError(R"({"version": 1, "isolate": 1})"),
               ContainsSubstring("\"isolate\" takes true or false"));
    CHECK_THAT(ApplyError(R"({"version": 1, "timeout": -2})"),
               ContainsSubstring("\"timeout\" takes a number of at least 0"));
    CHECK_THAT(ApplyError(R"({"version": 1, "tests": "[unit]"})"),
               ContainsSubstring("an array of test specs"));
    CHECK_THAT(ApplyError(R"({"version": 1, "warn": ["no-assertions", 3]})"),
               ContainsSubstring("an array of strings"));
    CHECK_THAT(ApplyError(R"({"version": 1, "verbosity": "loud"})"),
               ContainsSubstring("\"quiet\", \"normal\" or \"verbose\""));
    CHECK_THAT(ApplyError(R"({"version": 1, "history": true})"),
               ContainsSubstring("a path or false"));

    // The checks of the command line apply, named by the key.
    CHECK_THAT(ApplyError(R"({"version": 1, "order": "sideways"})"),
               ContainsSubstring("\"order\": --order takes one of"));
    CHECK_THAT(ApplyError(R"({"version": 1, "repeat": 0})"),
               ContainsSubstring("\"repeat\": --repeat takes a whole number"));
}

TEST_CASE("The command line wins over catch3.json",
          "[progmasoft][runner][config]")
{
    const Scratch scratch;
    scratch.Directory(".git");
    const auto file = scratch.Write("catch3.json", R"({
        "version": 1,
        "jobs": 3,
        "order": "name",
        "tests": ["[unit]"],
        "junit": "junit.xml"
    })");
    const auto here = scratch.Directory("build");

    const auto fromFile = Runner::ParseCommandLine({}, here);
    REQUIRE(fromFile);
    CHECK(fromFile->jobs == 3);
    CHECK(fromFile->order == Runner::Order::kName);
    CHECK(fromFile->testSpecs == std::vector<std::string>{ "[unit]" });
    CHECK(fs::path{ fromFile->junitPath }
          == (scratch.Root() / "junit.xml").lexically_normal());
    CHECK(fs::path{ fromFile->configPath } == file.lexically_normal());

    const auto overridden = Runner::ParseCommandLine(
        { "--jobs=8", "Some test", "--order", "declared" },
        here);
    REQUIRE(overridden);
    CHECK(overridden->jobs == 8);
    CHECK(overridden->order == Runner::Order::kDeclared);
    // Specs on the command line replace those of the file.
    CHECK(overridden->testSpecs == std::vector<std::string>{ "Some test" });
    CHECK_FALSE(overridden->junitPath.empty());

    const auto ignored = Runner::ParseCommandLine({ "--no-config" }, here);
    REQUIRE(ignored);
    CHECK(ignored->jobs == 0);
    CHECK(ignored->configPath.empty());
    CHECK(ignored->testSpecs.empty());
}

TEST_CASE("--config names the file, and some modes read none",
          "[progmasoft][runner][config]")
{
    const Scratch scratch;
    scratch.Write("ci/catch3.json", R"({"version": 1, "jobs": 2})");
    scratch.Write("broken.json", R"({"version": 1, "jobs": "many"})");

    const auto named = Runner::ParseCommandLine({ "--config=ci/catch3.json" },
                                                scratch.Root());
    REQUIRE(named);
    CHECK(named->jobs == 2);

    const auto broken = Runner::ParseCommandLine({ "--config", "broken.json" },
                                                 scratch.Root());
    REQUIRE_FALSE(broken);
    CHECK_THAT(broken.GetError().Message(),
               ContainsSubstring("broken.json: \"jobs\" takes a whole number"));

    const auto missing
        = Runner::ParseCommandLine({ "--config=absent.json" }, scratch.Root());
    REQUIRE_FALSE(missing);
    CHECK_THAT(missing.GetError().Message(),
               ContainsSubstring("cannot read the configuration"));

    // A broken file does not stand in the way of help, the version or a
    // worker, which takes everything from its runner.
    for (const char *mode : { "--help", "--version", "--catch3-worker" })
    {
        CAPTURE(mode);
        CHECK(Runner::ParseCommandLine({ "--config=broken.json", mode },
                                       scratch.Root()));
    }
    // The last of --config and --no-config decides.
    CHECK(Runner::ParseCommandLine({ "--config=broken.json", "--no-config" },
                                   scratch.Root()));
    CHECK_FALSE(
        Runner::ParseCommandLine({ "--no-config", "--config=broken.json" },
                                 scratch.Root()));
}
