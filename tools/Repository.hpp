// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace Progmasoft::Catch3::Tools
{
    /// What a tool found wrong, one line each.
    using Problems = std::vector<std::string>;

    [[nodiscard]] std::optional<std::string>
    ReadEnvironment(const char *name);

    /// The checkout `bazelisk run` was started in, if it was.
    [[nodiscard]] std::optional<std::filesystem::path>
    RepositoryRoot();

    /// The text of a file with LF line ends, whatever the checkout uses.
    [[nodiscard]] std::optional<std::string>
    ReadText(const std::filesystem::path &file);

    /// Like `ReadText`, with a line in `problems` when it cannot be read.
    [[nodiscard]] std::string
    ReadOrReport(const std::filesystem::path &file, Problems &problems);

    [[nodiscard]] bool
    WriteText(const std::filesystem::path &file, std::string_view text);

    /// The lines of `text`, each with its line end.
    [[nodiscard]] std::vector<std::string>
    SplitLines(std::string_view text);

    [[nodiscard]] std::string_view
    Trimmed(std::string_view text);

    [[nodiscard]] bool
    StartsWith(std::string_view text, std::string_view prefix);

    [[nodiscard]] bool
    EndsWith(std::string_view text, std::string_view suffix);

    /// The files of src/Progmasoft that come from Catch2 and keep its
    /// license, relative to the repository, from tools/upstreamFiles.txt.
    [[nodiscard]] std::set<std::string>
    UpstreamFiles(const std::filesystem::path &root, Problems &problems);

    /// The files with one of `extensions` below `directory`, sorted, as paths
    /// relative to `root` with forward slashes.
    [[nodiscard]] std::vector<std::string>
    FilesBelow(const std::filesystem::path &root,
               const std::filesystem::path &directory,
               const std::vector<std::string> &extensions,
               bool recursive);

    /// Writes `problems` to standard error; returns the exit code of a tool.
    int
    Finish(const Problems &problems);
} // namespace Progmasoft::Catch3::Tools
