// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Generator.hpp>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{
    namespace Detail
    {

        template<std::size_t Index, typename... ValueTypes>
        void
        AppendComponentCandidates(
            const std::tuple<Generator<ValueTypes>...> &generators,
            const std::tuple<ValueTypes...> &value,
            std::vector<std::tuple<ValueTypes...>> &candidates)
        {
            for (const auto &simpler :
                 std::get<Index>(generators).Shrink(std::get<Index>(value)))
            {
                std::tuple<ValueTypes...> replaced = value;
                std::get<Index>(replaced) = simpler;
                candidates.push_back(std::move(replaced));
            }
        }

        template<typename... ValueTypes, std::size_t... Indices>
        [[nodiscard]] std::vector<std::tuple<ValueTypes...>>
        ShrinkTuple(const std::tuple<Generator<ValueTypes>...> &generators,
                    const std::tuple<ValueTypes...> &value,
                    std::index_sequence<Indices...>)
        {
            std::vector<std::tuple<ValueTypes...>> candidates;
            // The comma fold visits components strictly left to right.
            (AppendComponentCandidates<Indices>(generators, value, candidates),
             ...);
            return candidates;
        }

    } // namespace Detail

    /// Combines independent generators into one generator of tuples, which
    /// is how a property receives several generated arguments.
    ///
    /// Components are drawn strictly left to right from the trial's random
    /// source, so a tuple is reproducible from its replay seed. Shrinking
    /// changes one component at a time: all candidates of the first
    /// component, then all of the second, and so on. Combined with the
    /// runner's greedy restart this minimizes every component without
    /// trying the cross product of candidates.
    template<typename... ValueTypes>
        requires(sizeof...(ValueTypes) > 0)
    [[nodiscard]] Generator<std::tuple<ValueTypes...>>
    Tuple(Generator<ValueTypes>... generators)
    {
        using Value = std::tuple<ValueTypes...>;
        const std::tuple<Generator<ValueTypes>...> components{ std::move(
            generators)... };

        return MakeGenerator<Value>(
            [components](Random &random) {
                return std::apply(
                    [&random](const Generator<ValueTypes> &...component) {
                        // Braced initialization fixes the evaluation order;
                        // a function-call argument list would not.
                        return Value{ component.Generate(random)... };
                    },
                    components);
            },
            [components](const Value &value) {
                return Detail::ShrinkTuple(
                    components,
                    value,
                    std::index_sequence_for<ValueTypes...>{});
            });
    }

} // namespace Progmasoft::Catch3
