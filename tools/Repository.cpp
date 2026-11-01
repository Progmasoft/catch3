// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <system_error>

#include "tools/Repository.hpp"

namespace Progmasoft::Catch3::Tools
{
    std::optional<std::string>
    ReadEnvironment(const char *name)
    {
#if defined(_WIN32)
        char *value = nullptr;
        std::size_t length = 0;
        if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
        {
            return std::nullopt;
        }
        std::string result{ value };
        std::free(value);
        return result;
#else
        const char *value = std::getenv(name);
        return value == nullptr ? std::nullopt
                                : std::optional<std::string>{ value };
#endif
    }

    std::optional<std::filesystem::path>
    RepositoryRoot()
    {
        const auto root = ReadEnvironment("BUILD_WORKSPACE_DIRECTORY");
        if (!root)
        {
            return std::nullopt;
        }
        return std::filesystem::path{ *root };
    }

    std::optional<std::string>
    ReadText(const std::filesystem::path &file)
    {
        std::ifstream stream{ file, std::ios::binary };
        if (!stream)
        {
            return std::nullopt;
        }
        const std::string bytes{ std::istreambuf_iterator<char>{ stream },
                                 std::istreambuf_iterator<char>{} };
        std::string text;
        text.reserve(bytes.size());
        for (std::size_t index = 0; index < bytes.size(); ++index)
        {
            if (bytes[index] != '\r' || index + 1 == bytes.size()
                || bytes[index + 1] != '\n')
            {
                text.push_back(bytes[index]);
            }
        }
        return text;
    }

    std::string
    ReadOrReport(const std::filesystem::path &file, Problems &problems)
    {
        auto text = ReadText(file);
        if (!text)
        {
            problems.push_back("cannot read " + file.generic_string());
            return {};
        }
        return *std::move(text);
    }

    bool
    WriteText(const std::filesystem::path &file, std::string_view text)
    {
        std::ofstream stream{ file, std::ios::binary | std::ios::trunc };
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        return static_cast<bool>(stream);
    }

    std::vector<std::string>
    SplitLines(std::string_view text)
    {
        std::vector<std::string> lines;
        std::size_t start = 0;
        while (start < text.size())
        {
            const auto end = text.find('\n', start);
            const auto next
                = end == std::string_view::npos ? text.size() : end + 1;
            lines.emplace_back(text.substr(start, next - start));
            start = next;
        }
        return lines;
    }

    std::string_view
    Trimmed(std::string_view text)
    {
        constexpr std::string_view kSpace = " \t\r\n";
        const auto first = text.find_first_not_of(kSpace);
        if (first == std::string_view::npos)
        {
            return {};
        }
        return text.substr(first, text.find_last_not_of(kSpace) - first + 1);
    }

    bool
    StartsWith(std::string_view text, std::string_view prefix)
    {
        return text.substr(0, prefix.size()) == prefix;
    }

    bool
    EndsWith(std::string_view text, std::string_view suffix)
    {
        return text.size() >= suffix.size()
               && text.substr(text.size() - suffix.size()) == suffix;
    }

    std::set<std::string>
    UpstreamFiles(const std::filesystem::path &root, Problems &problems)
    {
        std::set<std::string> files;
        const auto listing
            = ReadOrReport(root / "tools" / "upstreamFiles.txt", problems);
        for (const auto &line : SplitLines(listing))
        {
            const auto entry = Trimmed(line);
            if (!entry.empty() && entry.front() != '#')
            {
                files.emplace(entry);
            }
        }
        return files;
    }

    std::vector<std::string>
    FilesBelow(const std::filesystem::path &root,
               const std::filesystem::path &directory,
               const std::vector<std::string> &extensions,
               bool recursive)
    {
        namespace fs = std::filesystem;
        std::vector<std::string> files;
        const auto add = [&](const fs::directory_entry &entry) {
            const auto extension = entry.path().extension().string();
            if (entry.is_regular_file()
                && std::find(extensions.begin(), extensions.end(), extension)
                       != extensions.end())
            {
                files.push_back(
                    entry.path().lexically_relative(root).generic_string());
            }
        };
        std::error_code error;
        if (recursive)
        {
            for (const auto &entry :
                 fs::recursive_directory_iterator{ directory, error })
            {
                add(entry);
            }
        }
        else
        {
            for (const auto &entry : fs::directory_iterator{ directory, error })
            {
                add(entry);
            }
        }
        std::sort(files.begin(), files.end());
        return files;
    }

    int
    Finish(const Problems &problems)
    {
        for (const auto &problem : problems)
        {
            std::cerr << problem << '\n';
        }
        return problems.empty() ? 0 : 1;
    }
} // namespace Progmasoft::Catch3::Tools
