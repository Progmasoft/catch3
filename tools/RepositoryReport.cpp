// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>

#include "tools/Amalgamation.hpp"
#include "tools/Repository.hpp"
#include "tools/RepositoryReport.hpp"
#include "tools/Versions.hpp"

// yyjson, compiled here and nowhere else in the tools, like in the runner.
#define yyjson_api static
#include <yyjson.c>

namespace Progmasoft::Catch3::Tools
{
    namespace
    {
        namespace fs = std::filesystem;

#if defined(_WIN32)
        constexpr char kPathSeparator = ';';
        constexpr std::array<std::string_view, 4> kExtensions{ ".exe",
                                                               ".cmd",
                                                               ".bat",
                                                               ".com" };
        constexpr std::string_view kNoErrors = " 2>nul";
#else
        constexpr char kPathSeparator = ':';
        constexpr std::array<std::string_view, 1> kExtensions{ "" };
        constexpr std::string_view kNoErrors = " 2>/dev/null";
#endif

        constexpr std::array<std::string_view, 11> kTools{
            "git",      "bazelisk",     "cmake",    "ninja",
            "meson",    "xmake",        "premake5", "clang",
            "clang-cl", "clang-format", "gcc",
        };

        constexpr std::string_view
        OperatingSystem()
        {
#if defined(_WIN32)
            return "windows";
#elif defined(__APPLE__)
            return "darwin";
#elif defined(__linux__)
            return "linux";
#else
            return "unknown";
#endif
        }

        constexpr std::string_view
        Architecture()
        {
#if defined(_M_X64) || defined(__x86_64__)
            return "amd64";
#elif defined(_M_ARM64) || defined(__aarch64__)
            return "arm64";
#else
            return "unknown";
#endif
        }

        [[nodiscard]] ToolInfo
        Find(std::string_view name, std::string_view searchPath)
        {
            ToolInfo tool;
            tool.Name = name;
            while (!tool.Available && !searchPath.empty())
            {
                const auto end = searchPath.find(kPathSeparator);
                const fs::path directory{ std::string{
                    searchPath.substr(0, end) } };
                searchPath.remove_prefix(end == std::string_view::npos
                                             ? searchPath.size()
                                             : end + 1);
                if (directory.empty())
                {
                    continue;
                }
                for (const auto extension : kExtensions)
                {
                    const auto file
                        = directory
                          / (std::string{ name } + std::string{ extension });
                    std::error_code error;
                    if (fs::is_regular_file(file, error))
                    {
                        tool.Path = file.string();
                        tool.Available = true;
                        break;
                    }
                }
            }
            return tool;
        }

        /// What `git <arguments>` writes in `root`, or nothing when it fails.
        [[nodiscard]] std::optional<std::string>
        Git(const fs::path &root, std::string_view arguments)
        {
            const auto command = "git -C \"" + root.string() + "\" "
                                 + std::string{ arguments }
                                 + std::string{ kNoErrors };
#if defined(_WIN32)
            auto *pipe = _popen(command.c_str(), "r");
#else
            auto *pipe = popen(command.c_str(), "r");
#endif
            if (pipe == nullptr)
            {
                return std::nullopt;
            }
            std::string output;
            std::array<char, 512> buffer{};
            std::size_t count = 0;
            while ((count = std::fread(buffer.data(), 1, buffer.size(), pipe))
                   != 0)
            {
                output.append(buffer.data(), count);
            }
#if defined(_WIN32)
            const int status = _pclose(pipe);
#else
            const int status = pclose(pipe);
#endif
            if (status != 0)
            {
                return std::nullopt;
            }
            return std::string{ Trimmed(output) };
        }

        void
        AddText(yyjson_mut_doc *document,
                yyjson_mut_val *object,
                const char *key,
                const std::string &value)
        {
            if (!value.empty())
            {
                yyjson_mut_obj_add_strn(document,
                                        object,
                                        key,
                                        value.data(),
                                        value.size());
            }
        }

        struct FreeDocument final
        {
            void
            operator()(yyjson_mut_doc *document) const
            {
                yyjson_mut_doc_free(document);
            }
        };

        struct FreeText final
        {
            void
            operator()(char *text) const
            {
                std::free(text);
            }
        };
    } // namespace

    RepositoryReport
    CollectRepositoryReport(const fs::path &root)
    {
        RepositoryReport report;
        report.Root = root.string();
        report.Os = OperatingSystem();
        report.Architecture = Architecture();

        Problems ignored;
        report.Version = CurrentVersion(root, ignored).value_or("");
        report.EngineVersion
            = ParseEngineVersion(
                  ReadText(root / "src/Progmasoft/Catch3/EngineVersion.cpp")
                      .value_or(""))
                  .value_or("");

        const auto searchPath = ReadEnvironment("PATH").value_or("");
        for (const auto name : kTools)
        {
            report.Tools.push_back(Find(name, searchPath));
        }

        const auto branch = Git(root, "branch --show-current");
        const auto revision = Git(root, "rev-parse HEAD");
        const auto status
            = Git(root, "status --porcelain --untracked-files=no");
        if (branch && revision && status)
        {
            report.Branch = *branch;
            report.Revision = *revision;
            report.Dirty = !status->empty();
        }
        else
        {
            report.GitError = "Git checkout metadata could not be read";
        }
        return report;
    }

    std::string
    FormatReport(const RepositoryReport &report)
    {
        std::string text = "Checkout: " + report.Root + "\n";
        text += "Host: " + report.Os + "/" + report.Architecture + "\n";
        text += "Catch3: " + report.Version + ", on the Catch2 "
                + report.EngineVersion + " engine\n";
        text += "Branch: " + report.Branch + "\n";
        text += "Revision: " + report.Revision + "\n";
        text += std::string{ "Tracked changes: " }
                + (report.Dirty ? "true" : "false") + "\n";
        if (!report.GitError.empty())
        {
            text += report.GitError + "\n";
        }
        for (const auto &tool : report.Tools)
        {
            auto name = tool.Name;
            name.resize(std::max<std::size_t>(name.size(), 13), ' ');
            text += name + (tool.Available ? tool.Path : "not found") + "\n";
        }
        return text;
    }

    std::string
    ReportToJson(const RepositoryReport &report)
    {
        const std::unique_ptr<yyjson_mut_doc, FreeDocument> holder{
            yyjson_mut_doc_new(nullptr)
        };
        auto *document = holder.get();
        auto *root = yyjson_mut_obj(document);
        yyjson_mut_doc_set_root(document, root);
        AddText(document, root, "root", report.Root);
        AddText(document, root, "os", report.Os);
        AddText(document, root, "architecture", report.Architecture);
        AddText(document, root, "version", report.Version);
        AddText(document, root, "engineVersion", report.EngineVersion);
        AddText(document, root, "branch", report.Branch);
        AddText(document, root, "revision", report.Revision);
        yyjson_mut_obj_add_bool(document, root, "dirty", report.Dirty);
        AddText(document, root, "gitError", report.GitError);
        auto *tools = yyjson_mut_obj_add_arr(document, root, "tools");
        for (const auto &tool : report.Tools)
        {
            auto *entry = yyjson_mut_arr_add_obj(document, tools);
            AddText(document, entry, "name", tool.Name);
            AddText(document, entry, "path", tool.Path);
            yyjson_mut_obj_add_bool(document,
                                    entry,
                                    "available",
                                    tool.Available);
        }
        const std::unique_ptr<char, FreeText> text{
            yyjson_mut_write(document, YYJSON_WRITE_PRETTY_TWO_SPACES, nullptr)
        };
        return text == nullptr ? std::string{}
                               : std::string{ text.get() } + "\n";
    }
} // namespace Progmasoft::Catch3::Tools
