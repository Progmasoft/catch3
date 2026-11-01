// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "tools/Repository.hpp"

namespace Progmasoft::Catch3::Tools
{
    /// A.B.C.D. A version of three numbers has D = 0.
    using Version = std::array<unsigned, 4>;

    [[nodiscard]] std::optional<Version>
    ParseVersion(std::string_view text);

    [[nodiscard]] std::string
    FormatVersion(const Version &version);

    /// The version after `version`, which counts releases: D up to 9, then C
    /// up to 9, then B up to 19, then A.
    [[nodiscard]] Version
    NextVersion(Version version);

    /// A place that states the version: the file, and the text around it.
    struct VersionSite final
    {
        std::string_view File;
        std::string_view Before;
        std::string_view After;
    };

    [[nodiscard]] const std::array<VersionSite, 5> &
    VersionSites();

    /// The version `text`, a file of `site`, states.
    [[nodiscard]] std::optional<std::string>
    ReadVersionSite(std::string_view text, const VersionSite &site);

    /// `text` with `version` in the place of `site`.
    [[nodiscard]] std::optional<std::string>
    WriteVersionSite(std::string_view text,
                     const VersionSite &site,
                     std::string_view version);

    /// The version in Version.hpp, the one the other places have to follow.
    [[nodiscard]] std::optional<std::string>
    CurrentVersion(const std::filesystem::path &root, Problems &problems);

    /// Every place states the same version.
    void
    CheckVersions(const std::filesystem::path &root, Problems &problems);

    /// Writes `version` to every place.
    void
    SetVersion(const std::filesystem::path &root,
               const std::string &version,
               Problems &problems);

    /// The text of the section of `version` in release-notes.md, or nothing.
    [[nodiscard]] std::optional<std::string>
    ReleaseNotesOf(std::string_view notes, std::string_view version);
} // namespace Progmasoft::Catch3::Tools
