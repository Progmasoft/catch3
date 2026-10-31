// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

// What the runner's integration tests share: finding the fixture binary,
// running it in a directory of its own and reading its JSON report.

#include <Progmasoft/Runner/Files.hpp>
#include <Progmasoft/Runner/Json.hpp>
#include <Progmasoft/Runner/Process.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace FixtureClient
{
    namespace Runner = Progmasoft::Catch3::Runner;

    struct FixtureRun final
    {
        Runner::ExitStatus status;
        std::string output;
        Runner::JsonValue report;
        std::chrono::milliseconds wall{ 0 };

        /// The report entry of a test.
        [[nodiscard]] const Runner::JsonValue &
        Test(const std::string &name) const
        {
            for (const auto &test : report["tests"].Items())
            {
                if (test["name"].AsString() == name)
                {
                    return test;
                }
            }
            FAIL("no result for '" << name << "' in:\n" << output);
            return report;
        }

        [[nodiscard]] std::string
        Analysis(const std::string &name) const
        {
            std::string text;
            for (const auto &finding : Test(name)["analysis"].Items())
            {
                text += finding.AsString() + "\n";
            }
            return text;
        }
    };

    /// Finds a file Bazel provides as runfiles: beside the test, in the
    /// runfiles tree, or through the manifest Bazel writes on Windows.
    inline std::filesystem::path
    FromRunfiles(const std::string &path)
    {
        std::error_code error;
        if (std::filesystem::exists(path, error))
        {
            return std::filesystem::absolute(path, error);
        }
        for (const char *variable : { "RUNFILES_DIR", "TEST_SRCDIR" })
        {
            const auto directory = Runner::ReadEnvironment(variable);
            if (!directory.empty())
            {
                const auto candidate
                    = std::filesystem::path{ directory } / "_main" / path;
                if (std::filesystem::exists(candidate, error))
                {
                    return candidate;
                }
            }
        }
        const auto manifest = Runner::ReadEnvironment("RUNFILES_MANIFEST_FILE");
        if (!manifest.empty())
        {
            auto text = Runner::ReadFile(manifest);
            const std::string key = "_main/" + path + " ";
            if (text)
            {
                const auto found = text->find(key);
                if (found != std::string::npos)
                {
                    const auto start = found + key.size();
                    auto end = text->find('\n', start);
                    std::string target = text->substr(start,
                                                      end == std::string::npos
                                                          ? std::string::npos
                                                          : end - start);
                    if (!target.empty() && target.back() == '\r')
                    {
                        target.pop_back();
                    }
                    return target;
                }
            }
        }
        return path;
    }

    inline std::string
    Fixture()
    {
        const auto path = Runner::ReadEnvironment("CATCH3_RUNNER_FIXTURE");
        if (path.empty())
        {
            SKIP("CATCH3_RUNNER_FIXTURE names no fixture binary");
        }
        return FromRunfiles(path).string();
    }

    /// A fresh directory for one test.
    class Scratch final
    {
    public:
        Scratch()
        {
            std::error_code error;
            path_ = std::filesystem::temp_directory_path(error)
                    / ("catch3-runner-test-" + Runner::UniqueName());
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

    inline FixtureRun
    RunFixture(const Scratch &scratch,
               std::vector<std::string> arguments,
               std::vector<std::pair<std::string, std::string>> environment
               = {})
    {
        Runner::ChildOptions options;
        options.arguments.push_back(Fixture());
        const auto report
            = scratch.Path() / ("report-" + Runner::UniqueName() + ".json");
        for (auto &argument : arguments)
        {
            options.arguments.push_back(std::move(argument));
        }
        options.arguments.push_back("--json=" + report.string());
        options.arguments.emplace_back("--color=never");
        options.environment = std::move(environment);
        options.environment.emplace_back("CATCH3_FIXTURE_STATE",
                                         scratch.Path().string());
        const auto started = std::chrono::steady_clock::now();
        auto run = Runner::RunAndCapture(options);
        INFO((run ? std::string{} : run.GetError().Message()));
        REQUIRE(run);
        FixtureRun result;
        result.wall = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
        result.status = run->status;
        result.output = run->output + run->errors;
        if (std::filesystem::exists(report))
        {
            auto text = Runner::ReadFile(report);
            REQUIRE(text);
            auto json = Runner::ParseJson(*text);
            REQUIRE(json);
            result.report = std::move(*json);
        }
        return result;
    }

    inline std::int64_t
    ExitCode(const FixtureRun &run)
    {
        INFO(run.output);
        REQUIRE(run.status.kind == Runner::ExitStatus::Kind::kExited);
        return run.status.code;
    }

    /// Runs the fixture with `arguments` as they are, for the modes that
    /// write no report.
    inline Runner::CapturedRun
    RunPlain(std::vector<std::string> arguments,
             std::vector<std::pair<std::string, std::string>> environment = {})
    {
        Runner::ChildOptions options;
        options.arguments.push_back(Fixture());
        for (auto &argument : arguments)
        {
            options.arguments.push_back(std::move(argument));
        }
        options.environment = std::move(environment);
        auto run = Runner::RunAndCapture(options);
        INFO((run ? std::string{} : run.GetError().Message()));
        REQUIRE(run);
        return std::move(*run);
    }
} // namespace FixtureClient
