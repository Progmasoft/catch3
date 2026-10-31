// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include "Git.hpp"

#include "Process.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        Expected<std::string>
        Git(const std::filesystem::path &directory,
            std::vector<std::string> arguments)
        {
            ChildOptions options;
            options.arguments.emplace_back("git");
            options.arguments.insert(
                options.arguments.end(), arguments.begin(), arguments.end());
            options.workingDirectory = directory;
            options.searchPath = true;
            auto run = RunAndCapture(options);
            if (!run)
            {
                return run.GetError();
            }
            if (!run->status.Succeeded())
            {
                std::string message = run->errors;
                while (!message.empty() &&
                       (message.back() == '\n' || message.back() == '\r'))
                {
                    message.pop_back();
                }
                return Error{"git " + arguments.front() + " failed: " +
                             message};
            }
            return run->output;
        }

        std::vector<std::string>
        Lines(const std::string &text)
        {
            std::vector<std::string> lines;
            std::size_t start = 0;
            while (start < text.size())
            {
                auto end = text.find('\n', start);
                if (end == std::string::npos)
                {
                    end = text.size();
                }
                std::string line = text.substr(start, end - start);
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                if (!line.empty())
                {
                    lines.push_back(std::move(line));
                }
                start = end + 1;
            }
            return lines;
        }

        std::string
        Normalised(const std::filesystem::path &path)
        {
            std::string text = path.lexically_normal().generic_string();
#if defined(_WIN32)
            // Windows paths compare without case.
            std::transform(text.begin(), text.end(), text.begin(), [](char c) {
                return static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            });
#endif
            return text;
        }
    } // namespace

    Expected<Changes>
    ReadChanges(const std::filesystem::path &directory,
                const std::string &revision)
    {
        auto root = Git(directory, {"rev-parse", "--show-toplevel"});
        if (!root)
        {
            return root.GetError();
        }
        Changes changes;
        const auto rootLines = Lines(*root);
        if (rootLines.empty())
        {
            return Error{"git did not name the top of the checkout"};
        }
        changes.root = std::filesystem::path{rootLines.front()};
        changes.commit = CurrentCommit(directory);
        const std::string base = revision.empty() ? "HEAD" : revision;
        auto changed =
            Git(changes.root, {"diff", "--name-only", "--no-renames", base});
        if (!changed)
        {
            return changed.GetError();
        }
        auto untracked = Git(changes.root,
                             {"ls-files", "--others", "--exclude-standard"});
        for (const auto &line : Lines(*changed))
        {
            changes.files.push_back(changes.root / line);
        }
        if (untracked)
        {
            for (const auto &line : Lines(*untracked))
            {
                changes.files.push_back(changes.root / line);
            }
        }
        return changes;
    }

    std::string
    CurrentCommit(const std::filesystem::path &directory)
    {
        auto commit = Git(directory, {"rev-parse", "HEAD"});
        if (!commit)
        {
            return {};
        }
        const auto lines = Lines(*commit);
        return lines.empty() ? std::string{} : lines.front();
    }

    bool
    SameSourceFile(const std::filesystem::path &left,
                   const std::filesystem::path &right)
    {
        const std::string first = Normalised(left);
        const std::string second = Normalised(right);
        if (first == second)
        {
            return true;
        }
        const auto endsWith = [](const std::string &text,
                                 const std::string &suffix) {
            if (suffix.empty() || suffix.size() >= text.size())
            {
                return false;
            }
            return text.compare(text.size() - suffix.size(),
                                suffix.size(),
                                suffix) == 0 &&
                   text[text.size() - suffix.size() - 1] == '/';
        };
        // __FILE__ is often relative to the build directory.
        std::string relativeFirst = first;
        std::string relativeSecond = second;
        while (relativeFirst.rfind("../", 0) == 0)
        {
            relativeFirst.erase(0, 3);
        }
        while (relativeSecond.rfind("../", 0) == 0)
        {
            relativeSecond.erase(0, 3);
        }
        return (!left.is_absolute() && endsWith(second, relativeFirst)) ||
               (!right.is_absolute() && endsWith(first, relativeSecond));
    }
} // namespace Progmasoft::Catch3::Runner
