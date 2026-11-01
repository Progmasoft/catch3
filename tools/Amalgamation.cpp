// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <algorithm>
#include <array>
#include <cctype>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "tools/Amalgamation.hpp"

namespace Progmasoft::Catch3::Tools
{
    namespace
    {
        namespace fs = std::filesystem;

        constexpr std::string_view kHeaderHead = R"(
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

)";

        constexpr std::string_view kHeaderTail
            = R"(//  ----------------------------------------------------------
//  This file is an amalgamation of multiple different files.
//  You probably shouldn't edit it directly.
//
//  The parts that come from src/Progmasoft, the Catch3 runner, keep
//  their own copyright and license lines (MPL-2.0 WITH
//  AdditionRef-Progmasoft-Exception-2.0). The runner needs C++17.
//  ----------------------------------------------------------
)";

        constexpr std::string_view kThirdPartyTail
            = R"(// ----------------------------------------------------------
// This file is an amalgamation of src/Progmasoft/Runner/Yyjson.inl,
// src/Progmasoft/PugiXml.inl and the libraries they include from
// third_party/. You probably shouldn't edit it directly.
//
// yyjson and pugixml keep their MIT licenses, stated in their text below;
// the rest is MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0. Only
// catch_amalgamated.cpp includes this file.
// ----------------------------------------------------------
)";

        // The comments at the top of each engine file.
        constexpr std::array<std::string_view, 5> kCopyrightLines{
            "//              Copyright Catch2 Authors\n",
            "// Distributed under the Boost Software License, Version 1.0.\n",
            "//   (See accompanying file LICENSE.txt or copy at\n",
            "//        https://www.boost.org/LICENSE_1_0.txt)\n",
            "// SPDX-License-Identifier: BSL-1.0\n",
        };

        constexpr std::string_view kUserConfig
            = "Progmasoft/Catch3/UserConfig.hpp";

        const std::map<std::string, std::string> &
        ThirdPartyFiles()
        {
            static const auto *files = new std::map<std::string, std::string>{
                { "yyjson.c", "yyjson/yyjson.c" },
                { "yyjson.h", "yyjson/yyjson.h" },
                { "pugixml.cpp", "pugixml/pugixml.cpp" },
                { "pugixml.hpp", "pugixml/pugixml.hpp" },
                { "pugiconfig.hpp", "pugixml/pugiconfig.hpp" },
            };
            return *files;
        }

        [[nodiscard]] bool
        Consists(std::string_view text, std::string_view extra)
        {
            return std::all_of(text.begin(), text.end(), [extra](char c) {
                return std::isalpha(static_cast<unsigned char>(c)) != 0
                       || extra.find(c) != std::string_view::npos;
            });
        }

        [[nodiscard]] bool
        IsPragmaOnce(std::string_view line)
        {
            return Trimmed(line) == "#pragma once";
        }

        /// A runner's include: `<Progmasoft/...>` or, inside one directory,
        /// `"Name.hpp"`.
        [[nodiscard]] std::optional<IncludedFile>
        ParseRunnerInclude(std::string_view line)
        {
            auto include = ParseInclude(line);
            if (!include)
            {
                return std::nullopt;
            }
            const bool found = include->Angled
                                   ? StartsWith(include->Name, "Progmasoft/")
                                   : EndsWith(include->Name, ".hpp")
                                         && Consists(include->Name, "/.");
            return found ? include : std::nullopt;
        }

        /// The runner compiles yyjson and pugixml through these files.
        [[nodiscard]] bool
        IsWrapperInclude(std::string_view line)
        {
            const auto include = ParseInclude(line);
            return include && !include->Angled
                   && EndsWith(include->Name, ".inl")
                   && Consists(include->Name, "./");
        }

        /// An include of yyjson and pugixml, which space them in many ways.
        [[nodiscard]] std::optional<std::string>
        ParseThirdPartyInclude(std::string_view line)
        {
            const auto skipSpace = [&line] {
                const auto first = line.find_first_not_of(" \t");
                line.remove_prefix(first == std::string_view::npos ? line.size()
                                                                   : first);
            };
            skipSpace();
            if (!StartsWith(line, "#"))
            {
                return std::nullopt;
            }
            line.remove_prefix(1);
            skipSpace();
            if (!StartsWith(line, "include"))
            {
                return std::nullopt;
            }
            line.remove_prefix(7);
            const auto before = line.size();
            skipSpace();
            if (line.size() == before || line.empty()
                || (line.front() != '<' && line.front() != '"'))
            {
                return std::nullopt;
            }
            line.remove_prefix(1);
            const auto end = line.find_first_of(">\"");
            if (end == std::string_view::npos || end == 0
                || !Consists(line.substr(0, end), "."))
            {
                return std::nullopt;
            }
            return std::string{ line.substr(0, end) };
        }

        class Generator final
        {
        public:
            Generator(const fs::path &repository, Problems &found)
                : root(repository)
                , source(repository / "src")
                , thirdParty(repository / "third_party")
                , problems(found)
                , upstream(UpstreamFiles(repository, found))
            {}

            [[nodiscard]] std::string
            Header()
            {
                std::string out = FileHeader();
                out += "#ifndef CATCH_AMALGAMATED_HPP_INCLUDED\n";
                out += "#define CATCH_AMALGAMATED_HPP_INCLUDED\n";
                Concatenate(out, source / "Progmasoft/Catch3/All.hpp", true);
                // Mocks are Progmasoft's own, and tests use them through
                // this header.
                Concatenate(out, source / "Progmasoft/Catch3/Mock.hpp", true);
                out += "#endif // CATCH_AMALGAMATED_HPP_INCLUDED\n";
                return out;
            }

            [[nodiscard]] std::string
            Source()
            {
                std::string out = FileHeader();
                out += "\n#include \"catch_amalgamated.hpp\"\n";
                Concatenate(
                    out,
                    source / "Progmasoft/Catch3/Internal/WindowsHProxy.hpp",
                    false);
                // The runner's headers come first: Catch::Session reaches
                // the runner.
                auto headers = FilesBelow(root,
                                          source / "Progmasoft/Runner",
                                          { ".hpp" },
                                          false);
                for (const char *name : { "Results.hpp",
                                          "Runner.hpp",
                                          "Version.hpp",
                                          "XmlWriter.hpp" })
                {
                    headers.push_back(std::string{ "src/Progmasoft/Catch3/" }
                                      + name);
                }
                for (const auto &header : headers)
                {
                    const auto path = Normal(root / header);
                    if (runnerHeaders.insert(path).second)
                    {
                        ConcatenateRunner(out, path, true);
                    }
                }
                for (const auto &file : upstream)
                {
                    if (EndsWith(file, ".cpp"))
                    {
                        Concatenate(out, root / file, false);
                    }
                }
                std::vector<std::string> files{
                    "src/Progmasoft/Mock.cpp",
                    "src/Progmasoft/Results.cpp",
                    "src/Progmasoft/XmlWriter.cpp"
                };
                for (auto &file : FilesBelow(root,
                                             source / "Progmasoft/Runner",
                                             { ".cpp" },
                                             false))
                {
                    files.push_back(std::move(file));
                }
                for (const auto &file : files)
                {
                    ConcatenateRunner(out, root / file, false);
                }
                return out;
            }

            [[nodiscard]] std::string
            ThirdParty()
            {
                std::string out
                    = "// Catch3 v" + Catch3Version()
                      + ": yyjson and pugixml for catch_amalgamated.cpp\n";
                out += kThirdPartyTail;
                out += "#pragma once\n";
                std::set<std::string> done;
                for (const char *wrapper :
                     { "Runner/Yyjson.inl", "PugiXml.inl" })
                {
                    const auto text
                        = ReadOrReport(source / "Progmasoft" / wrapper,
                                       problems);
                    for (const auto &line : SplitLines(text))
                    {
                        if (StartsWith(line, "// Copyright")
                            || StartsWith(line, "// SPDX-License-Identifier"))
                        {
                            continue;
                        }
                        const auto name = ParseThirdPartyInclude(line);
                        if (name && ThirdPartyFiles().count(*name) != 0)
                        {
                            ExpandThirdParty(out, *name, done);
                            continue;
                        }
                        out += line;
                    }
                }
                return out;
            }

        private:
            [[nodiscard]] static std::string
            Normal(const fs::path &path)
            {
                return path.lexically_normal().generic_string();
            }

            [[nodiscard]] std::string
            Catch3Version()
            {
                const auto text
                    = ReadOrReport(source / "Progmasoft/Catch3/Version.hpp",
                                   problems);
                constexpr std::string_view kKey = "kVersion = \"";
                const auto start = text.find(kKey);
                if (start == std::string::npos)
                {
                    problems.emplace_back("no kVersion in Version.hpp");
                    return {};
                }
                const auto first = start + kKey.size();
                return text.substr(first, text.find('"', first) - first);
            }

            [[nodiscard]] std::string
            FileHeader()
            {
                const auto engine = ParseEngineVersion(
                    ReadOrReport(source / "Progmasoft/Catch3/EngineVersion.cpp",
                                 problems));
                if (!engine)
                {
                    problems.emplace_back("no version in EngineVersion.cpp");
                }
                std::string out{ kHeaderHead };
                out += "//  Catch3 v" + Catch3Version() + ", on the Catch2 v"
                       + engine.value_or("") + " engine\n";
                out += kHeaderTail;
                return out;
            }

            /// The engine header a line includes, or nothing.
            [[nodiscard]] std::optional<std::string>
            EngineInclude(std::string_view line) const
            {
                const auto include = ParseInclude(line);
                if (!include || !include->Angled
                    || !StartsWith(include->Name, "Progmasoft/Catch3/"))
                {
                    return std::nullopt;
                }
                if (upstream.count("src/" + include->Name) != 0
                    || include->Name == kUserConfig)
                {
                    return include->Name;
                }
                return std::nullopt;
            }

            /// Copies a file, leaving out copyright lines; with
            /// `expandHeaders`, engine headers are put in place of their
            /// includes, once each.
            void
            Concatenate(std::string &out,
                        const fs::path &file,
                        bool expandHeaders)
            {
                const auto text = ReadOrReport(file, problems);
                for (const auto &line : SplitLines(text))
                {
                    if (IsPragmaOnce(line)
                        || std::find(kCopyrightLines.begin(),
                                     kCopyrightLines.end(),
                                     line)
                               != kCopyrightLines.end())
                    {
                        continue;
                    }
                    const auto engine = EngineInclude(line);
                    if (!engine)
                    {
                        // The runner's headers are expanded at the start of
                        // the .cpp file.
                        if (expandHeaders || !ParseRunnerInclude(line))
                        {
                            out += line;
                        }
                        continue;
                    }
                    if (!expandHeaders
                        || !concatenatedHeaders.insert(*engine).second)
                    {
                        continue;
                    }
                    // The generated configuration does not exist yet, and
                    // leaving it out means the default one.
                    if (*engine != kUserConfig)
                    {
                        Concatenate(out, source / *engine, true);
                    }
                }
            }

            /// Copies a file of the runner, expanding the runner's headers it
            /// includes once, or dropping those includes from .cpp files.
            void
            ConcatenateRunner(std::string &out,
                              const fs::path &file,
                              bool expandHeaders)
            {
                const auto directory = file.parent_path();
                const auto text = ReadOrReport(file, problems);
                for (const auto &line : SplitLines(text))
                {
                    if (IsPragmaOnce(line) || EngineInclude(line))
                    {
                        continue;
                    }
                    if (IsWrapperInclude(line))
                    {
                        if (!thirdPartyIncluded)
                        {
                            out += "#include "
                                   "\"catch_amalgamated_third_party.hpp\"\n";
                            thirdPartyIncluded = true;
                        }
                        continue;
                    }
                    const auto include = ParseRunnerInclude(line);
                    if (!include)
                    {
                        out += line;
                        continue;
                    }
                    const auto header = Normal(
                        (include->Angled ? source : directory) / include->Name);
                    if (expandHeaders && runnerHeaders.insert(header).second)
                    {
                        ConcatenateRunner(out, header, true);
                    }
                }
            }

            void
            ExpandThirdParty(std::string &out,
                             const std::string &name,
                             std::set<std::string> &done)
            {
                if (!done.insert(name).second)
                {
                    return;
                }
                const auto text
                    = ReadOrReport(thirdParty / ThirdPartyFiles().at(name),
                                   problems);
                for (const auto &line : SplitLines(text))
                {
                    auto included = ParseThirdPartyInclude(line);
                    // pugixml includes its source through a macro in
                    // header-only mode.
                    if (Trimmed(line) == "#\tinclude PUGIXML_SOURCE")
                    {
                        included = "pugixml.cpp";
                    }
                    if (included && ThirdPartyFiles().count(*included) != 0)
                    {
                        ExpandThirdParty(out, *included, done);
                    }
                    else
                    {
                        out += line;
                    }
                }
            }

            fs::path root;
            fs::path source;
            fs::path thirdParty;
            Problems &problems;
            std::set<std::string> upstream;
            std::set<std::string> concatenatedHeaders;
            std::set<std::string> runnerHeaders;
            bool thirdPartyIncluded = false;
        };

        [[nodiscard]] std::array<std::pair<const char *, const std::string *>,
                                 3>
        Parts(const Amalgamation &amalgamation)
        {
            return { { { "catch_amalgamated.hpp", &amalgamation.Header },
                       { "catch_amalgamated.cpp", &amalgamation.Source },
                       { "catch_amalgamated_third_party.hpp",
                         &amalgamation.ThirdParty } } };
        }
    } // namespace

    std::optional<IncludedFile>
    ParseInclude(std::string_view line)
    {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string_view::npos)
        {
            return std::nullopt;
        }
        line.remove_prefix(first);
        constexpr std::string_view kInclude = "#include ";
        if (!StartsWith(line, kInclude) || line.size() <= kInclude.size())
        {
            return std::nullopt;
        }
        line.remove_prefix(kInclude.size());
        const char open = line.front();
        if (open != '<' && open != '"')
        {
            return std::nullopt;
        }
        line.remove_prefix(1);
        const auto end = line.find(open == '<' ? '>' : '"');
        if (end == std::string_view::npos || end == 0)
        {
            return std::nullopt;
        }
        return IncludedFile{ std::string{ line.substr(0, end) }, open == '<' };
    }

    std::optional<std::string>
    ParseEngineVersion(std::string_view source)
    {
        constexpr std::string_view kKey = "static Version version";
        const auto key = source.find(kKey);
        if (key == std::string_view::npos)
        {
            return std::nullopt;
        }
        const auto open = source.find('(', key);
        const auto close = source.find(')', key);
        if (open == std::string_view::npos || close == std::string_view::npos)
        {
            return std::nullopt;
        }
        std::vector<std::string_view> parts;
        auto rest = source.substr(open + 1, close - open - 1);
        while (true)
        {
            const auto comma = rest.find(',');
            parts.push_back(Trimmed(rest.substr(0, comma)));
            if (comma == std::string_view::npos)
            {
                break;
            }
            rest.remove_prefix(comma + 1);
        }
        if (parts.size() != 5)
        {
            return std::nullopt;
        }
        std::string version{ parts[0] };
        version.append(".").append(parts[1]).append(".").append(parts[2]);
        auto branch = parts[3];
        if (branch.size() > 2)
        {
            branch = branch.substr(1, branch.size() - 2);
            version.append("-").append(branch).append(".").append(parts[4]);
        }
        return version;
    }

    Amalgamation
    GenerateAmalgamation(const std::filesystem::path &root, Problems &problems)
    {
        Generator generator{ root, problems };
        Amalgamation amalgamation;
        amalgamation.Header = generator.Header();
        amalgamation.Source = generator.Source();
        amalgamation.ThirdParty = generator.ThirdParty();
        return amalgamation;
    }

    void
    CheckAmalgamation(const std::filesystem::path &root,
                      const Amalgamation &expected,
                      Problems &problems)
    {
        for (const auto &[name, text] : Parts(expected))
        {
            if (ReadText(root / "extras" / name) != *text)
            {
                problems.push_back(std::string{ "extras/" } + name
                                   + " is out of date: run bazelisk run "
                                     "//tools:amalgamate");
            }
        }
    }

    void
    WriteAmalgamation(const std::filesystem::path &root,
                      const Amalgamation &amalgamation,
                      Problems &problems)
    {
        for (const auto &[name, text] : Parts(amalgamation))
        {
            if (!WriteText(root / "extras" / name, *text))
            {
                problems.push_back(std::string{ "cannot write extras/" }
                                   + name);
            }
        }
    }
} // namespace Progmasoft::Catch3::Tools
