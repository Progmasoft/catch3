// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// Per-test coverage: `--coverage`, the formats its reports read and write,
// and a run of a binary built with coverage instrumentation.

#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Runner/CoverageReport.hpp>
#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/Json.hpp>
#include <Progmasoft/Runner/Options.hpp>
#include <Progmasoft/Runner/Process.hpp>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "RunnerFixtureClient.hpp"

namespace
{
    namespace Runner = Progmasoft::Catch3::Runner;
    using Catch::Matchers::ContainsSubstring;
    using FixtureClient::Scratch;

    /// The lines of `file` that end with the marker `// coverage: name`.
    std::size_t
    MarkedLine(const std::string &file, const std::string &name)
    {
        auto text = Runner::ReadFile(file);
        REQUIRE(text);
        std::size_t line = 1;
        std::size_t start = 0;
        const std::string marker = "// coverage: " + name;
        while (start < text->size())
        {
            auto end = text->find('\n', start);
            if (end == std::string::npos)
            {
                end = text->size();
            }
            if (text->substr(start, end - start).find(marker)
                != std::string::npos)
            {
                return line;
            }
            start = end + 1;
            ++line;
        }
        FAIL("no line of " << file << " is marked " << name);
        return 0;
    }

    /// The lines a test of `coverage.json` reached in the fixture's source.
    std::vector<std::size_t>
    ReachedLines(const Runner::JsonValue &report,
                 const std::string &test,
                 std::string &source)
    {
        for (const auto &entry : report["tests"].Items())
        {
            if (entry["name"].AsString() != test)
            {
                continue;
            }
            for (const auto &file : entry["files"].Items())
            {
                const std::string path = file["file"].AsString();
                if (path.find("CoverageFixture.cpp") == std::string::npos)
                {
                    continue;
                }
                source = path;
                std::vector<std::size_t> lines;
                for (const auto &line : file["lines"].Items())
                {
                    lines.push_back(
                        static_cast<std::size_t>(line.AsUnsigned()));
                }
                return lines;
            }
        }
        FAIL("coverage.json has no lines of the fixture for '" << test << "'");
        return {};
    }

    bool
    Contains(const std::vector<std::size_t> &lines, std::size_t line)
    {
        return std::find(lines.begin(), lines.end(), line) != lines.end();
    }
} // namespace

TEST_CASE("--coverage takes a directory and needs worker processes",
          "[progmasoft][runner][coverage]")
{
    auto options = Runner::ParseOptions({ "--coverage=out/coverage" });
    REQUIRE(options);
    CHECK(options->coverageDirectory == "out/coverage");

    auto inProcess = Runner::ParseOptions({ "--coverage=c", "--in-process" });
    REQUIRE_FALSE(inProcess);
    CHECK_THAT(inProcess.GetError().Message(),
               ContainsSubstring("cannot be used with --in-process"));

    auto empty = Runner::ParseOptions({ "--coverage=" });
    REQUIRE_FALSE(empty);
}

TEST_CASE("LCOV tracefiles are read and written",
          "[progmasoft][runner][coverage]")
{
    const auto map = Runner::ParseLcov("TN:\n"
                                       "SF:/src/a.cpp\n"
                                       "DA:3,2\n"
                                       "DA:4,0\n"
                                       "BRDA:3,0,0,2\n"
                                       "BRDA:3,0,1,-\n"
                                       "end_of_record\r\n"
                                       "SF:/src/b.cpp\n"
                                       "DA:1,1\n"
                                       "end_of_record\n");
    REQUIRE(map.size() == 2);
    const auto &a = map.at("/src/a.cpp");
    CHECK(a.lines.at(3) == 2);
    CHECK(a.lines.at(4) == 0);
    REQUIRE(a.branches.size() == 2);
    CHECK(a.branches.at({ 3, 0 }) == 2);
    CHECK(a.branches.at({ 3, 1 }) == 0);
    CHECK(map.at("/src/b.cpp").lines.at(1) == 1);

    const std::string written = Runner::ToLcov(map);
    CHECK_THAT(written, ContainsSubstring("SF:/src/a.cpp\nDA:3,2\nDA:4,0\n"));
    CHECK_THAT(written, ContainsSubstring("BRDA:3,0,1,-\n"));
    CHECK_THAT(written, ContainsSubstring("LF:2\nLH:1\nBRF:2\nBRH:1\n"));
    const auto again = Runner::ParseLcov(written);
    CHECK(again.at("/src/a.cpp").lines == a.lines);
    CHECK(again.at("/src/a.cpp").branches == a.branches);
}

TEST_CASE("gcov JSON adds up across data files",
          "[progmasoft][runner][coverage]")
{
    Runner::CoverageMap map;
    const std::string json
        = R"({"format_version":"2","files":[{"file":"/src/a.c","lines":[)"
          R"({"line_number":2,"count":3,"branches":[{"count":3},{"count":0}]},)"
          R"({"line_number":5,"count":0,"branches":[]}]}]})";
    Runner::AddGcovJson(json, map);
    Runner::AddGcovJson(json, map);
    const auto &a = map.at("/src/a.c");
    CHECK(a.lines.at(2) == 6);
    CHECK(a.lines.at(5) == 0);
    CHECK(a.branches.at({ 2, 0 }) == 6);
    CHECK(a.branches.at({ 2, 1 }) == 0);
    Runner::AddGcovJson("not json", map);
    CHECK(map.size() == 1);
}

TEST_CASE("Each test of a coverage run gets its own profile and map",
          "[progmasoft][runner][coverage]")
{
    const auto fixture = Runner::ReadEnvironment("CATCH3_COVERAGE_FIXTURE");
    if (fixture.empty())
    {
        SKIP("CATCH3_COVERAGE_FIXTURE names no binary built with coverage");
    }
    const Scratch scratch;
    const auto directory = scratch.Path() / "coverage";
    // A profile of an earlier run is removed, not added to.
    std::filesystem::create_directories(directory / "profiles");
    REQUIRE(
        Runner::WriteFileAtomically(directory / "profiles" / "stale.profraw",
                                    "stale"));

    Runner::ChildOptions options;
    options.arguments = { FixtureClient::FromRunfiles(fixture).string(),
                          "--coverage=" + directory.string(),
                          "--no-history",
                          "--color=never" };
    options.workingDirectory = scratch.Path();
    auto run = Runner::RunAndCapture(options);
    REQUIRE(run);
    const std::string output = run->output + run->errors;
    INFO(output);
    REQUIRE(run->status.Succeeded());
    CHECK_FALSE(
        std::filesystem::exists(directory / "profiles" / "stale.profraw"));
    CHECK_THAT(output, ContainsSubstring("--coverage: 2 test profile(s)"));

    if (output.find("not found, so there is no map") != std::string::npos)
    {
        // CI sets the variable where the tools are installed, so that a
        // missing map fails there instead of passing as skipped.
        if (!Runner::ReadEnvironment("CATCH3_COVERAGE_REQUIRE_MAP").empty())
        {
            FAIL("the coverage tools are not on PATH");
        }
        SKIP("the coverage tools of the compiler are not on PATH");
    }
    auto text = Runner::ReadFile(directory / "coverage.json");
    REQUIRE(text);
    auto report = Runner::ParseJson(*text);
    REQUIRE(report);
    CHECK(((*report)["format"].AsString() == "llvm"
           || (*report)["format"].AsString() == "gcov"));
    REQUIRE((*report)["tests"].Items().size() == 2);

    std::string source;
    const auto adds = ReachedLines(*report, "Adds one", source);
    const auto subtracts = ReachedLines(*report, "Subtracts one", source);
    const auto addLine = MarkedLine(source, "add-one");
    const auto subtractLine = MarkedLine(source, "subtract-one");
    CHECK(Contains(adds, addLine));
    CHECK_FALSE(Contains(adds, subtractLine));
    CHECK(Contains(subtracts, subtractLine));
    CHECK_FALSE(Contains(subtracts, addLine));

    auto lcov = Runner::ReadFile(directory / "coverage.lcov");
    REQUIRE(lcov);
    const auto combined = Runner::ParseLcov(*lcov);
    const auto file = combined.find(source);
    REQUIRE(file != combined.end());
    CHECK(file->second.lines.at(addLine) > 0);
    CHECK(file->second.lines.at(subtractLine) > 0);
}
