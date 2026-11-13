// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <system_error>
#include <thread>

#include "CoverageReport.hpp"
#include "Files.hpp"
#include "Json.hpp"
#include "Process.hpp"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        /// Reads a non-negative decimal number; nothing for anything else,
        /// such as the `-` LCOV writes for a branch never reached.
        std::optional<std::uint64_t>
        Number(std::string_view text)
        {
            if (text.empty() || text.size() > 19)
            {
                return std::nullopt;
            }
            std::uint64_t value = 0;
            for (const char character : text)
            {
                if (character < '0' || character > '9')
                {
                    return std::nullopt;
                }
                value
                    = value * 10 + static_cast<std::uint64_t>(character - '0');
            }
            return value;
        }

        std::vector<std::string_view>
        Split(std::string_view text, char separator)
        {
            std::vector<std::string_view> parts;
            std::size_t start = 0;
            while (true)
            {
                const auto end = text.find(separator, start);
                parts.push_back(text.substr(start, end - start));
                if (end == std::string_view::npos)
                {
                    return parts;
                }
                start = end + 1;
            }
        }

        /// A tool of a coverage format: the program, or the command that
        /// runs it, such as `xcrun llvm-cov` on macOS.
        using Tool = std::vector<std::string>;

        std::optional<Tool>
        FindTool(const char *variable, const std::string &name)
        {
            std::vector<Tool> candidates;
            if (auto configured = ReadEnvironment(variable);
                !configured.empty())
            {
                candidates.push_back({ std::move(configured) });
            }
            candidates.push_back({ name });
#if defined(__APPLE__)
            candidates.push_back({ "xcrun", name });
#endif
            for (auto &candidate : candidates)
            {
                ChildOptions options;
                options.arguments = candidate;
                options.arguments.emplace_back("--version");
                options.searchPath = true;
                if (auto run = RunAndCapture(options);
                    run && run->status.Succeeded())
                {
                    return std::move(candidate);
                }
            }
            return std::nullopt;
        }

        /// Runs `tool` with `arguments`; its output, or why it failed.
        Expected<std::string>
        RunTool(const Tool &tool,
                const std::vector<std::string> &arguments,
                const std::filesystem::path &directory = {})
        {
            ChildOptions options;
            options.arguments = tool;
            options.arguments.insert(options.arguments.end(),
                                     arguments.begin(),
                                     arguments.end());
            options.searchPath = true;
            options.workingDirectory = directory;
            auto run = RunAndCapture(options);
            if (!run)
            {
                return run.GetError();
            }
            if (!run->status.Succeeded())
            {
                std::string message = run->errors;
                while (!message.empty()
                       && (message.back() == '\n' || message.back() == '\r'))
                {
                    message.pop_back();
                }
                return Error{ tool.back() + " failed: " + message };
            }
            return std::move(run->output);
        }

        void
        Add(CoverageMap &total, const CoverageMap &part)
        {
            for (const auto &[file, coverage] : part)
            {
                FileCoverage &into = total[file];
                for (const auto &[line, count] : coverage.lines)
                {
                    into.lines[line] += count;
                }
                for (const auto &[branch, count] : coverage.branches)
                {
                    into.branches[branch] += count;
                }
            }
        }

        /// What one test reached: the lines it ran and, per line, how many
        /// of its branches it took.
        JsonValue
        ReachedJson(const CoverageMap &map)
        {
            JsonValue files = JsonValue::Array();
            for (const auto &[file, coverage] : map)
            {
                JsonValue lines = JsonValue::Array();
                for (const auto &[line, count] : coverage.lines)
                {
                    if (count > 0)
                    {
                        lines.Append(line);
                    }
                }
                JsonValue branches = JsonValue::Array();
                std::map<std::size_t, std::pair<std::size_t, std::size_t>>
                    perLine;
                for (const auto &[branch, count] : coverage.branches)
                {
                    auto &[taken, total] = perLine[branch.first];
                    ++total;
                    if (count > 0)
                    {
                        ++taken;
                    }
                }
                for (const auto &[line, counts] : perLine)
                {
                    if (counts.first == 0)
                    {
                        continue;
                    }
                    branches.Append(
                        JsonValue::Object({ { "line", line },
                                            { "taken", counts.first },
                                            { "total", counts.second } }));
                }
                if (lines.Items().empty())
                {
                    continue;
                }
                files.Append(
                    JsonValue::Object({ { "file", file },
                                        { "lines", std::move(lines) },
                                        { "branches", std::move(branches) } }));
            }
            return files;
        }

        /// The .gcda files gcov wrote below `prefix`, each with the path the
        /// object was built at, where its .gcno is.
        std::vector<std::pair<std::filesystem::path, std::filesystem::path>>
        GcdaFiles(const std::filesystem::path &prefix)
        {
            std::vector<std::pair<std::filesystem::path, std::filesystem::path>>
                files;
            std::error_code error;
            for (auto entry
                 = std::filesystem::recursive_directory_iterator(prefix, error);
                 !error
                 && entry != std::filesystem::recursive_directory_iterator();
                 entry.increment(error))
            {
                if (!entry->is_regular_file(error)
                    || entry->path().extension() != ".gcda")
                {
                    continue;
                }
                // GCOV_PREFIX is put before the absolute path of the object.
                const auto relative
                    = std::filesystem::relative(entry->path(), prefix, error);
                std::filesystem::path original
                    = std::filesystem::path{ "/" } / relative;
                files.emplace_back(entry->path(), original);
            }
            std::sort(files.begin(), files.end());
            return files;
        }

        Expected<CoverageMap>
        GcovCoverage(const Tool &gcov, const std::filesystem::path &prefix)
        {
            CoverageMap map;
            for (const auto &[gcda, original] : GcdaFiles(prefix))
            {
                auto gcno = original;
                gcno.replace_extension(".gcno");
                auto beside = gcda;
                beside.replace_extension(".gcno");
                std::error_code error;
                std::filesystem::copy_file(
                    gcno,
                    beside,
                    std::filesystem::copy_options::overwrite_existing,
                    error);
                if (error)
                {
                    return Error{ "the notes file " + gcno.string()
                                  + " could not be read: " + error.message() };
                }
                auto json = RunTool(
                    gcov,
                    // Without --branch-probabilities the JSON has no branches.
                    { "--json-format",
                      "--branch-probabilities",
                      "--stdout",
                      gcda.filename().string() },
                    gcda.parent_path());
                if (!json)
                {
                    return json.GetError();
                }
                // One JSON document per data file, one per line.
                for (const auto line : Split(*json, '\n'))
                {
                    if (!line.empty())
                    {
                        AddGcovJson(std::string{ line }, map);
                    }
                }
            }
            return map;
        }
    } // namespace

    CoverageMap
    ParseLcov(const std::string &text)
    {
        CoverageMap map;
        FileCoverage *current = nullptr;
        for (auto line : Split(text, '\n'))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.remove_suffix(1);
            }
            if (line.substr(0, 3) == "SF:")
            {
                current = &map[std::string{ line.substr(3) }];
            }
            else if (line == "end_of_record")
            {
                current = nullptr;
            }
            else if (current != nullptr && line.substr(0, 3) == "DA:")
            {
                const auto fields = Split(line.substr(3), ',');
                const auto number = Number(fields[0]);
                const auto count
                    = fields.size() > 1 ? Number(fields[1]) : std::nullopt;
                if (number && count)
                {
                    current->lines[static_cast<std::size_t>(*number)] += *count;
                }
            }
            else if (current != nullptr && line.substr(0, 5) == "BRDA:")
            {
                const auto fields = Split(line.substr(5), ',');
                if (fields.size() != 4)
                {
                    continue;
                }
                const auto parsed = Number(fields[0]);
                if (!parsed)
                {
                    continue;
                }
                const auto number = static_cast<std::size_t>(*parsed);
                // The branches of a line are numbered in the order they come;
                // `-` counts as never taken.
                std::size_t index = 0;
                for (auto it = current->branches.lower_bound({ number, 0 });
                     it != current->branches.end()
                     && it->first.first == number;
                     ++it)
                {
                    ++index;
                }
                current->branches[{ number, index }]
                    += Number(fields[3]).value_or(0);
            }
        }
        return map;
    }

    void
    AddGcovJson(const std::string &text, CoverageMap &map)
    {
        auto document = ParseJson(text);
        if (!document)
        {
            return;
        }
        for (const auto &file : (*document)["files"].Items())
        {
            FileCoverage &coverage = map[file["file"].AsString()];
            for (const auto &line : file["lines"].Items())
            {
                const auto number = static_cast<std::size_t>(
                    line["line_number"].AsUnsigned());
                coverage.lines[number] += line["count"].AsUnsigned();
                std::size_t index = 0;
                for (const auto &branch : line["branches"].Items())
                {
                    coverage.branches[{ number, index++ }]
                        += branch["count"].AsUnsigned();
                }
            }
        }
    }

    std::string
    ToLcov(const CoverageMap &map)
    {
        std::string text;
        for (const auto &[file, coverage] : map)
        {
            text += "SF:" + file + "\n";
            std::size_t linesHit = 0;
            for (const auto &[line, count] : coverage.lines)
            {
                text += "DA:" + std::to_string(line) + ","
                        + std::to_string(count) + "\n";
                linesHit += count > 0 ? 1 : 0;
            }
            std::size_t branchesHit = 0;
            for (const auto &[branch, count] : coverage.branches)
            {
                text += "BRDA:" + std::to_string(branch.first) + ",0,"
                        + std::to_string(branch.second) + ","
                        + (count > 0 ? std::to_string(count) : "-") + "\n";
                branchesHit += count > 0 ? 1 : 0;
            }
            text += "LF:" + std::to_string(coverage.lines.size()) + "\n";
            text += "LH:" + std::to_string(linesHit) + "\n";
            text += "BRF:" + std::to_string(coverage.branches.size()) + "\n";
            text += "BRH:" + std::to_string(branchesHit) + "\n";
            text += "end_of_record\n";
        }
        return text;
    }

    Status
    PrepareCoverageDirectory(const std::filesystem::path &directory)
    {
        std::error_code error;
        std::filesystem::remove_all(directory / "profiles", error);
        if (error)
        {
            return Error{ "the profiles of an earlier run in "
                          + directory.string()
                          + " could not be removed: " + error.message() };
        }
        for (const char *report :
             { "coverage.json", "coverage.lcov", "coverage.profdata" })
        {
            std::filesystem::remove(directory / report, error);
            if (error)
            {
                return Error{ "the report " + (directory / report).string()
                              + " of an earlier run could not be removed: "
                              + error.message() };
            }
        }
        std::filesystem::create_directories(directory / "profiles", error);
        if (error)
        {
            return Error{ "the coverage directory " + directory.string()
                          + " could not be created: " + error.message() };
        }
        return Status::Ok();
    }

    const Attempt *
    ProfiledAttempt(const TestResult &test, const std::string &suffix)
    {
        for (auto attempt = test.attempts.rbegin();
             attempt != test.attempts.rend();
             ++attempt)
        {
            std::error_code error;
            if (!attempt->coverageProfile.empty()
                && std::filesystem::exists(attempt->coverageProfile + suffix,
                                           error))
            {
                return &*attempt;
            }
        }
        return nullptr;
    }

    namespace
    {
        /// The coverage tools of one profile format.
        struct CoverageTools final
        {
            bool llvm = false;
            std::optional<Tool> profdata;
            std::optional<Tool> cov;
            std::optional<Tool> gcov;
        };

        /// What the profile of one test gave.
        struct TestCoverage final
        {
            const TestResult *test = nullptr;
            std::string profile;
            std::optional<CoverageMap> reached;
            std::string failure;
        };

        TestCoverage
        ReadTestCoverage(const TestResult &test,
                         const Attempt &attempt,
                         const CoverageTools &tools,
                         const std::string &program)
        {
            TestCoverage result;
            result.test = &test;
            const std::string base = attempt.coverageProfile;
            if (tools.llvm)
            {
                result.profile = base + ".profraw";
                auto merged = RunTool(*tools.profdata,
                                      { "merge",
                                        "-sparse",
                                        result.profile,
                                        "-o",
                                        base + ".profdata" });
                if (!merged)
                {
                    result.failure = merged.GetError().Message();
                    return result;
                }
                auto lcov = RunTool(*tools.cov,
                                    { "export",
                                      "-format=lcov",
                                      "-instr-profile=" + base + ".profdata",
                                      program });
                if (!lcov)
                {
                    result.failure = lcov.GetError().Message();
                    return result;
                }
                result.reached = ParseLcov(*lcov);
                return result;
            }
            result.profile = base + ".gcov";
            auto reached = GcovCoverage(*tools.gcov, result.profile);
            if (!reached)
            {
                result.failure = reached.GetError().Message();
                return result;
            }
            result.reached = std::move(*reached);
            return result;
        }

        /// Reads the profile of each of `tests`, up to `jobs` at a time;
        /// the results keep the order of `tests`.
        std::vector<TestCoverage>
        ReadAll(
            const std::vector<std::pair<const TestResult *, const Attempt *>>
                &tests,
            const CoverageTools &tools,
            const std::string &program,
            std::size_t jobs)
        {
            std::vector<TestCoverage> results(tests.size());
            std::atomic<std::size_t> next{ 0 };
            const auto work = [&] {
                for (std::size_t index = next++; index < tests.size();
                     index = next++)
                {
                    results[index] = ReadTestCoverage(*tests[index].first,
                                                      *tests[index].second,
                                                      tools,
                                                      program);
                }
            };
            const std::size_t threads
                = std::min(std::max<std::size_t>(jobs, 1), tests.size());
            std::vector<std::thread> pool;
            for (std::size_t thread = 1; thread < threads; ++thread)
            {
                pool.emplace_back(work);
            }
            work();
            for (auto &thread : pool)
            {
                thread.join();
            }
            return results;
        }
    } // namespace

    CoverageReports
    WriteCoverageReports(const std::filesystem::path &directory,
                         const RunResult &run,
                         const std::string &program,
                         std::size_t jobs)
    {
        CoverageReports reports;
        std::vector<std::pair<const TestResult *, const Attempt *>> llvmTests;
        std::vector<std::pair<const TestResult *, const Attempt *>> gcovTests;
        for (const auto &test : run.tests)
        {
            if (const Attempt *attempt = ProfiledAttempt(test, ".profraw"))
            {
                llvmTests.emplace_back(&test, attempt);
            }
            if (const Attempt *attempt = ProfiledAttempt(test, ".gcov"))
            {
                gcovTests.emplace_back(&test, attempt);
            }
        }
        if (llvmTests.empty() && gcovTests.empty())
        {
            reports.notes.emplace_back(
                "--coverage: the tests wrote no coverage; build them with "
                "-fprofile-instr-generate -fcoverage-mapping (Clang) or "
                "--coverage (GCC)");
            return reports;
        }
        CoverageTools tools;
        tools.llvm = llvmTests.size() >= gcovTests.size();
        const auto &profiled = tools.llvm ? llvmTests : gcovTests;
        if (tools.llvm)
        {
            tools.profdata = FindTool("CATCH3_LLVM_PROFDATA", "llvm-profdata");
            tools.cov = FindTool("CATCH3_LLVM_COV", "llvm-cov");
        }
        else
        {
            tools.gcov = FindTool("CATCH3_GCOV", "gcov");
        }
        if (tools.llvm ? (!tools.profdata || !tools.cov) : !tools.gcov)
        {
            reports.notes.push_back(
                "--coverage: " + std::to_string(profiled.size())
                + " profile(s) in " + (directory / "profiles").string() + "; "
                + (tools.llvm ? "llvm-profdata and llvm-cov were" : "gcov was")
                + " not found, so there is no map of what each test reached");
            return reports;
        }

        CoverageMap combined;
        std::vector<std::string> raw;
        JsonValue tests = JsonValue::Array();
        for (auto &read : ReadAll(profiled, tools, program, jobs))
        {
            if (!read.reached)
            {
                reports.failures.push_back(read.test->name + ": "
                                           + read.failure);
                continue;
            }
            if (tools.llvm)
            {
                raw.push_back(read.profile);
            }
            else
            {
                Add(combined, *read.reached);
            }
            tests.Append(JsonValue::Object(
                { { "name", read.test->name },
                  { "outcome", OutcomeName(read.test->outcome) },
                  { "profile", read.profile },
                  { "files", ReachedJson(*read.reached) } }));
        }

        std::string lcovText;
        if (tools.llvm && !raw.empty())
        {
            std::vector<std::string> arguments{ "merge", "-sparse" };
            arguments.insert(arguments.end(), raw.begin(), raw.end());
            const std::string all = (directory / "coverage.profdata").string();
            arguments.emplace_back("-o");
            arguments.push_back(all);
            auto merged = RunTool(*tools.profdata, arguments);
            auto exported = merged ? RunTool(*tools.cov,
                                             { "export",
                                               "-format=lcov",
                                               "-instr-profile=" + all,
                                               program })
                                   : Expected<std::string>{ merged.GetError() };
            if (exported)
            {
                lcovText = *exported;
            }
            else
            {
                reports.failures.push_back("the combined report: "
                                           + exported.GetError().Message());
            }
        }
        else
        {
            lcovText = ToLcov(combined);
        }

        JsonValue report
            = JsonValue::Object({ { "version", 1 },
                                  { "format", tools.llvm ? "llvm" : "gcov" },
                                  { "binary", program },
                                  { "tests", std::move(tests) } });
        const auto json = directory / "coverage.json";
        const auto lcov = directory / "coverage.lcov";
        const auto jsonWritten
            = WriteFileAtomically(json, report.ToPrettyString());
        if (!jsonWritten)
        {
            reports.failures.push_back(jsonWritten.GetError().Message());
        }
        const auto lcovWritten = WriteFileAtomically(lcov, lcovText);
        if (!lcovWritten)
        {
            reports.failures.push_back(lcovWritten.GetError().Message());
        }
        std::string written;
        if (jsonWritten && lcovWritten)
        {
            written = "; what each test reached is in " + json.string()
                      + ", the run combined in " + lcov.string();
        }
        else if (jsonWritten)
        {
            written = "; what each test reached is in " + json.string();
        }
        else if (lcovWritten)
        {
            written = "; the run combined is in " + lcov.string();
        }
        reports.notes.push_back("--coverage: " + std::to_string(profiled.size())
                                + " test profile(s)" + written);
        for (auto &failure : reports.failures)
        {
            failure = "--coverage: " + failure;
        }
        return reports;
    }
} // namespace Progmasoft::Catch3::Runner
