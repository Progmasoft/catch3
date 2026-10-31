// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Strategy/Strategy.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <type_traits>

namespace Progmasoft::Catch3
{

    /// The range of Gen::Floats, and what it may produce besides finite
    /// values inside the range.
    struct FloatOptions final
    {
        std::optional<double> Minimum = std::nullopt;
        std::optional<double> Maximum = std::nullopt;
        bool AllowNaN = false;
        bool AllowInfinity = false;
        bool AllowNegativeZero = true;
    };

} // namespace Progmasoft::Catch3

namespace Progmasoft::Catch3::Gen
{

    /// Integers in [minimum, maximum]. They shrink towards the value of the
    /// range closest to zero; generation favours small magnitudes, zero and
    /// the bounds, scaled by the trial size.
    template<typename IntegerType,
             Catch3::Detail::EnableIfInteger<IntegerType> = 0>
    [[nodiscard]] Strategy<IntegerType>
    Integers(IntegerType minimum = std::numeric_limits<IntegerType>::lowest(),
             IntegerType maximum = std::numeric_limits<IntegerType>::max())
    {
        static_assert(sizeof(IntegerType) <= sizeof(std::uint64_t));
        if (minimum > maximum)
        {
            Detail::RejectInvalidArgument(
                "Integers requires minimum <= maximum");
        }
        return Strategy<IntegerType>(
            [minimum, maximum](
                ChoiceSource &source) -> std::optional<IntegerType> {
                // Magnitudes are computed in 64 bits; the casts are exact
                // because every magnitude lies inside the requested range.
                if (minimum >= 0)
                {
                    const auto width = static_cast<std::uint64_t>(maximum)
                                       - static_cast<std::uint64_t>(minimum);
                    return static_cast<IntegerType>(
                        static_cast<std::uint64_t>(minimum)
                        + source.DrawBiased(width));
                }
                if (maximum <= 0)
                {
                    // Two's complement arithmetic in 64 bits: maximum minus
                    // minimum may not fit a signed 64-bit integer.
                    const auto width = static_cast<std::uint64_t>(
                                           static_cast<std::int64_t>(maximum))
                                       - static_cast<std::uint64_t>(
                                           static_cast<std::int64_t>(minimum));
                    const auto value = static_cast<std::uint64_t>(
                                           static_cast<std::int64_t>(maximum))
                                       - source.DrawBiased(width);
                    return static_cast<IntegerType>(
                        static_cast<std::int64_t>(value));
                }
                // The range crosses zero: a sign, then a magnitude. Choice 0
                // is non-negative, so values shrink to 0, never to minimum.
                if (source.Draw(1) == 0)
                {
                    return static_cast<IntegerType>(source.DrawBiased(
                        static_cast<std::uint64_t>(maximum)));
                }
                // -(minimum + 1) cannot overflow, and the magnitude of -1 is
                // choice 0.
                const auto negativeWidth = static_cast<std::uint64_t>(
                    -(static_cast<std::int64_t>(minimum) + 1));
                const auto magnitude = source.DrawBiased(negativeWidth);
                return static_cast<IntegerType>(
                    -1 - static_cast<std::int64_t>(magnitude));
            });
    }

    /// true or false; shrinks towards false.
    [[nodiscard]] inline Strategy<bool>
    Booleans()
    {
        return Strategy<bool>(
            [](ChoiceSource &source) -> std::optional<bool> {
                return source.Draw(1) == 1;
            });
    }

    /// Floating-point values. They shrink towards zero, or the bound
    /// closest to it, then towards integral values and short fractions.
    /// Generation mixes integral values, short binary fractions, uniform
    /// values of the range, arbitrary bit patterns, and edge values such as
    /// the smallest subnormal and the largest finite value.
    template<typename FloatType,
             std::enable_if_t<std::is_floating_point_v<FloatType>, int> = 0>
    [[nodiscard]] Strategy<FloatType>
    Floats(FloatOptions options = {})
    {
        constexpr double kLargest = std::numeric_limits<FloatType>::max();
        const double minimum = options.Minimum.value_or(-kLargest);
        const double maximum = options.Maximum.value_or(kLargest);
        if (!(minimum <= maximum) || std::isnan(minimum) || std::isnan(maximum))
        {
            Detail::RejectInvalidArgument(
                "Floats requires minimum <= maximum, neither NaN");
        }
        return Strategy<FloatType>(
            [options, minimum, maximum](
                ChoiceSource &source) -> std::optional<FloatType> {
                const auto inRange = [&](double value) {
                    return value >= minimum && value <= maximum;
                };
                const double anchor
                    = inRange(0.0) ? 0.0
                                   : (std::abs(minimum) < std::abs(maximum)
                                          ? minimum
                                          : maximum);
                const auto finish = [&](double value) -> FloatType {
                    if (!inRange(value))
                    {
                        value = anchor;
                    }
                    return static_cast<FloatType>(value);
                };
                // The kind is the first choice: 0 is the anchor itself, and
                // each later kind is less simple than the one before.
                const std::uint64_t kinds = 5U
                                            + (options.AllowInfinity ? 1U : 0U)
                                            + (options.AllowNaN ? 1U : 0U);
                const std::uint64_t kind = source.Draw(kinds);
                const auto sign = [&]() {
                    return source.Draw(1) == 0 ? 1.0 : -1.0;
                };
                switch (kind)
                {
                    case 0:
                        return finish(anchor);
                    case 1:
                        // Integral values near zero.
                        return finish(sign()
                                      * static_cast<double>(
                                          source.DrawBiased(1ULL << 53U)));
                    case 2:
                    {
                        // Short binary fractions such as 2.75.
                        const double whole = static_cast<double>(
                            source.DrawBiased(1ULL << 20U));
                        const double fraction
                            = static_cast<double>(source.Draw(255))
                              / 256.0;
                        return finish(sign() * (whole + fraction));
                    }
                    case 3:
                    {
                        // Uniform inside the range; finite ranges only.
                        const double unit
                            = static_cast<double>(source.Draw(
                                  (1ULL << 53U) - 1U))
                              * 0x1.0p-53;
                        const double value
                            = minimum * (1.0 - unit) + maximum * unit;
                        return finish(std::isfinite(value) ? value : anchor);
                    }
                    case 4:
                    {
                        static constexpr std::array<double, 9> kEdges = {
                            -0.0,
                            std::numeric_limits<double>::denorm_min(),
                            std::numeric_limits<double>::min(),
                            std::numeric_limits<double>::epsilon(),
                            1.0,
                            0.5,
                            static_cast<double>(
                                std::numeric_limits<FloatType>::max()),
                            static_cast<double>(
                                std::numeric_limits<FloatType>::lowest()),
                            static_cast<double>(
                                std::numeric_limits<FloatType>::min()),
                        };
                        const double edge = kEdges[static_cast<std::size_t>(
                            source.Draw(kEdges.size() - 1U))];
                        const double signedEdge = sign() * edge;
                        if (!options.AllowNegativeZero && signedEdge == 0.0
                            && std::signbit(signedEdge))
                        {
                            return finish(0.0);
                        }
                        return finish(signedEdge);
                    }
                    case 5:
                    {
                        // Any bit pattern; non-finite patterns fall back.
                        const auto bits = source.Draw(UINT64_MAX);
                        static_assert(sizeof(double) == sizeof(bits));
                        double value{};
                        std::memcpy(&value, &bits, sizeof(value));
                        return finish(std::isfinite(value) ? value : anchor);
                    }
                    default:
                        break;
                }
                const bool wantsInfinity
                    = options.AllowInfinity && kind == 6U;
                if (wantsInfinity)
                {
                    return static_cast<FloatType>(
                        sign() * std::numeric_limits<double>::infinity());
                }
                return std::numeric_limits<FloatType>::quiet_NaN();
            });
    }

} // namespace Progmasoft::Catch3::Gen
