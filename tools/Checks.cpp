// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "tools/Amalgamation.hpp"
#include "tools/Checks.hpp"

namespace Progmasoft::Catch3::Tools
{
    namespace
    {
        namespace fs = std::filesystem;

        constexpr std::string_view kCatch2License = R"(
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
)";

        constexpr std::string_view kProgmasoftLicense
            = R"(// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
)";

        [[nodiscard]] bool
        IsInternalFolder(std::string_view name)
        {
            return name == "Detail" || name == "Internal";
        }

        class ConvenienceHeaders final
        {
        public:
            ConvenienceHeaders(const fs::path &repository, Problems &found)
                : root(repository)
                , problems(found)
            {
                for (const auto &file : UpstreamFiles(repository, found))
                {
                    if (EndsWith(file, ".hpp"))
                    {
                        headers.insert(file.substr(4));
                    }
                }
            }

            void
            Walk(const std::string &folder)
            {
                Verify(folder);
                for (const auto &name : Subfolders(folder))
                {
                    if (!IsInternalFolder(name))
                    {
                        Walk(folder + "/" + name);
                    }
                }
            }

        private:
            [[nodiscard]] std::vector<std::string>
            HeadersIn(const std::string &folder) const
            {
                const auto prefix = folder + "/";
                std::vector<std::string> found;
                for (const auto &header : headers)
                {
                    if (StartsWith(header, prefix)
                        && header.find('/', prefix.size()) == std::string::npos)
                    {
                        found.push_back(header);
                    }
                }
                return found;
            }

            [[nodiscard]] std::set<std::string>
            Subfolders(const std::string &folder) const
            {
                const auto prefix = folder + "/";
                std::set<std::string> names;
                for (const auto &header : headers)
                {
                    const auto slash = header.find('/', prefix.size());
                    if (StartsWith(header, prefix)
                        && slash != std::string::npos)
                    {
                        names.insert(header.substr(prefix.size(),
                                                   slash - prefix.size()));
                    }
                }
                return names;
            }

            [[nodiscard]] std::vector<std::string>
            Wanted(const std::string &folder) const
            {
                auto wanted = HeadersIn(folder);
                for (const auto &name : Subfolders(folder))
                {
                    const auto subfolder = folder + "/" + name;
                    if (IsInternalFolder(name))
                    {
                        const auto inner = HeadersIn(subfolder);
                        wanted.insert(wanted.end(), inner.begin(), inner.end());
                    }
                    else
                    {
                        wanted.push_back(subfolder + "/All.hpp");
                    }
                }
                std::sort(wanted.begin(), wanted.end());
                return wanted;
            }

            void
            Verify(const std::string &folder)
            {
                const auto name = folder + "/All.hpp";
                if (headers.count(name) == 0)
                {
                    problems.push_back("missing convenience header: " + name);
                    return;
                }
                std::vector<std::string> includes;
                const auto text = ReadOrReport(root / "src" / name, problems);
                for (const auto &line : SplitLines(text))
                {
                    const auto include = ParseInclude(line);
                    if (StartsWith(line, "#include") && include
                        && include->Angled
                        && StartsWith(include->Name, "Progmasoft/Catch3/")
                        && EndsWith(include->Name, ".hpp"))
                    {
                        includes.push_back(include->Name);
                    }
                }
                if (!std::is_sorted(includes.begin(), includes.end()))
                {
                    problems.push_back(name + ": includes are not sorted");
                }
                std::set<std::string> seen;
                for (const auto &include : includes)
                {
                    if (!seen.insert(include).second)
                    {
                        problems.push_back(name + ": duplicated include "
                                           + include);
                    }
                }
                std::set<std::string> wanted;
                for (auto &include : Wanted(folder))
                {
                    if (include != name)
                    {
                        wanted.insert(std::move(include));
                    }
                }
                for (const auto &include : wanted)
                {
                    if (seen.count(include) == 0
                        && include
                               != "Progmasoft/Catch3/Internal/"
                                  "WindowsHProxy.hpp")
                    {
                        problems.push_back(name + ": missing include "
                                           + include);
                    }
                }
                for (const auto &include : seen)
                {
                    if (wanted.count(include) == 0)
                    {
                        problems.push_back(name + ": superfluous include "
                                           + include);
                    }
                }
            }

            fs::path root;
            Problems &problems;
            std::set<std::string> headers;
        };
    } // namespace

    void
    CheckLicenses(const fs::path &root, Problems &problems)
    {
        const auto upstream = UpstreamFiles(root, problems);
        for (const char *directory :
             { "src/Progmasoft", "tests", "examples", "fuzzing", "tools" })
        {
            for (const auto &file :
                 FilesBelow(root, root / directory, { ".cpp", ".hpp" }, true))
            {
                const bool progmasoft
                    = StartsWith(file, "tools/")
                      || (file.find("Progmasoft/") != std::string::npos
                          && upstream.count(file) == 0);
                const auto expected
                    = progmasoft ? kProgmasoftLicense : kCatch2License;
                if (!StartsWith(ReadOrReport(root / file, problems), expected))
                {
                    problems.push_back(file + " does not have its license");
                }
            }
        }
    }

    void
    CheckConvenienceHeaders(const fs::path &root, Problems &problems)
    {
        ConvenienceHeaders{ root, problems }.Walk("Progmasoft/Catch3");
    }

    void
    CheckPragmaOnce(const fs::path &root, Problems &problems)
    {
        for (const auto &file :
             FilesBelow(root, root / "src/Progmasoft", { ".hpp" }, true))
        {
            const auto text = ReadOrReport(root / file, problems);
            if (!StartsWith(text, "#pragma once\n")
                && text.find("\n#pragma once\n") == std::string::npos)
            {
                problems.push_back(file + " has no #pragma once");
            }
        }
    }

    void
    CheckDuplicateFilenames(const fs::path &root, Problems &problems)
    {
        std::set<std::string> names;
        for (const auto &file :
             FilesBelow(root, root / "src/Progmasoft", { ".cpp" }, true))
        {
            const auto name = fs::path{ file }.filename().string();
            if (!names.insert(name).second)
            {
                problems.push_back(file
                                   + " shares its name with another source");
            }
        }
    }
} // namespace Progmasoft::Catch3::Tools
