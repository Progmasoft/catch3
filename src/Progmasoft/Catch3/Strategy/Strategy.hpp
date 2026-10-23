// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Detail/InvalidArgument.hpp>
#include <Progmasoft/Catch3/Strategy/ChoiceSource.hpp>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{

    /// Describes how to build values of one type from a ChoiceSource.
    ///
    /// A strategy draws choices and returns a value, or no value when the
    /// choices do not describe one: a filter that found nothing, or a
    /// source that ran out of room. Shrinking needs no code here; see
    /// ChoiceSource. Strategies are cheap to copy and immutable.
    template<typename ValueType>
    class Strategy final
    {
    public:
        using Value = ValueType;
        using Function
            = std::function<std::optional<ValueType>(ChoiceSource &)>;

        explicit Strategy(Function draw)
            : draw_(std::make_shared<const Function>(std::move(draw)))
        {
            if (!*draw_)
            {
                Detail::RejectInvalidArgument("a strategy requires a function");
            }
        }

        /// Builds one value, or nothing when the choices describe none.
        [[nodiscard]] std::optional<ValueType>
        Draw(ChoiceSource &source) const
        {
            if (source.Exhausted())
            {
                return std::nullopt;
            }
            return (*draw_)(source);
        }

        /// Transforms every value. The result shrinks with this strategy.
        template<typename Transform>
        [[nodiscard]] auto
        Map(Transform transform) const
        {
            using Result = std::remove_cvref_t<
                std::invoke_result_t<Transform &, ValueType>>;
            Strategy self = *this;
            return Strategy<Result>(
                [self, transform](
                    ChoiceSource &source) mutable -> std::optional<Result> {
                    auto value = self.Draw(source);
                    if (!value.has_value())
                    {
                        return std::nullopt;
                    }
                    return std::invoke(transform, std::move(*value));
                });
        }

        /// Keeps only values that satisfy the predicate. Each value costs
        /// one attempt; after maximumAttempts rejected values the strategy
        /// gives up for this trial, which the runner counts as a discarded
        /// trial and reports when it happens too often.
        template<typename Predicate>
        [[nodiscard]] Strategy
        Filter(Predicate predicate, std::size_t maximumAttempts = 100) const
        {
            if (maximumAttempts == 0)
            {
                Detail::RejectInvalidArgument(
                    "Filter requires at least one attempt");
            }
            Strategy self = *this;
            return Strategy(
                [self, predicate, maximumAttempts](
                    ChoiceSource &source) mutable -> std::optional<ValueType> {
                    for (std::size_t attempt = 0; attempt < maximumAttempts;
                         ++attempt)
                    {
                        auto value = self.Draw(source);
                        if (!value.has_value())
                        {
                            return std::nullopt;
                        }
                        if (std::invoke(predicate, std::as_const(*value)))
                        {
                            return value;
                        }
                    }
                    return std::nullopt;
                });
        }

        /// Builds a value, then a second strategy from it, and draws from
        /// that. Unlike value-based shrinking, both levels shrink: the
        /// engine shrinks the shared choice sequence.
        template<typename Bind>
        [[nodiscard]] auto
        FlatMap(Bind bind) const
        {
            using Next
                = std::remove_cvref_t<std::invoke_result_t<Bind &, ValueType>>;
            using Result = typename Next::Value;
            Strategy self = *this;
            return Strategy<Result>(
                [self, bind](
                    ChoiceSource &source) mutable -> std::optional<Result> {
                    auto value = self.Draw(source);
                    if (!value.has_value())
                    {
                        return std::nullopt;
                    }
                    return std::invoke(bind, std::move(*value)).Draw(source);
                });
        }

    private:
        std::shared_ptr<const Function> draw_;
    };

    /// The value type of a strategy.
    template<typename StrategyType>
    using StrategyValue = typename std::remove_cvref_t<StrategyType>::Value;

    namespace Gen
    {

        /// Always the same value; draws no choice.
        template<typename ValueType>
        [[nodiscard]] Strategy<std::remove_cvref_t<ValueType>>
        Just(ValueType value)
        {
            using Result = std::remove_cvref_t<ValueType>;
            return Strategy<Result>(
                [value = Result(std::move(value))](ChoiceSource &)
                    -> std::optional<Result> { return value; });
        }

        /// One of the strategies, chosen uniformly. Shrinks towards the
        /// first one, so list the simplest first.
        template<typename ValueType>
        [[nodiscard]] Strategy<ValueType>
        OneOf(std::vector<Strategy<ValueType>> strategies)
        {
            if (strategies.empty())
            {
                Detail::RejectInvalidArgument(
                    "OneOf requires at least one strategy");
            }
            return Strategy<ValueType>(
                [strategies = std::move(strategies)](
                    ChoiceSource &source) -> std::optional<ValueType> {
                    const auto index = source.Draw(strategies.size() - 1U);
                    return strategies[static_cast<std::size_t>(index)].Draw(
                        source);
                });
        }

        template<typename First, typename... Rest>
            requires(std::same_as<StrategyValue<First>, StrategyValue<Rest>>
                     && ...)
        [[nodiscard]] Strategy<StrategyValue<First>>
        OneOf(First first, Rest... rest)
        {
            return OneOf(std::vector<Strategy<StrategyValue<First>>>{
                std::move(first), std::move(rest)... });
        }

        /// A strategy chosen with probability proportional to its weight.
        /// Shrinks towards the first entry, whatever its weight.
        template<typename ValueType>
        [[nodiscard]] Strategy<ValueType>
        Frequency(
            std::vector<std::pair<std::uint32_t, Strategy<ValueType>>> entries)
        {
            std::uint64_t total = 0;
            for (const auto &entry : entries)
            {
                total += entry.first;
            }
            if (entries.empty() || total == 0)
            {
                Detail::RejectInvalidArgument(
                    "Frequency requires a positive total weight");
            }
            return Strategy<ValueType>(
                [entries = std::move(entries), total](
                    ChoiceSource &source) -> std::optional<ValueType> {
                    // The choice is an index into the cumulative weights, so
                    // choice 0 always selects the first entry.
                    std::uint64_t point = source.Draw(total - 1U);
                    for (const auto &entry : entries)
                    {
                        if (point < entry.first)
                        {
                            return entry.second.Draw(source);
                        }
                        point -= entry.first;
                    }
                    return entries.back().second.Draw(source);
                });
        }

        /// A uniformly chosen element of a fixed list; shrinks towards the
        /// first element.
        template<typename ValueType>
        [[nodiscard]] Strategy<ValueType>
        SampledFrom(std::vector<ValueType> values)
        {
            if (values.empty())
            {
                Detail::RejectInvalidArgument(
                    "SampledFrom requires at least one value");
            }
            return Strategy<ValueType>(
                [values = std::move(values)](
                    ChoiceSource &source) -> std::optional<ValueType> {
                    return values[static_cast<std::size_t>(
                        source.Draw(values.size() - 1U))];
                });
        }

        template<typename ValueType>
        [[nodiscard]] Strategy<ValueType>
        SampledFrom(std::initializer_list<ValueType> values)
        {
            return SampledFrom(std::vector<ValueType>(values));
        }

        /// Defers building a strategy until it is drawn from, so that a
        /// strategy can refer to itself.
        template<typename Factory>
        [[nodiscard]] auto
        Lazy(Factory factory)
        {
            using Result = std::remove_cvref_t<std::invoke_result_t<Factory &>>;
            using ValueType = typename Result::Value;
            return Strategy<ValueType>(
                [factory](
                    ChoiceSource &source) mutable -> std::optional<ValueType> {
                    return std::invoke(factory).Draw(source);
                });
        }

        /// A recursive structure: base builds the leaves, and extend builds a
        /// node from a strategy for its children. The depth is bounded by
        /// maximumDepth and grows with the size, so early trials are flat.
        /// Recursion shrinks towards a leaf, since choosing a leaf is the
        /// choice 0.
        template<typename ValueType, typename Extend>
        [[nodiscard]] Strategy<ValueType>
        Recursive(Strategy<ValueType> base,
                  Extend extend,
                  std::size_t maximumDepth = 8)
        {
            struct Node final
            {
                Strategy<ValueType> base;
                Extend extend;
                std::size_t maximumDepth;

                std::optional<ValueType>
                Draw(ChoiceSource &source, std::size_t depth) const
                {
                    const std::size_t allowed = std::min<std::size_t>(
                        maximumDepth, 1U + source.Size() / 12U);
                    if (depth >= allowed || !source.DrawBoolean(0.5))
                    {
                        return base.Draw(source);
                    }
                    const Node *self = this;
                    const Strategy<ValueType> children(
                        [self, depth](ChoiceSource &inner) {
                            return self->Draw(inner, depth + 1U);
                        });
                    return std::invoke(extend, children).Draw(source);
                }
            };
            auto node = std::make_shared<const Node>(
                Node{ std::move(base), std::move(extend), maximumDepth });
            return Strategy<ValueType>(
                [node](ChoiceSource &source) { return node->Draw(source, 0); });
        }

    } // namespace Gen

} // namespace Progmasoft::Catch3
