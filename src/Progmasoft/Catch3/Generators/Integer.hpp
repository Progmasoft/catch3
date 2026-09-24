// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Generator.hpp>

#include <algorithm>
#include <concepts>
#include <limits>
#include <type_traits>

namespace Progmasoft::Catch3 {
    namespace Detail {

        template <std::integral IntegerType>
        requires( !std::same_as<IntegerType, bool> )
            [[nodiscard]] std::vector<IntegerType> ShrinkInteger(
                IntegerType value, IntegerType minimum, IntegerType maximum ) {
            std::vector<IntegerType> candidates;
            const auto addCandidate = [&]( IntegerType candidate ) {
                if ( candidate < minimum || candidate > maximum ||
                     candidate == value ||
                     std::ranges::find( candidates, candidate ) !=
                         candidates.end() ) {
                    return;
                }
                candidates.push_back( candidate );
            };

            if ( value == 0 ) { return candidates; }

            // First try the conventional simplest value, then halve toward it.
            // The adjacent value at the end lets the runner find boundary
            // counterexamples (for example, the first failing integer in a
            // threshold property).
            if ( minimum <= 0 && maximum >= 0 ) { addCandidate( 0 ); }

            IntegerType smaller = value;
            while ( smaller != 0 ) {
                smaller = static_cast<IntegerType>( smaller / 2 );
                if ( smaller == 0 ) { break; }
                addCandidate( smaller );
            }

            if ( value > 0 ) {
                if ( value > minimum ) {
                    addCandidate( static_cast<IntegerType>( value - 1 ) );
                }
                if ( minimum > 0 ) { addCandidate( minimum ); }
            } else {
                if ( value < maximum ) {
                    addCandidate( static_cast<IntegerType>( value + 1 ) );
                }
                if ( maximum < 0 ) { addCandidate( maximum ); }
            }
            return candidates;
        }

    } // namespace Detail

    /// Generates integers over the inclusive range and shrinks toward simple
    /// values.
    template <std::integral IntegerType>
    requires( !std::same_as<IntegerType, bool> )
        [[nodiscard]] Generator<IntegerType> Integer(
            IntegerType minimum = std::numeric_limits<IntegerType>::lowest(),
            IntegerType maximum = std::numeric_limits<IntegerType>::max() ) {
        if ( minimum > maximum ) {
            throw std::invalid_argument(
                "Integer requires minimum <= maximum" );
        }

        return MakeGenerator<IntegerType>(
            [minimum, maximum]( Random& random ) {
                return random.Between( minimum, maximum );
            },
            [minimum, maximum]( const IntegerType& value ) {
                return Detail::ShrinkInteger( value, minimum, maximum );
            } );
    }

} // namespace Progmasoft::Catch3
