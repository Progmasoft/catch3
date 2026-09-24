// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace Progmasoft::Catch3 {

    /// Small, deterministic random source for repeatable property-test cases.
    ///
    /// The SplitMix64 sequence is specified here instead of delegating to a
    /// standard-library engine, whose output sequence is not portable between
    /// library implementations. A Random object is intentionally not
    /// thread-safe; property trials receive independent instances derived from
    /// the root seed.
    class Random final {
    public:
        explicit constexpr Random( std::uint64_t seed ) noexcept:
            state_( seed ) {}

        /// Returns the next value in the stable SplitMix64 sequence.
        [[nodiscard]] constexpr std::uint64_t Next() noexcept {
            state_ += kGamma;
            std::uint64_t value = state_;
            value = ( value ^ ( value >> 30U ) ) * kMultiplierOne;
            value = ( value ^ ( value >> 27U ) ) * kMultiplierTwo;
            return value ^ ( value >> 31U );
        }

        /// Returns a value in [0, bound); bound == 0 requests the full uint64
        /// range.
        [[nodiscard]] constexpr std::uint64_t
        Below( std::uint64_t bound ) noexcept {
            if ( bound == 0 ) { return Next(); }

            // Rejection sampling avoids the modulo bias of a single `% bound`.
            const std::uint64_t threshold =
                ( std::uint64_t{ 0 } - bound ) % bound;
            while ( true ) {
                const std::uint64_t value = Next();
                if ( value >= threshold ) { return value % bound; }
            }
        }

        /// Returns an inclusive integral value without overflowing at type
        /// limits.
        template <std::integral IntegerType>
        requires( !std::same_as<IntegerType, bool> ) [[nodiscard]] IntegerType
            Between( IntegerType minimum, IntegerType maximum ) {
            if ( minimum > maximum ) {
                throw std::invalid_argument(
                    "Random::Between requires minimum <= maximum" );
            }

            using UnsignedType = std::make_unsigned_t<IntegerType>;
            static_assert( std::numeric_limits<UnsignedType>::digits <= 64 );

            constexpr unsigned kBitCount = static_cast<unsigned>(
                std::numeric_limits<UnsignedType>::digits );
            constexpr UnsignedType kSignBit = UnsignedType{ 1 }
                                              << ( kBitCount - 1U );

            const auto encode =
                []( IntegerType value ) constexpr -> UnsignedType {
                if constexpr ( std::is_signed_v<IntegerType> ) {
                    return std::bit_cast<UnsignedType>( value ) ^ kSignBit;
                } else {
                    return value;
                }
            };
            const auto decode =
                []( UnsignedType value ) constexpr -> IntegerType {
                if constexpr ( std::is_signed_v<IntegerType> ) {
                    return std::bit_cast<IntegerType>( value ^ kSignBit );
                } else {
                    return value;
                }
            };

            const UnsignedType lower = encode( minimum );
            const UnsignedType upper = encode( maximum );
            // Unsigned wrap to zero represents the complete value domain.
            const UnsignedType width =
                static_cast<UnsignedType>( upper - lower + UnsignedType{ 1 } );
            const UnsignedType offset =
                width == 0 ? static_cast<UnsignedType>( Next() )
                           : static_cast<UnsignedType>(
                                 Below( static_cast<std::uint64_t>( width ) ) );
            return decode( static_cast<UnsignedType>( lower + offset ) );
        }

        /// Derives an independent, reproducible seed for one trial.
        [[nodiscard]] static constexpr std::uint64_t
        DeriveSeed( std::uint64_t rootSeed,
                    std::uint64_t trialIndex ) noexcept {
            Random trialRandom( rootSeed + kGamma * ( trialIndex + 1U ) );
            return trialRandom.Next();
        }

    private:
        static constexpr std::uint64_t kGamma = 0x9E3779B97F4A7C15ULL;
        static constexpr std::uint64_t kMultiplierOne = 0xBF58476D1CE4E5B9ULL;
        static constexpr std::uint64_t kMultiplierTwo = 0x94D049BB133111EBULL;

        std::uint64_t state_;
    };

} // namespace Progmasoft::Catch3
