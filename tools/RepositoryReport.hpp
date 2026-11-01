// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Progmasoft::Catch3::Tools
{
    /// A program the checkout builds with, and where PATH finds it.
    struct ToolInfo final
    {
        std::string Name;
        std::string Path;
        bool Available = false;
    };

    /// The state of a checkout and of the machine around it. It holds no
    /// environment, remote URL or credential.
    struct RepositoryReport final
    {
        std::string Root;
        std::string Os;
        std::string Architecture;
        std::string Version;
        std::string EngineVersion;
        std::string Branch;
        std::string Revision;
        bool Dirty = false;
        std::string GitError;
        std::vector<ToolInfo> Tools;
    };

    /// Reads the checkout at `root`, without changing anything.
    [[nodiscard]] RepositoryReport
    CollectRepositoryReport(const std::filesystem::path &root);

    /// The report as lines for a person.
    [[nodiscard]] std::string
    FormatReport(const RepositoryReport &report);

    /// The report as JSON: empty texts are left out, the rest has the names
    /// of Tala's repo-info, in camelCase.
    [[nodiscard]] std::string
    ReportToJson(const RepositoryReport &report);
} // namespace Progmasoft::Catch3::Tools
