// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Generator.hpp>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{

    /// Picks uniformly from a fixed, non-empty list of values.
    ///
    /// The list order is the simplicity order: a value shrinks to the values
    /// listed before it, earliest first, so place the simplest case first.
    /// A value that is not in the list has no candidates.
    template<typename ValueType>
        requires std::equality_comparable<ValueType>
    [[nodiscard]] Generator<ValueType>
    Element(std::vector<ValueType> values)
    {
        if (values.empty())
        {
            Detail::RejectInvalidArgument(
                "Element requires at least one value");
        }

        // Both callbacks share one immutable copy of the list.
        const auto shared
            = std::make_shared<const std::vector<ValueType>>(std::move(values));
        return MakeGenerator<ValueType>(
            [shared](Random &random) {
                const std::size_t index
                    = random.Between(std::size_t{ 0 }, shared->size() - 1);
                return static_cast<ValueType>((*shared)[index]);
            },
            [shared](const ValueType &value) {
                const auto found
                    = std::find(shared->begin(), shared->end(), value);
                if (found == shared->end())
                {
                    return std::vector<ValueType>{};
                }
                return std::vector<ValueType>(shared->begin(), found);
            });
    }

    /// Convenience overload for a literal list of values.
    template<typename ValueType>
        requires std::equality_comparable<ValueType>
    [[nodiscard]] Generator<ValueType>
    Element(std::initializer_list<ValueType> values)
    {
        return Element(std::vector<ValueType>(values));
    }

} // namespace Progmasoft::Catch3
