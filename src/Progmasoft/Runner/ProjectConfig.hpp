// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Expected.hpp"
#include "Options.hpp"

namespace Progmasoft::Catch3::Runner
{
    /// The name of the project configuration file.
    inline constexpr std::string_view kProjectConfigName = "catch3.json";

    /// The `catch3.json` that applies in `start`: the nearest one in `start`
    /// or a directory above it, up to the root of the Git repository that
    /// holds `start`. Outside a repository only `start` itself is searched,
    /// so a stray file in a home directory never applies.
    [[nodiscard]] std::optional<std::filesystem::path>
    FindProjectConfig(const std::filesystem::path &start);

    /// Applies the text of a `catch3.json` to `options`. Its keys are the long
    /// options of the command line, and its values go through the same checks;
    /// relative paths are read from `directory`, the file's own. `file` names
    /// the file in messages.
    [[nodiscard]] Expected<Options>
    ApplyProjectConfig(std::string_view text,
                       const std::filesystem::path &directory,
                       std::string_view file,
                       Options options = {});

    /// Reads the command line together with the `catch3.json` it selects:
    /// --config names one, --no-config reads none, and otherwise
    /// FindProjectConfig searches from `workingDirectory`. Options on the
    /// command line win over the file, and test specs on the command line
    /// replace those of the file. Workers, --help and --version read no file.
    [[nodiscard]] Expected<Options>
    ParseCommandLine(const std::vector<std::string> &arguments,
                     const std::filesystem::path &workingDirectory);
} // namespace Progmasoft::Catch3::Runner
