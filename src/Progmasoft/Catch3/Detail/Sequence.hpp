// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3::Detail
{

    /// Upper bound on the candidates one sequence value offers per shrink
    /// step. The property runner re-shrinks after every accepted candidate,
    /// so a bounded list still reaches small counterexamples while keeping
    /// each step linear in this constant instead of in the sequence length.
    inline constexpr std::size_t kMaximumSequenceCandidates = 256;

    /// Largest sequence a built-in generator will create. A size range above
    /// this is rejected instead of silently allocating gigabytes in a test.
    inline constexpr std::size_t kMaximumSequenceLength = std::size_t{ 1 }
                                                          << 20U;

    /**
     * @brief Ordered shrink candidates shared by Vector and Text.
     *
     * Candidates are produced in a fixed order, simplest first:
     *
     * 1. the shortest allowed prefix,
     * 2. the first half and then the second half,
     * 3. the value with one element removed, position by position,
     * 4. the value with one element replaced by each of that element's own
     *    shrink candidates, position by position.
     *
     * No candidate is shorter than @p minimumSize, every candidate differs
     * from @p value in length or in exactly one element, and the list is cut
     * at kMaximumSequenceCandidates. The same candidate may appear twice,
     * for example when removing the last element equals the shortest prefix;
     * the runner simply tests it again.
     */
    template<typename Sequence, typename ShrinkElement>
    [[nodiscard]] std::vector<Sequence>
    ShrinkSequence(const Sequence &value,
                   std::size_t minimumSize,
                   ShrinkElement &&shrinkElement)
    {
        using Element = typename Sequence::value_type;
        using Difference = typename std::iterator_traits<
            typename Sequence::const_iterator>::difference_type;

        std::vector<Sequence> candidates;
        const std::size_t size = value.size();
        const auto full = [&candidates] {
            return candidates.size() >= kMaximumSequenceCandidates;
        };
        const auto at = [&value](std::size_t index) {
            return value.begin() + static_cast<Difference>(index);
        };

        if (size > minimumSize)
        {
            candidates.push_back(Sequence(at(0), at(minimumSize)));

            const std::size_t kept = size - size / 2;
            if (kept > minimumSize && kept < size)
            {
                candidates.push_back(Sequence(at(0), at(kept)));
                candidates.push_back(Sequence(at(size - kept), at(size)));
            }

            for (std::size_t index = 0; index < size && !full(); ++index)
            {
                Sequence shorter(at(0), at(index));
                shorter.insert(shorter.end(), at(index + 1), at(size));
                candidates.push_back(std::move(shorter));
            }
        }

        for (std::size_t index = 0; index < size && !full(); ++index)
        {
            const Element current = static_cast<Element>(*at(index));
            for (const auto &simpler : shrinkElement(current))
            {
                if (full())
                {
                    break;
                }
                Sequence replaced = value;
                *(replaced.begin() + static_cast<Difference>(index))
                    = static_cast<Element>(simpler);
                candidates.push_back(std::move(replaced));
            }
        }
        return candidates;
    }

} // namespace Progmasoft::Catch3::Detail
