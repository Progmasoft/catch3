// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Property.hpp>
#include <Progmasoft/Catch3/Strategy/Containers.hpp>
#include <Progmasoft/Catch3/Strategy/Context.hpp>
#include <Progmasoft/Catch3/Strategy/Engine.hpp>
#include <Progmasoft/Catch3/Strategy/Strategy.hpp>
#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Progmasoft::Catch3
{

    namespace Detail
    {

        /// Runs a body on one input and turns what happened into a verdict.
        /// A body returns bool, or void and fails through
        /// CATCH3_PROPERTY_ASSERT; an exception from it is a failure.
        template<typename Body, typename Tuple>
        void
        RunBody(Body &&body, const Tuple &values, TrialOutcome &outcome)
        {
            TrialRecordScope scope(outcome.Record);
#if defined(__cpp_exceptions)
            try
            {
#endif
                using Result = decltype(std::apply(body, values));
                if constexpr (std::is_void_v<Result>)
                {
                    std::apply(body, values);
                }
                else
                {
                    if (!static_cast<bool>(std::apply(body, values)))
                    {
                        RecordPropertyFailureMessage(
                            "property returned false");
                    }
                }
#if defined(__cpp_exceptions)
            }
            catch (const std::exception &exception)
            {
                RecordPropertyFailureMessage(
                    std::string("property threw std::exception: ")
                    + exception.what());
            }
            catch (...)
            {
                RecordPropertyFailureMessage(
                    "property threw a non-standard exception");
            }
#endif
            if (outcome.Record.Failure.has_value())
            {
                outcome.Verdict = TrialVerdict::kFailed;
                outcome.Reason = *outcome.Record.Failure;
            }
            else if (outcome.Record.Discarded)
            {
                outcome.Verdict = TrialVerdict::kDiscarded;
            }
        }

        /// Draws a value, catching what a strategy throws.
        template<typename ValueType>
        [[nodiscard]] std::optional<ValueType>
        DrawGuarded(const Strategy<ValueType> &strategy,
                    ChoiceSource &source,
                    TrialOutcome &outcome)
        {
#if defined(__cpp_exceptions)
            try
            {
#endif
                auto value = strategy.Draw(source);
                if (!value.has_value())
                {
                    outcome.Verdict = TrialVerdict::kInvalid;
                }
                return value;
#if defined(__cpp_exceptions)
            }
            catch (const std::exception &exception)
            {
                outcome.Verdict = TrialVerdict::kErrored;
                outcome.Reason = std::string("strategy threw std::exception: ")
                                 + exception.what();
            }
            catch (...)
            {
                outcome.Verdict = TrialVerdict::kErrored;
                outcome.Reason = "strategy threw a non-standard exception";
            }
            return std::nullopt;
#endif
        }

        template<typename... ValueTypes>
        [[nodiscard]] std::string
        DescribeArguments(const std::tuple<ValueTypes...> &values)
        {
            if constexpr (sizeof...(ValueTypes) == 1)
            {
                return DescribeValue(std::get<0>(values));
            }
            else
            {
                return DescribeValue(values);
            }
        }

        template<typename Body, typename... ValueTypes>
        [[nodiscard]] PropertyResult
        ForAllWith(const PropertyOptions &options,
                   Body body,
                   Strategy<ValueTypes>... strategies)
        {
            static_assert(sizeof...(ValueTypes) > 0,
                          "ForAll needs at least one strategy");
            const auto input = Gen::Tuples(std::move(strategies)...);
            const TrialRunner runner =
                [&](ChoiceSource &source, bool describe) -> TrialOutcome {
                TrialOutcome outcome;
                auto values = DrawGuarded(input, source, outcome);
                if (!values.has_value())
                {
                    return outcome;
                }
                if (describe)
                {
                    outcome.Description = DescribeArguments(*values);
                }
                RunBody(body, std::as_const(*values), outcome);
                return outcome;
            };
            return RunPropertyEngine(options, runner);
        }

        template<typename Type>
        inline constexpr bool kIsPropertyOptions
            = std::is_same_v<std::remove_cvref_t<Type>, PropertyOptions>;

    } // namespace Detail

    /// Checks that a property holds for values drawn from strategies.
    ///
    ///     ForAll([options,] strategy..., body)
    ///
    /// body receives one value per strategy, as const references. It
    /// returns bool, or returns void and uses CATCH3_PROPERTY_ASSERT and
    /// CATCH3_PROPERTY_ASSUME; Note, Classify, Cover and Target may be
    /// called from it. A failure is minimized by shrinking the choices the
    /// strategies made, and the report carries a replay token for exactly
    /// the minimized input.
    template<typename... Arguments>
    [[nodiscard]] PropertyResult
    ForAll(Arguments &&...arguments)
    {
        constexpr std::size_t kCount = sizeof...(Arguments);
        static_assert(kCount >= 2, "ForAll needs strategies and a body");
        auto all = std::forward_as_tuple(std::forward<Arguments>(arguments)...);
        using First = std::tuple_element_t<0, std::tuple<Arguments...>>;
        if constexpr (Detail::kIsPropertyOptions<First>)
        {
            return [&]<std::size_t... Index>(std::index_sequence<Index...>) {
                return Detail::ForAllWith(std::get<0>(all),
                                          std::get<kCount - 1U>(all),
                                          std::get<Index + 1U>(all)...);
            }(std::make_index_sequence<kCount - 2U>{});
        }
        else
        {
            return [&]<std::size_t... Index>(std::index_sequence<Index...>) {
                return Detail::ForAllWith(PropertyOptions{},
                                          std::get<kCount - 1U>(all),
                                          std::get<Index>(all)...);
            }(std::make_index_sequence<kCount - 1U>{});
        }
    }

} // namespace Progmasoft::Catch3
