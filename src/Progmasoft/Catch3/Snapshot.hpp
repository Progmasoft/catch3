// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace Progmasoft::Catch3 {

    /// Updates are deliberately explicit so normal test runs never rewrite
    /// files.
    enum class SnapshotUpdateMode : unsigned char {
        kNever,
        kCreateMissing,
        kAlways,
    };

    enum class SnapshotStatus : unsigned char {
        kMatched,
        kCreated,
        kUpdated,
        kMismatched,
        kInvalidName,
        kInvalidOptions,
        kIoError,
    };

    struct SnapshotOptions final {
        std::filesystem::path Directory;
        SnapshotUpdateMode UpdateMode = SnapshotUpdateMode::kNever;
        bool NormalizeLineEndings = true;
    };

    /// Structured comparison result; DifferenceLine/Column are one-based.
    struct SnapshotResult final {
        SnapshotStatus Status = SnapshotStatus::kInvalidOptions;
        std::filesystem::path File;
        std::string Expected;
        std::string Actual;
        std::string Message;
        std::size_t DifferenceLine = 0;
        std::size_t DifferenceColumn = 0;

        [[nodiscard]] bool Succeeded() const noexcept;
    };

    /// Compare against `<Directory>/<name>.snap`; names cannot contain paths.
    [[nodiscard]] SnapshotResult
    CompareSnapshot( std::string_view name,
                     std::string_view actual,
                     const SnapshotOptions& options );

} // namespace Progmasoft::Catch3
