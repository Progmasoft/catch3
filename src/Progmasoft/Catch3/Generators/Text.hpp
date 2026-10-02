// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Detail/Sequence.hpp>
#include <Progmasoft/Catch3/Generator.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{

    /// The 95 printable ASCII characters, space through tilde, with the
    /// lower-case letter `a` first so that it is the simplest character.
    [[nodiscard]] inline std::string
    PrintableAscii()
    {
        std::string alphabet = "a";
        for (char character = ' '; character <= '~'; ++character)
        {
            if (character != 'a')
            {
                alphabet.push_back(character);
            }
        }
        return alphabet;
    }

    /// Generates byte strings whose characters are drawn from @p alphabet
    /// and whose length lies in the inclusive range.
    ///
    /// The alphabet order is the simplicity order: shrinking first shortens
    /// the string and then replaces single characters with earlier alphabet
    /// characters, earliest first. Characters are bytes; the generator does
    /// not interpret an encoding, so supply an alphabet that is valid for
    /// the code under test.
    [[nodiscard]] inline Generator<std::string>
    Text(std::string alphabet = PrintableAscii(),
         std::size_t minimumLength = 0,
         std::size_t maximumLength = 32)
    {
        if (alphabet.empty())
        {
            Detail::RejectInvalidArgument(
                "Text requires a non-empty alphabet");
        }
        if (minimumLength > maximumLength)
        {
            Detail::RejectInvalidArgument(
                "Text requires minimumLength <= maximumLength");
        }
        if (maximumLength > Detail::kMaximumSequenceLength)
        {
            Detail::RejectInvalidArgument(
                "Text maximumLength exceeds the supported sequence length");
        }

        const auto shared
            = std::make_shared<const std::string>(std::move(alphabet));
        return MakeGenerator<std::string>(
            [shared, minimumLength, maximumLength](Random &random) {
                const std::size_t length
                    = random.Between(minimumLength, maximumLength);
                std::string text;
                text.reserve(length);
                for (std::size_t index = 0; index < length; ++index)
                {
                    text.push_back((*shared)[random.Between(
                        std::size_t{ 0 },
                        shared->size() - 1)]);
                }
                return text;
            },
            [shared, minimumLength](const std::string &value) {
                return Detail::ShrinkSequence(
                    value,
                    minimumLength,
                    [&shared](char character) {
                        const std::size_t position = shared->find(character);
                        if (position == std::string::npos)
                        {
                            return std::string{};
                        }
                        return shared->substr(0, position);
                    });
            });
    }

} // namespace Progmasoft::Catch3
