// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Strategy/Engine.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace Progmasoft::Catch3::Detail
{

    bool
    IsSimpler(std::span<const std::uint64_t> candidate,
              std::span<const std::uint64_t> current) noexcept
    {
        if (candidate.size() != current.size())
        {
            return candidate.size() < current.size();
        }
        return std::lexicographical_compare(
            candidate.begin(), candidate.end(), current.begin(), current.end());
    }

    namespace
    {

        constexpr std::string_view kTokenPrefix = "c3.";

        /// Replays candidates against the current best and keeps the
        /// simplest failing one. Every replay counts against the budget.
        class Shrinker final
        {
        public:
            using StillFails
                = std::function<std::optional<std::vector<std::uint64_t>>(
                    const std::vector<std::uint64_t> &)>;

            Shrinker(std::vector<std::uint64_t> failing,
                     std::size_t budget,
                     const StillFails &stillFails)
                : current_(std::move(failing))
                , budget_(budget)
                , stillFails_(stillFails)
            {}

            [[nodiscard]] std::vector<std::uint64_t>
            Run(std::size_t &calls)
            {
                bool improved = true;
                while (improved && !Spent())
                {
                    improved = false;
                    improved |= DeleteBlocks();
                    improved |= ZeroBlocks();
                    improved |= MinimizeChoices();
                    improved |= SortNeighbours();
                    improved |= MoveBetweenNeighbours();
                }
                calls = calls_;
                return current_;
            }

        private:
            static constexpr std::array<std::size_t, 5> kBlockSizes
                = { 16, 8, 4, 2, 1 };

            [[nodiscard]] bool
            Spent() const noexcept
            {
                return calls_ >= budget_;
            }

            /// Replays a candidate; true when it fails and the sequence it
            /// consumed is simpler than the current one, which it replaces.
            bool
            Try(const std::vector<std::uint64_t> &candidate)
            {
                if (Spent() || !IsSimpler(candidate, current_))
                {
                    return false;
                }
                ++calls_;
                auto consumed = stillFails_(candidate);
                if (!consumed.has_value() || !IsSimpler(*consumed, current_))
                {
                    return false;
                }
                current_ = std::move(*consumed);
                return true;
            }

            bool
            DeleteBlocks()
            {
                bool improved = false;
                for (const std::size_t block : kBlockSizes)
                {
                    // From the end, so that a deletion does not move the
                    // blocks still to be tried.
                    for (std::size_t index = current_.size();
                         index-- > 0 && !Spent();)
                    {
                        if (index + block > current_.size())
                        {
                            continue;
                        }
                        auto candidate = current_;
                        candidate.erase(
                            candidate.begin()
                                + static_cast<std::ptrdiff_t>(index),
                            candidate.begin()
                                + static_cast<std::ptrdiff_t>(index + block));
                        improved |= Try(candidate);
                    }
                }
                return improved;
            }

            bool
            ZeroBlocks()
            {
                bool improved = false;
                for (const std::size_t block : kBlockSizes)
                {
                    for (std::size_t index = 0;
                         index + block <= current_.size() && !Spent();
                         ++index)
                    {
                        const auto first = current_.begin()
                                           + static_cast<std::ptrdiff_t>(index);
                        if (std::all_of(first,
                                        first
                                            + static_cast<std::ptrdiff_t>(block),
                                        [](std::uint64_t value) {
                                            return value == 0;
                                        }))
                        {
                            continue;
                        }
                        auto candidate = current_;
                        std::fill_n(candidate.begin()
                                        + static_cast<std::ptrdiff_t>(index),
                                    block,
                                    0U);
                        improved |= Try(candidate);
                    }
                }
                return improved;
            }

            /// Lowers each choice as far as the failure allows, assuming
            /// that a smaller choice is usually still failing when a larger
            /// one is: a binary search between 0 and the current value.
            bool
            MinimizeChoices()
            {
                bool improved = false;
                for (std::size_t index = 0;
                     index < current_.size() && !Spent();
                     ++index)
                {
                    std::uint64_t high = current_[index];
                    if (high == 0)
                    {
                        continue;
                    }
                    if (TryValue(index, 0))
                    {
                        improved = true;
                        continue;
                    }
                    std::uint64_t low = 0;
                    while (low + 1U < high && !Spent()
                           && index < current_.size())
                    {
                        const std::uint64_t middle = low + (high - low) / 2U;
                        if (TryValue(index, middle))
                        {
                            high = middle;
                            improved = true;
                        }
                        // Binary search assumes that failures are monotone
                        // in the choice; a filter or a parity condition
                        // breaks that, so the neighbour gets a chance too:
                        // with only even values passing a filter, an odd
                        // middle would otherwise stop the search.
                        else if (middle + 1U < high
                                 && TryValue(index, middle + 1U))
                        {
                            high = middle + 1U;
                            improved = true;
                        }
                        else
                        {
                            low = middle + 1U;
                        }
                    }
                    // A few single steps below the result, once: 104 -> 102
                    // when only even values pass. The next round of passes
                    // repeats the search from there.
                    for (std::uint64_t step = 1; step <= 8 && !Spent()
                                                 && index < current_.size()
                                                 && current_[index] >= step;
                         ++step)
                    {
                        if (TryValue(index, current_[index] - step))
                        {
                            improved = true;
                            break;
                        }
                    }
                }
                return improved;
            }

            bool
            TryValue(std::size_t index, std::uint64_t value)
            {
                if (index >= current_.size() || current_[index] <= value)
                {
                    return false;
                }
                auto candidate = current_;
                candidate[index] = value;
                return Try(candidate);
            }

            bool
            SortNeighbours()
            {
                bool improved = false;
                for (std::size_t index = 0;
                     index + 1U < current_.size() && !Spent();
                     ++index)
                {
                    if (current_[index] <= current_[index + 1U])
                    {
                        continue;
                    }
                    auto candidate = current_;
                    std::swap(candidate[index], candidate[index + 1U]);
                    improved |= Try(candidate);
                }
                return improved;
            }

            /// Moves one unit from a choice to one of the next few: the
            /// sequence gets lexicographically smaller while a sum such as
            /// x + y > 10 keeps failing.
            bool
            MoveBetweenNeighbours()
            {
                bool improved = false;
                for (std::size_t index = 0;
                     index < current_.size() && !Spent();
                     ++index)
                {
                    for (std::size_t offset = 1;
                         offset <= 4 && index + offset < current_.size()
                         && !Spent();
                         ++offset)
                    {
                        if (current_[index] == 0
                            || current_[index + offset] == UINT64_MAX)
                        {
                            break;
                        }
                        auto candidate = current_;
                        --candidate[index];
                        ++candidate[index + offset];
                        improved |= Try(candidate);
                    }
                }
                return improved;
            }

            std::vector<std::uint64_t> current_;
            std::size_t budget_;
            std::size_t calls_ = 0;
            const StillFails &stillFails_;
        };

    } // namespace

    std::vector<std::uint64_t>
    ShrinkChoices(
        std::vector<std::uint64_t> failing,
        std::size_t budget,
        const std::function<std::optional<std::vector<std::uint64_t>>(
            const std::vector<std::uint64_t> &)> &stillFails,
        std::size_t &calls)
    {
        Shrinker shrinker(std::move(failing), budget, stillFails);
        return shrinker.Run(calls);
    }

    std::string
    EncodeChoices(std::size_t size, std::span<const std::uint64_t> choices)
    {
        // c3.<size>.<choice>-<choice>-... with each number in base 36: short
        // enough to paste from a CI log, and only characters that survive
        // shells and C++ string literals unescaped.
        std::string token(kTokenPrefix);
        std::array<char, 24> buffer{};
        const auto append = [&](std::uint64_t value) {
            const auto end = std::to_chars(
                                 buffer.data(), buffer.data() + buffer.size(),
                                 value, 36)
                                 .ptr;
            token.append(buffer.data(), end);
        };
        append(size);
        token += '.';
        for (std::size_t index = 0; index < choices.size(); ++index)
        {
            if (index != 0)
            {
                token += '-';
            }
            append(choices[index]);
        }
        return token;
    }

    std::optional<DecodedChoices>
    DecodeChoices(std::string_view token)
    {
        if (!token.starts_with(kTokenPrefix))
        {
            return std::nullopt;
        }
        token.remove_prefix(kTokenPrefix.size());
        const auto parse = [](std::string_view text)
            -> std::optional<std::uint64_t> {
            std::uint64_t value = 0;
            const auto result = std::from_chars(
                text.data(), text.data() + text.size(), value, 36);
            if (text.empty() || result.ec != std::errc()
                || result.ptr != text.data() + text.size())
            {
                return std::nullopt;
            }
            return value;
        };
        const auto dot = token.find('.');
        if (dot == std::string_view::npos)
        {
            return std::nullopt;
        }
        const auto size = parse(token.substr(0, dot));
        if (!size.has_value() || *size > 100U)
        {
            return std::nullopt;
        }
        DecodedChoices decoded;
        decoded.Size = static_cast<std::size_t>(*size);
        std::string_view rest = token.substr(dot + 1U);
        while (!rest.empty())
        {
            const auto dash = rest.find('-');
            const auto value = parse(rest.substr(0, dash));
            if (!value.has_value())
            {
                return std::nullopt;
            }
            decoded.Choices.push_back(*value);
            if (dash == std::string_view::npos)
            {
                break;
            }
            rest.remove_prefix(dash + 1U);
            if (rest.empty())
            {
                return std::nullopt;
            }
        }
        return decoded;
    }

} // namespace Progmasoft::Catch3::Detail
