// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <algorithm>
#include <cctype>
#include <charconv>
#include <utility>
#include <vector>

#include "tools/Versions.hpp"

namespace Progmasoft::Catch3::Tools
{
    namespace
    {
        constexpr std::string_view kVersionHeader
            = "src/Progmasoft/Catch3/Version.hpp";
    } // namespace

    std::optional<Version>
    ParseVersion(std::string_view text)
    {
        Version version{};
        std::size_t count = 0;
        while (true)
        {
            const auto dot = text.find('.');
            const auto number = text.substr(0, dot);
            if (count == version.size() || number.empty()
                || (number.size() > 1 && number.front() == '0'))
            {
                return std::nullopt;
            }
            const auto *end = number.data() + number.size();
            const auto parsed
                = std::from_chars(number.data(), end, version[count]);
            if (parsed.ec != std::errc{} || parsed.ptr != end)
            {
                return std::nullopt;
            }
            ++count;
            if (dot == std::string_view::npos)
            {
                break;
            }
            text.remove_prefix(dot + 1);
        }
        if (count < 3)
        {
            return std::nullopt;
        }
        return version;
    }

    std::string
    FormatVersion(const Version &version)
    {
        return std::to_string(version[0]) + "." + std::to_string(version[1])
               + "." + std::to_string(version[2]) + "."
               + std::to_string(version[3]);
    }

    Version
    NextVersion(Version version)
    {
        if (version[3] < 9)
        {
            ++version[3];
            return version;
        }
        version[3] = 0;
        if (version[2] < 9)
        {
            ++version[2];
            return version;
        }
        version[2] = 0;
        if (version[1] < 19)
        {
            ++version[1];
            return version;
        }
        version[1] = 0;
        ++version[0];
        return version;
    }

    const std::array<VersionSite, 5> &
    VersionSites()
    {
        static constexpr std::array<VersionSite, 5> kSites{ {
            { "CMakeLists.txt", "\n  VERSION ", "\n" },
            { "MODULE.bazel", "module(name = \"catch3\", version = \"", "\"" },
            { "meson.build", "\n  version: '", "'" },
            { "xmake.lua", "\nset_version(\"", "\"" },
            { kVersionHeader, "kVersion = \"", "\"" },
        } };
        return kSites;
    }

    std::optional<std::string>
    ReadVersionSite(std::string_view text, const VersionSite &site)
    {
        const auto before = text.find(site.Before);
        if (before == std::string_view::npos)
        {
            return std::nullopt;
        }
        const auto first = before + site.Before.size();
        const auto end = text.find(site.After, first);
        if (end == std::string_view::npos)
        {
            return std::nullopt;
        }
        return std::string{ text.substr(first, end - first) };
    }

    std::optional<std::string>
    WriteVersionSite(std::string_view text,
                     const VersionSite &site,
                     std::string_view version)
    {
        const auto current = ReadVersionSite(text, site);
        if (!current)
        {
            return std::nullopt;
        }
        const auto first = text.find(site.Before) + site.Before.size();
        std::string result{ text.substr(0, first) };
        result += version;
        result += text.substr(first + current->size());
        return result;
    }

    std::optional<std::string>
    CurrentVersion(const std::filesystem::path &root, Problems &problems)
    {
        const auto &sites = VersionSites();
        const auto &site
            = *std::find_if(sites.begin(),
                            sites.end(),
                            [](const VersionSite &candidate) {
                                return candidate.File == kVersionHeader;
                            });
        auto version
            = ReadVersionSite(ReadOrReport(root / site.File, problems), site);
        if (!version)
        {
            problems.emplace_back("no version in Version.hpp");
        }
        return version;
    }

    void
    CheckVersions(const std::filesystem::path &root, Problems &problems)
    {
        const auto current = CurrentVersion(root, problems);
        if (!current)
        {
            return;
        }
        for (const auto &site : VersionSites())
        {
            const auto version
                = ReadVersionSite(ReadOrReport(root / site.File, problems),
                                  site);
            if (version != current)
            {
                problems.push_back(std::string{ site.File } + " states "
                                   + version.value_or("no version")
                                   + ", Version.hpp states " + *current);
            }
        }
    }

    void
    SetVersion(const std::filesystem::path &root,
               const std::string &version,
               Problems &problems)
    {
        std::vector<std::pair<std::filesystem::path, std::string>> writes;
        for (const auto &site : VersionSites())
        {
            const auto path = root / site.File;
            auto text
                = WriteVersionSite(ReadOrReport(path, problems), site, version);
            if (text)
            {
                writes.emplace_back(path, *std::move(text));
            }
            else
            {
                problems.push_back("cannot set the version in "
                                   + std::string{ site.File });
            }
        }
        // All of them or none, so that the places never disagree.
        if (!problems.empty())
        {
            return;
        }
        for (const auto &[path, text] : writes)
        {
            if (!WriteText(path, text))
            {
                problems.push_back("cannot write " + path.generic_string());
            }
        }
    }

    std::optional<std::string>
    ReleaseNotesOf(std::string_view notes, std::string_view version)
    {
        auto heading = "\n## " + std::string{ version } + "\n";
        auto found = notes.find(heading);
        // A release with D = 0 may be headed by its three numbers.
        const auto parsed = ParseVersion(version);
        if (found == std::string_view::npos && parsed && (*parsed)[3] == 0)
        {
            heading = "\n## " + std::to_string((*parsed)[0]) + "."
                      + std::to_string((*parsed)[1]) + "."
                      + std::to_string((*parsed)[2]) + "\n";
            found = notes.find(heading);
        }
        if (found == std::string_view::npos)
        {
            return std::nullopt;
        }
        auto section = notes.substr(found + heading.size());
        section = section.substr(0, section.find("\n## "));
        return std::string{ Trimmed(section) } + "\n";
    }
} // namespace Progmasoft::Catch3::Tools
