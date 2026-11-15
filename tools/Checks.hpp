// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <filesystem>

#include "tools/Repository.hpp"

namespace Progmasoft::Catch3::Tools
{
    /// Every source file starts with the license of its part: engine files
    /// keep Catch2's, the rest carry Progmasoft's.
    void
    CheckLicenses(const std::filesystem::path &root, Problems &problems);

    /// Every `All.hpp` of the engine includes the engine headers of its folder
    /// and nothing else, sorted and once each.
    void
    CheckConvenienceHeaders(const std::filesystem::path &root,
                            Problems &problems);

    /// Every header of the library has `#pragma once`.
    void
    CheckPragmaOnce(const std::filesystem::path &root, Problems &problems);

    /// No two sources of the library share a file name: build tools that keep
    /// object files in one directory would compile one over the other.
    void
    CheckDuplicateFilenames(const std::filesystem::path &root,
                            Problems &problems);
} // namespace Progmasoft::Catch3::Tools
