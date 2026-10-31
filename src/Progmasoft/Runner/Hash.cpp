// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include "Hash.hpp"

#include <cstring>
#include <fstream>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        constexpr std::array<std::uint32_t, 64> kRoundConstants = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
            0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
            0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
            0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
            0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
            0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
            0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
            0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
            0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
            0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };

        constexpr std::uint32_t
        RotateRight(std::uint32_t value, int count) noexcept
        {
            return (value >> count) | (value << (32 - count));
        }
    } // namespace

    Sha256::Sha256()
        : state_{0x6a09e667,
                 0xbb67ae85,
                 0x3c6ef372,
                 0xa54ff53a,
                 0x510e527f,
                 0x9b05688c,
                 0x1f83d9ab,
                 0x5be0cd19}
    {
    }

    void
    Sha256::Update(const void *data, std::size_t size)
    {
        const auto *bytes = static_cast<const std::uint8_t *>(data);
        totalSize_ += size;
        while (size > 0)
        {
            const std::size_t room = buffer_.size() - bufferSize_;
            const std::size_t take = size < room ? size : room;
            std::memcpy(buffer_.data() + bufferSize_, bytes, take);
            bufferSize_ += take;
            bytes += take;
            size -= take;
            if (bufferSize_ == buffer_.size())
            {
                Compress(buffer_.data());
                bufferSize_ = 0;
            }
        }
    }

    void
    Sha256::Compress(const std::uint8_t *block)
    {
        std::array<std::uint32_t, 64> schedule{};
        for (int index = 0; index < 16; ++index)
        {
            schedule[index] =
                (static_cast<std::uint32_t>(block[index * 4]) << 24) |
                (static_cast<std::uint32_t>(block[index * 4 + 1]) << 16) |
                (static_cast<std::uint32_t>(block[index * 4 + 2]) << 8) |
                static_cast<std::uint32_t>(block[index * 4 + 3]);
        }
        for (int index = 16; index < 64; ++index)
        {
            const std::uint32_t s0 = RotateRight(schedule[index - 15], 7) ^
                                     RotateRight(schedule[index - 15], 18) ^
                                     (schedule[index - 15] >> 3);
            const std::uint32_t s1 = RotateRight(schedule[index - 2], 17) ^
                                     RotateRight(schedule[index - 2], 19) ^
                                     (schedule[index - 2] >> 10);
            schedule[index] =
                schedule[index - 16] + s0 + schedule[index - 7] + s1;
        }
        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];
        std::uint32_t f = state_[5];
        std::uint32_t g = state_[6];
        std::uint32_t h = state_[7];
        for (int index = 0; index < 64; ++index)
        {
            const std::uint32_t s1 =
                RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const std::uint32_t choice = (e & f) ^ (~e & g);
            const std::uint32_t first =
                h + s1 + choice + kRoundConstants[index] + schedule[index];
            const std::uint32_t s0 =
                RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t second = s0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + first;
            d = c;
            c = b;
            b = a;
            a = first + second;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::string
    Sha256::HexDigest()
    {
        const std::uint64_t bitCount = totalSize_ * 8;
        const std::uint8_t marker = 0x80;
        Update(&marker, 1);
        const std::uint8_t zero = 0;
        while (bufferSize_ != 56)
        {
            Update(&zero, 1);
        }
        std::uint8_t length[8];
        for (int index = 0; index < 8; ++index)
        {
            length[index] =
                static_cast<std::uint8_t>(bitCount >> (56 - index * 8));
        }
        Update(length, sizeof(length));
        static constexpr char kDigits[] = "0123456789abcdef";
        std::string text;
        text.reserve(64);
        for (const std::uint32_t word : state_)
        {
            for (int shift = 28; shift >= 0; shift -= 4)
            {
                text.push_back(kDigits[(word >> shift) & 0xF]);
            }
        }
        return text;
    }

    std::string
    Sha256Hex(std::string_view text)
    {
        Sha256 hash;
        hash.Update(text);
        return hash.HexDigest();
    }

    Expected<std::string>
    Sha256File(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return Error{"cannot read '" + path.string() + "'"};
        }
        Sha256 hash;
        char buffer[1 << 16];
        while (file)
        {
            file.read(buffer, sizeof(buffer));
            const auto count = file.gcount();
            if (count > 0)
            {
                hash.Update(buffer, static_cast<std::size_t>(count));
            }
        }
        if (file.bad())
        {
            return Error{"cannot read '" + path.string() + "'"};
        }
        return hash.HexDigest();
    }

    std::uint64_t
    Fnv1a64(std::string_view text) noexcept
    {
        std::uint64_t hash = 0xcbf29ce484222325ULL;
        for (const char character : text)
        {
            hash ^= static_cast<unsigned char>(character);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    std::string
    ToHex(std::uint64_t value)
    {
        static constexpr char kDigits[] = "0123456789abcdef";
        std::string text(16, '0');
        for (int index = 15; index >= 0; --index)
        {
            text[static_cast<std::size_t>(index)] = kDigits[value & 0xF];
            value >>= 4;
        }
        return text;
    }
} // namespace Progmasoft::Catch3::Runner
