// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include "Expected.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace Progmasoft::Catch3::Runner
{
    /// SHA-256, for fingerprints that must not collide by accident: the
    /// test binary, the source files a test depends on.
    class Sha256 final
    {
    public:
        Sha256();

        void
        Update(const void *data, std::size_t size);

        void
        Update(std::string_view text)
        {
            Update(text.data(), text.size());
        }

        /// The digest as 64 lowercase hexadecimal digits. The object cannot
        /// be updated afterwards.
        [[nodiscard]] std::string
        HexDigest();

    private:
        void
        Compress(const std::uint8_t *block);

        std::array<std::uint32_t, 8> state_{};
        std::array<std::uint8_t, 64> buffer_{};
        std::size_t bufferSize_ = 0;
        std::uint64_t totalSize_ = 0;
    };

    [[nodiscard]] std::string
    Sha256Hex(std::string_view text);

    /// The SHA-256 of a file's contents.
    [[nodiscard]] Expected<std::string>
    Sha256File(const std::filesystem::path &path);

    /// FNV-1a: a fast, stable 64-bit hash for sharding and signatures,
    /// where a collision only costs balance or merges two groups.
    [[nodiscard]] std::uint64_t
    Fnv1a64(std::string_view text) noexcept;

    /// The value as 16 lowercase hexadecimal digits.
    [[nodiscard]] std::string
    ToHex(std::uint64_t value);
} // namespace Progmasoft::Catch3::Runner
