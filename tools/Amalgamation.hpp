// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "tools/Repository.hpp"

namespace Progmasoft::Catch3::Tools
{
    /// The single-file distribution in extras/.
    struct Amalgamation final
    {
        std::string Header;
        std::string Source;
        std::string ThirdParty;
    };

    /// A file `#include` names at the start of a line.
    struct IncludedFile final
    {
        std::string Name;
        bool Angled = false;
    };

    /// Reads `#include <name>` or `#include "name"`, written exactly so.
    [[nodiscard]] std::optional<IncludedFile>
    ParseInclude(std::string_view line);

    /// "3.16.1", or "3.17.0-preview.1", from the source of EngineVersion.cpp.
    [[nodiscard]] std::optional<std::string>
    ParseEngineVersion(std::string_view source);

    /// Builds the distribution from the sources; deterministic, so that it
    /// can be compared with what is in extras/.
    [[nodiscard]] Amalgamation
    GenerateAmalgamation(const std::filesystem::path &root, Problems &problems);

    /// Adds a line to `problems` for each file of extras/ that `expected`
    /// does not match.
    void
    CheckAmalgamation(const std::filesystem::path &root,
                      const Amalgamation &expected,
                      Problems &problems);

    /// Writes `amalgamation` to extras/.
    void
    WriteAmalgamation(const std::filesystem::path &root,
                      const Amalgamation &amalgamation,
                      Problems &problems);
} // namespace Progmasoft::Catch3::Tools
