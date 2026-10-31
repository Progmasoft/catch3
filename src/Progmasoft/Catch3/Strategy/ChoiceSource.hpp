// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Random.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Progmasoft::Catch3
{

    /// The stream of decisions a strategy makes while it builds one value.
    ///
    /// Every random decision of a strategy is one choice: an integer in the
    /// inclusive range [0, maximum]. During generation the choices come from
    /// a Random and are recorded; during shrinking and replay they are read
    /// back from a recorded sequence. A strategy therefore never shrinks
    /// values itself. The engine shrinks the recorded choices, and every
    /// strategy built from these choices, including mapped, filtered and
    /// dependent ones, shrinks with it.
    ///
    /// The contract that makes this work: choice 0 is the simplest decision,
    /// and a smaller choice never produces a more complex value. A shorter
    /// sequence is simpler than a longer one. A read past the end of a
    /// replayed sequence yields 0, the simplest decision, so deleting
    /// choices is always a valid shrink.
    class ChoiceSource final
    {
    public:
        /// Generation: choices come from a seeded Random, scaled by size.
        ChoiceSource(std::uint64_t seed, std::size_t size) noexcept
            : random_(seed)
            , size_(size)
        {}

        /// Replay: choices are read from a recorded sequence, which has to
        /// outlive this object.
        ChoiceSource(const std::vector<std::uint64_t> &replay,
                     std::size_t size) noexcept
            : random_(0)
            , replay_(&replay)
            , replaying_(true)
            , size_(size)
        {}

        /// A uniform choice in [0, maximum].
        [[nodiscard]] std::uint64_t
        Draw(std::uint64_t maximum)
        {
            if (replaying_)
            {
                return Record(ReadReplay(maximum));
            }
            const std::uint64_t value
                = maximum == UINT64_MAX ? random_.Next()
                                        : random_.Below(maximum + 1U);
            return Record(value);
        }

        /// A choice in [0, maximum] whose generation favours small values
        /// and the edges of the range, scaled by the current size. Replay
        /// is identical to Draw: only the distribution differs, never the
        /// meaning of a choice.
        [[nodiscard]] std::uint64_t
        DrawBiased(std::uint64_t maximum)
        {
            if (replaying_)
            {
                return Record(ReadReplay(maximum));
            }
            const std::uint64_t roll = random_.Below(32);
            std::uint64_t value = 0;
            if (roll == 0)
            {
                value = 0;
            }
            else if (roll == 1)
            {
                value = maximum;
            }
            else if (roll < 4)
            {
                value = maximum == 0 ? 0 : random_.Below(2);
            }
            else if (roll < 8 || maximum <= 1)
            {
                value = maximum == UINT64_MAX ? random_.Next()
                                              : random_.Below(maximum + 1U);
            }
            else
            {
                // Grows with the size so that early trials stay small.
                const std::uint64_t limit = ScaledLimit(maximum);
                value = random_.Below(limit + 1U);
            }
            return Record(value);
        }

        /// A biased coin: true with roughly the given probability during
        /// generation. false (choice 0) is the simpler answer.
        [[nodiscard]] bool
        DrawBoolean(double probabilityOfTrue)
        {
            if (replaying_)
            {
                return Record(ReadReplay(1)) == 1;
            }
            const double unit
                = static_cast<double>(random_.Next() >> 11U) * 0x1.0p-53;
            return Record(unit < probabilityOfTrue ? 1 : 0) == 1;
        }

        /// The size of the current trial, from 0 to 100. Strategies use it
        /// to keep early trials small.
        [[nodiscard]] std::size_t
        Size() const noexcept
        {
            return size_;
        }

        /// The choices drawn so far.
        [[nodiscard]] const std::vector<std::uint64_t> &
        Choices() const noexcept
        {
            return choices_;
        }

        /// True when a replay read past the end of its sequence.
        [[nodiscard]] bool
        Overran() const noexcept
        {
            return overran_;
        }

        /// Stops generation when a strategy exceeds this many choices, so
        /// that an unbounded recursive strategy cannot exhaust memory.
        static constexpr std::size_t kMaximumChoices = 1U << 20U;

        [[nodiscard]] bool
        Exhausted() const noexcept
        {
            return choices_.size() >= kMaximumChoices;
        }

    private:
        [[nodiscard]] std::uint64_t
        ReadReplay(std::uint64_t maximum) noexcept
        {
            if (position_ >= replay_->size())
            {
                overran_ = true;
                return 0;
            }
            const std::uint64_t value = (*replay_)[position_++];
            // A shrink may lower a choice that an earlier, changed choice
            // now bounds more tightly; clamping keeps the replay valid.
            return value > maximum ? maximum : value;
        }

        [[nodiscard]] std::uint64_t
        ScaledLimit(std::uint64_t maximum) const noexcept
        {
            // 2^(size / 4 + 2): 4 at size 0, about 2^27 at size 100.
            const std::size_t bits = size_ / 4U + 2U;
            if (bits >= 64U)
            {
                return maximum;
            }
            const std::uint64_t limit = (std::uint64_t{ 1 } << bits) - 1U;
            return limit < maximum ? limit : maximum;
        }

        std::uint64_t
        Record(std::uint64_t value)
        {
            choices_.push_back(value);
            return value;
        }

        Random random_;
        const std::vector<std::uint64_t> *replay_ = nullptr;
        std::size_t position_ = 0;
        bool replaying_ = false;
        bool overran_ = false;
        std::size_t size_;
        std::vector<std::uint64_t> choices_;
    };

} // namespace Progmasoft::Catch3
