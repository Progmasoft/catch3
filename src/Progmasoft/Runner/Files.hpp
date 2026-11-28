// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Expected.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Progmasoft::Catch3::Runner
{
    /// A lock between processes: a file created exclusively beside the one
    /// it protects. A lock file older than `stale` belongs to a process that
    /// died and is broken.
    class FileLock final
    {
    public:
        FileLock(const std::filesystem::path &file,
                 std::chrono::milliseconds wait,
                 std::chrono::milliseconds stale = std::chrono::seconds(30));
        FileLock(const FileLock &) = delete;
        FileLock &
        operator=(const FileLock &) = delete;
        ~FileLock();

        /// Whether the lock was taken before `wait` ran out.
        [[nodiscard]] bool
        Held() const noexcept
        {
            return held_;
        }

    private:
        std::filesystem::path path_;
        bool held_ = false;
    };

    [[nodiscard]] Expected<std::string>
    ReadFile(const std::filesystem::path &file);

    /// Replaces `file` with `contents` in one rename, so that a reader sees
    /// the old or the new file, never half of one. Creates the directory.
    Status
    WriteFileAtomically(const std::filesystem::path &file,
                        std::string_view contents);

    /// Creates an empty file only the user can read and write, truncating a
    /// regular file of that name. A symbolic link or anything else that is
    /// not a regular file is refused, so the path cannot redirect the write.
    Status
    CreatePrivateFile(const std::filesystem::path &file);

    /// A name for a temporary file that no other process or thread uses.
    [[nodiscard]] std::string
    UniqueName();

    /// A digest of the inputs of the tests besides their binary: the
    /// contents of every regular file under `paths`, a file or a directory
    /// each, and the values of the variables `names`. A missing path, an
    /// unreadable file and an unset variable each count as a value of their
    /// own, so that a change between any two states changes the digest.
    /// Empty when both lists are, so that fingerprints without declared
    /// inputs stay as they were.
    [[nodiscard]] std::string
    DeclaredInputsDigest(std::vector<std::string> paths,
                         std::vector<std::string> names);
} // namespace Progmasoft::Catch3::Runner
