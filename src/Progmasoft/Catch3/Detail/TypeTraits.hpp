// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <type_traits>
#include <utility>

namespace Progmasoft::Catch3::Detail
{

    /// std::remove_cvref_t, which C++17 does not have.
    template<typename Type>
    using RemoveCvRef = std::remove_cv_t<std::remove_reference_t<Type>>;

    /// std::identity, which C++17 does not have.
    struct Identity final
    {
        template<typename Type>
        [[nodiscard]] constexpr Type &&
        operator()(Type &&value) const noexcept
        {
            return std::forward<Type>(value);
        }
    };

    /// An integral type that holds numbers, so anything but bool.
    template<typename Type>
    inline constexpr bool kIsInteger
        = std::is_integral_v<Type>
          && !std::is_same_v<std::remove_cv_t<Type>, bool>;

    /// Limits a function template to the types of kIsInteger.
    template<typename Type>
    using EnableIfInteger = std::enable_if_t<kIsInteger<Type>, int>;

    template<typename Type, typename = void>
    inline constexpr bool kIsEqualityComparable = false;

    template<typename Type>
    inline constexpr bool kIsEqualityComparable<
        Type,
        std::enable_if_t<
            std::is_convertible_v<decltype(std::declval<const Type &>()
                                           == std::declval<const Type &>()),
                                  bool>>> = true;

} // namespace Progmasoft::Catch3::Detail
