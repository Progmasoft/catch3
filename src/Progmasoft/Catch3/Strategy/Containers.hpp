// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Strategy/Strategy.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{

    /// Bounds of a generated collection.
    struct SizeRange final
    {
        std::size_t Minimum = 0;
        /// No upper bound when unset; the length then grows with the size
        /// of the trial, around a quarter of it on average.
        std::optional<std::size_t> Maximum = std::nullopt;
    };

    namespace Detail
    {

        /// Draws a collection element by element. After the minimum, each
        /// further element is announced by a "more" choice, so that the
        /// engine removes an element by deleting its choices and shortens a
        /// collection by turning one "more" into 0.
        template<typename AddElement>
        [[nodiscard]] bool
        DrawElements(ChoiceSource &source,
                     const SizeRange &range,
                     AddElement addElement)
        {
            const std::size_t maximum
                = range.Maximum.value_or(std::numeric_limits<std::size_t>::max());
            const double average
                = std::max(1.0, static_cast<double>(source.Size()) / 4.0);
            const double probability = average / (average + 1.0);
            std::size_t count = 0;
            while (count < maximum)
            {
                if (count >= range.Minimum && !source.DrawBoolean(probability))
                {
                    break;
                }
                if (!addElement())
                {
                    return false;
                }
                ++count;
            }
            return true;
        }

        inline void
        CheckRange(const SizeRange &range)
        {
            if (range.Maximum.has_value() && *range.Maximum < range.Minimum)
            {
                Detail::RejectInvalidArgument(
                    "a size range requires Minimum <= Maximum");
            }
        }

    } // namespace Detail

    namespace Gen
    {

        /// Vectors of elements; they shrink by losing elements and by
        /// shrinking the elements that remain.
        template<typename ElementType>
        [[nodiscard]] Strategy<std::vector<ElementType>>
        Vectors(Strategy<ElementType> element, SizeRange range = {})
        {
            Detail::CheckRange(range);
            return Strategy<std::vector<ElementType>>(
                [element, range](ChoiceSource &source)
                    -> std::optional<std::vector<ElementType>> {
                    std::vector<ElementType> result;
                    const bool complete
                        = Detail::DrawElements(source, range, [&]() {
                              auto value = element.Draw(source);
                              if (!value.has_value())
                              {
                                  return false;
                              }
                              result.push_back(std::move(*value));
                              return true;
                          });
                    if (!complete)
                    {
                        return std::nullopt;
                    }
                    return result;
                });
        }

        /// Vectors whose elements are distinct under key. A duplicate is
        /// drawn again up to maximumRetries times before the trial gives
        /// up.
        template<typename ElementType, typename Key = std::identity>
        [[nodiscard]] Strategy<std::vector<ElementType>>
        UniqueVectors(Strategy<ElementType> element,
                      SizeRange range = {},
                      Key key = {},
                      std::size_t maximumRetries = 50)
        {
            Detail::CheckRange(range);
            return Strategy<std::vector<ElementType>>(
                [element, range, key, maximumRetries](ChoiceSource &source)
                    -> std::optional<std::vector<ElementType>> {
                    std::vector<ElementType> result;
                    const auto isNew = [&](const ElementType &candidate) {
                        return std::none_of(
                            result.begin(), result.end(), [&](const auto &old) {
                                return std::invoke(key, old)
                                       == std::invoke(key, candidate);
                            });
                    };
                    const bool complete
                        = Detail::DrawElements(source, range, [&]() {
                              for (std::size_t retry = 0;
                                   retry <= maximumRetries;
                                   ++retry)
                              {
                                  auto value = element.Draw(source);
                                  if (!value.has_value())
                                  {
                                      return false;
                                  }
                                  if (isNew(*value))
                                  {
                                      result.push_back(std::move(*value));
                                      return true;
                                  }
                              }
                              return false;
                          });
                    if (!complete || result.size() < range.Minimum)
                    {
                        return std::nullopt;
                    }
                    return result;
                });
        }

        /// Ordered sets of distinct elements.
        template<typename ElementType>
        [[nodiscard]] Strategy<std::set<ElementType>>
        Sets(Strategy<ElementType> element, SizeRange range = {})
        {
            return UniqueVectors(std::move(element), range)
                .Map([](std::vector<ElementType> values) {
                    return std::set<ElementType>(values.begin(), values.end());
                });
        }

        /// Ordered maps with distinct keys.
        template<typename KeyType, typename ValueType>
        [[nodiscard]] Strategy<std::map<KeyType, ValueType>>
        Maps(Strategy<KeyType> key,
             Strategy<ValueType> value,
             SizeRange range = {})
        {
            Detail::CheckRange(range);
            return Strategy<std::map<KeyType, ValueType>>(
                [key, value, range](ChoiceSource &source)
                    -> std::optional<std::map<KeyType, ValueType>> {
                    std::map<KeyType, ValueType> result;
                    const bool complete
                        = Detail::DrawElements(source, range, [&]() {
                              for (int retry = 0; retry <= 50; ++retry)
                              {
                                  auto drawnKey = key.Draw(source);
                                  if (!drawnKey.has_value())
                                  {
                                      return false;
                                  }
                                  if (result.contains(*drawnKey))
                                  {
                                      continue;
                                  }
                                  auto drawnValue = value.Draw(source);
                                  if (!drawnValue.has_value())
                                  {
                                      return false;
                                  }
                                  result.emplace(std::move(*drawnKey),
                                                 std::move(*drawnValue));
                                  return true;
                              }
                              return false;
                          });
                    if (!complete || result.size() < range.Minimum)
                    {
                        return std::nullopt;
                    }
                    return result;
                });
        }

        /// Strings whose characters come from character.
        [[nodiscard]] inline Strategy<std::string>
        Strings(Strategy<char> character, SizeRange range = {})
        {
            return Vectors(std::move(character), range)
                .Map([](std::vector<char> characters) {
                    return std::string(characters.begin(), characters.end());
                });
        }

        /// Characters of an alphabet; shrinks towards its first character.
        [[nodiscard]] inline Strategy<char>
        Characters(std::string alphabet)
        {
            return SampledFrom(std::vector<char>(alphabet.begin(), alphabet.end()));
        }

        /// Printable ASCII, simplest first: letters and digits, then the
        /// rest, so that counterexamples read well.
        [[nodiscard]] inline Strategy<char>
        PrintableCharacters()
        {
            return Characters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
        }

        /// UTF-8 encoded text of Unicode scalar values in [minimum, maximum];
        /// surrogates are never produced. Code points shrink towards
        /// minimum.
        [[nodiscard]] inline Strategy<std::string>
        Utf8Strings(SizeRange range = {},
                    char32_t minimum = U' ',
                    char32_t maximum = 0x10FFFF)
        {
            if (minimum > maximum || maximum > 0x10FFFF)
            {
                Detail::RejectInvalidArgument(
                    "Utf8Strings requires minimum <= maximum <= U+10FFFF");
            }
            const Strategy<char32_t> codePoint(
                [minimum, maximum](
                    ChoiceSource &source) -> std::optional<char32_t> {
                    std::uint64_t value
                        = minimum + source.DrawBiased(maximum - minimum);
                    if (value >= 0xD800 && value <= 0xDFFF)
                    {
                        value = minimum;
                    }
                    return static_cast<char32_t>(value);
                });
            return Vectors(codePoint, range)
                .Map([](const std::vector<char32_t> &codePoints) {
                    std::string text;
                    for (const char32_t code : codePoints)
                    {
                        if (code < 0x80)
                        {
                            text += static_cast<char>(code);
                        }
                        else if (code < 0x800)
                        {
                            text += static_cast<char>(0xC0 | (code >> 6U));
                            text += static_cast<char>(0x80 | (code & 0x3FU));
                        }
                        else if (code < 0x10000)
                        {
                            text += static_cast<char>(0xE0 | (code >> 12U));
                            text += static_cast<char>(
                                0x80 | ((code >> 6U) & 0x3FU));
                            text += static_cast<char>(0x80 | (code & 0x3FU));
                        }
                        else
                        {
                            text += static_cast<char>(0xF0 | (code >> 18U));
                            text += static_cast<char>(
                                0x80 | ((code >> 12U) & 0x3FU));
                            text += static_cast<char>(
                                0x80 | ((code >> 6U) & 0x3FU));
                            text += static_cast<char>(0x80 | (code & 0x3FU));
                        }
                    }
                    return text;
                });
        }

        /// No value or a value; shrinks towards no value.
        template<typename ValueType>
        [[nodiscard]] Strategy<std::optional<ValueType>>
        Optionals(Strategy<ValueType> value)
        {
            return Strategy<std::optional<ValueType>>(
                [value](ChoiceSource &source)
                    -> std::optional<std::optional<ValueType>> {
                    if (source.Draw(1) == 0)
                    {
                        return std::optional<ValueType>();
                    }
                    auto drawn = value.Draw(source);
                    if (!drawn.has_value())
                    {
                        return std::nullopt;
                    }
                    return std::optional<ValueType>(std::move(*drawn));
                });
        }

        /// One value from each strategy, drawn left to right.
        template<typename... ValueTypes>
        [[nodiscard]] Strategy<std::tuple<ValueTypes...>>
        Tuples(Strategy<ValueTypes>... strategies)
        {
            return Strategy<std::tuple<ValueTypes...>>(
                [strategies...](ChoiceSource &source)
                    -> std::optional<std::tuple<ValueTypes...>> {
                    std::tuple<std::optional<ValueTypes>...> parts;
                    bool complete = true;
                    // A braced list fixes the evaluation order: left to
                    // right, which keeps the choice sequence stable.
                    std::apply(
                        [&](auto &...part) {
                            ((complete = complete
                                         && (part = strategies.Draw(source))
                                                .has_value()),
                             ...);
                        },
                        parts);
                    if (!complete)
                    {
                        return std::nullopt;
                    }
                    return std::apply(
                        [](auto &...part) {
                            return std::tuple<ValueTypes...>(
                                std::move(*part)...);
                        },
                        parts);
                });
        }

        /// Builds an object from one value of each strategy, as
        /// ObjectType{ values... }.
        template<typename ObjectType, typename... ValueTypes>
        [[nodiscard]] Strategy<ObjectType>
        Construct(Strategy<ValueTypes>... strategies)
        {
            return Tuples(std::move(strategies)...)
                .Map([](std::tuple<ValueTypes...> values) {
                    return std::apply(
                        [](ValueTypes &...value) {
                            return ObjectType{ std::move(value)... };
                        },
                        values);
                });
        }

        /// Permutations of a list; they shrink towards the original order.
        template<typename ValueType>
        [[nodiscard]] Strategy<std::vector<ValueType>>
        Permutations(std::vector<ValueType> values)
        {
            return Strategy<std::vector<ValueType>>(
                [values](ChoiceSource &source)
                    -> std::optional<std::vector<ValueType>> {
                    std::vector<ValueType> result = values;
                    // Each choice says how far ahead the next element comes
                    // from; choice 0 keeps it in place.
                    for (std::size_t index = 0; index + 1U < result.size();
                         ++index)
                    {
                        const auto offset = static_cast<std::size_t>(
                            source.Draw(result.size() - index - 1U));
                        std::swap(result[index], result[index + offset]);
                    }
                    return result;
                });
        }

    } // namespace Gen

} // namespace Progmasoft::Catch3
