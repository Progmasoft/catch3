// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Detail/Sequence.hpp>
#include <Progmasoft/Catch3/Generator.hpp>
#include <cstddef>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{

    /// Generates vectors whose length lies in the inclusive size range and
    /// whose elements come from @p element, drawn in index order.
    ///
    /// Shrinking first shortens the vector, then simplifies single elements
    /// with the element generator's own candidates; see
    /// Detail::ShrinkSequence for the exact order. A shrunk vector never
    /// becomes shorter than @p minimumSize, so a property may rely on it.
    template<typename ValueType>
    [[nodiscard]] Generator<std::vector<ValueType>>
    Vector(Generator<ValueType> element,
           std::size_t minimumSize = 0,
           std::size_t maximumSize = 32)
    {
        if (minimumSize > maximumSize)
        {
            Detail::RejectInvalidArgument(
                "Vector requires minimumSize <= maximumSize");
        }
        if (maximumSize > Detail::kMaximumSequenceLength)
        {
            Detail::RejectInvalidArgument(
                "Vector maximumSize exceeds the supported sequence length");
        }

        return MakeGenerator<std::vector<ValueType>>(
            [element, minimumSize, maximumSize](Random &random) {
                const std::size_t size
                    = random.Between(minimumSize, maximumSize);
                std::vector<ValueType> values;
                values.reserve(size);
                for (std::size_t index = 0; index < size; ++index)
                {
                    values.push_back(element.Generate(random));
                }
                return values;
            },
            [element, minimumSize](const std::vector<ValueType> &value) {
                return Detail::ShrinkSequence(
                    value,
                    minimumSize,
                    [&element](const ValueType &item) {
                        return element.Shrink(item);
                    });
            });
    }

} // namespace Progmasoft::Catch3
