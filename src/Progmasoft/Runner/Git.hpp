// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include "Expected.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace Progmasoft::Catch3::Runner
{
    /// What Git says about the checkout the tests come from.
    struct Changes final
    {
        std::filesystem::path root;
        std::string commit;
        /// Changed since the revision, staged or not, and untracked files;
        /// absolute paths.
        std::vector<std::filesystem::path> files;
    };

    /// The changes since `revision` (HEAD when empty) in the checkout that
    /// holds `directory`. An error when Git or the checkout is missing.
    [[nodiscard]] Expected<Changes>
    ReadChanges(const std::filesystem::path &directory,
                const std::string &revision);

    /// The commit checked out in the checkout that holds `directory`, or
    /// empty.
    [[nodiscard]] std::string
    CurrentCommit(const std::filesystem::path &directory);

    /// Whether two spellings name the same source file: equal after
    /// normalisation, or one a relative path the other ends with.
    [[nodiscard]] bool
    SameSourceFile(const std::filesystem::path &left,
                   const std::filesystem::path &right);
} // namespace Progmasoft::Catch3::Runner
