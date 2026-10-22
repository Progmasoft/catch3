// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    using Progmasoft::Catch3::ChoiceSource;
    using Progmasoft::Catch3::SizeRange;
    using Progmasoft::Catch3::Strategy;
    namespace Gen = Progmasoft::Catch3::Gen;

    template <typename ValueType>
    std::vector<ValueType> Sample( const Strategy<ValueType>& strategy,
                                   std::size_t count,
                                   std::uint64_t seed ) {
        std::vector<ValueType> values;
        for ( std::size_t index = 0; index < count; ++index ) {
            ChoiceSource source( seed + index, index % 101 );
            auto value = strategy.Draw( source );
            if ( value.has_value() ) { values.push_back( *value ); }
        }
        return values;
    }

    // Draws once, then replays the recorded choices: the same value must
    // come back, which is what shrinking and replay tokens rely on.
    template <typename ValueType>
    bool ReplaysIdentically( const Strategy<ValueType>& strategy,
                             std::uint64_t seed ) {
        ChoiceSource original( seed, 60 );
        const auto value = strategy.Draw( original );
        const auto choices = original.Choices();
        ChoiceSource replay( choices, 60 );
        const auto again = strategy.Draw( replay );
        return value == again && replay.Choices() == choices &&
               !replay.Overran();
    }

} // namespace

TEST_CASE( "Choice sources replay, clamp and run out to zero",
           "[progmasoft][strategy]" ) {
    const std::vector<std::uint64_t> recorded{ 3, 99, 1 };
    ChoiceSource replay( recorded, 10 );
    CHECK( replay.Draw( 10 ) == 3 );
    CHECK( replay.Draw( 7 ) == 7 ); // clamped to the new maximum
    CHECK( replay.DrawBoolean( 0.0 ) );
    CHECK_FALSE( replay.Overran() );
    CHECK( replay.Draw( 5 ) == 0 ); // past the end: the simplest choice
    CHECK( replay.Overran() );
    CHECK( replay.Choices() == std::vector<std::uint64_t>{ 3, 7, 1, 0 } );

    ChoiceSource generated( 42, 50 );
    for ( int index = 0; index < 1000; ++index ) {
        CHECK( generated.Draw( 9 ) <= 9 );
        CHECK( generated.DrawBiased( 1000 ) <= 1000 );
    }
    CHECK( generated.Choices().size() == 2000 );
}

TEST_CASE( "Integer strategies respect every kind of range",
           "[progmasoft][strategy]" ) {
    const auto crossing = Gen::Integers<int>( -50, 70 );
    for ( int value : Sample( crossing, 2000, 1 ) ) {
        CHECK( value >= -50 );
        CHECK( value <= 70 );
    }
    const auto negative = Gen::Integers<std::int64_t>( -900, -100 );
    for ( auto value : Sample( negative, 2000, 2 ) ) {
        CHECK( value >= -900 );
        CHECK( value <= -100 );
    }
    const auto positive = Gen::Integers<std::uint8_t>( 10, 20 );
    for ( auto value : Sample( positive, 2000, 3 ) ) {
        CHECK( value >= 10 );
        CHECK( value <= 20 );
    }
    const auto full = Gen::Integers<std::int64_t>();
    const auto values = Sample( full, 4000, 4 );
    CHECK( std::any_of( values.begin(), values.end(), []( auto value ) {
        return value == std::numeric_limits<std::int64_t>::max();
    } ) );
    CHECK( std::any_of( values.begin(), values.end(), []( auto value ) {
        return value < 0;
    } ) );
    const auto upToZero = Gen::Integers<std::int64_t>(
        std::numeric_limits<std::int64_t>::lowest(), 0 );
    for ( auto value : Sample( upToZero, 2000, 5 ) ) { CHECK( value <= 0 ); }

    // The simplest choices give the value closest to zero.
    const std::vector<std::uint64_t> zeros;
    ChoiceSource simplest( zeros, 0 );
    CHECK( crossing.Draw( simplest ) == 0 );
    ChoiceSource simplestNegative( zeros, 0 );
    CHECK( negative.Draw( simplestNegative ) == -100 );
    ChoiceSource simplestPositive( zeros, 0 );
    CHECK( positive.Draw( simplestPositive ) == 10 );

    CHECK_THROWS_AS( Gen::Integers<int>( 2, 1 ), std::invalid_argument );
    for ( std::uint64_t seed = 0; seed < 50; ++seed ) {
        CHECK( ReplaysIdentically( full, seed ) );
    }
}

TEST_CASE( "Float strategies stay finite and in range unless allowed",
           "[progmasoft][strategy]" ) {
    using Progmasoft::Catch3::FloatOptions;
    const auto bounded = Gen::Floats<double>(
        FloatOptions{ .Minimum = -2.5, .Maximum = 1000.0 } );
    for ( double value : Sample( bounded, 3000, 6 ) ) {
        CHECK( std::isfinite( value ) );
        CHECK( value >= -2.5 );
        CHECK( value <= 1000.0 );
    }
    const auto everything = Gen::Floats<double>(
        FloatOptions{ .AllowNaN = true, .AllowInfinity = true } );
    const auto values = Sample( everything, 3000, 7 );
    CHECK( std::any_of( values.begin(), values.end(), []( double value ) {
        return std::isnan( value );
    } ) );
    CHECK( std::any_of( values.begin(), values.end(), []( double value ) {
        return std::isinf( value );
    } ) );
    const auto positive = Gen::Floats<float>(
        FloatOptions{ .Minimum = 1.0, .Maximum = 2.0 } );
    const std::vector<std::uint64_t> zeros;
    ChoiceSource simplest( zeros, 0 );
    CHECK( positive.Draw( simplest ) == 1.0F );
    CHECK_THROWS_AS( Gen::Floats<double>( FloatOptions{ .Minimum = 2.0,
                                                        .Maximum = 1.0 } ),
                     std::invalid_argument );
}

TEST_CASE( "Collection strategies respect their size ranges",
           "[progmasoft][strategy]" ) {
    const auto vectors = Gen::Vectors( Gen::Integers<int>( 0, 9 ),
                                       SizeRange{ 2, 5 } );
    for ( const auto& value : Sample( vectors, 1000, 8 ) ) {
        CHECK( value.size() >= 2 );
        CHECK( value.size() <= 5 );
    }
    const auto unique = Gen::UniqueVectors( Gen::Integers<int>( 0, 30 ),
                                            SizeRange{ 0, 20 } );
    for ( const auto& value : Sample( unique, 1000, 9 ) ) {
        CHECK( std::set<int>( value.begin(), value.end() ).size() ==
               value.size() );
    }
    const auto sets = Gen::Sets( Gen::Integers<int>( 0, 3 ),
                                 SizeRange{ 4, 4 } );
    for ( const auto& value : Sample( sets, 200, 10 ) ) {
        CHECK( value == std::set<int>{ 0, 1, 2, 3 } );
    }
    const auto maps = Gen::Maps( Gen::Integers<int>( 0, 100 ),
                                 Gen::Booleans(),
                                 SizeRange{ 1, 10 } );
    for ( const auto& value : Sample( maps, 500, 11 ) ) {
        CHECK( !value.empty() );
        CHECK( value.size() <= 10 );
    }
    const auto permutations = Gen::Permutations( std::vector<int>{ 1, 2, 3, 4, 5 } );
    for ( auto value : Sample( permutations, 300, 12 ) ) {
        std::sort( value.begin(), value.end() );
        CHECK( value == std::vector<int>{ 1, 2, 3, 4, 5 } );
    }
    const std::vector<std::uint64_t> zeros;
    ChoiceSource simplest( zeros, 0 );
    CHECK( permutations.Draw( simplest ) ==
           std::vector<int>{ 1, 2, 3, 4, 5 } );
    CHECK_THROWS_AS( Gen::Vectors( Gen::Booleans(), SizeRange{ 3, 2 } ),
                     std::invalid_argument );
    for ( std::uint64_t seed = 0; seed < 50; ++seed ) {
        CHECK( ReplaysIdentically( vectors, seed ) );
        CHECK( ReplaysIdentically( maps, seed ) );
    }
}

TEST_CASE( "Text strategies produce valid UTF-8 without surrogates",
           "[progmasoft][strategy]" ) {
    const auto text = Gen::Utf8Strings( SizeRange{ 0, 30 }, 0, 0x10FFFF );
    for ( const auto& value : Sample( text, 1000, 13 ) ) {
        for ( std::size_t index = 0; index + 1 < value.size(); ++index ) {
            const auto lead = static_cast<unsigned char>( value[index] );
            const auto next = static_cast<unsigned char>( value[index + 1] );
            // ED A0..BF would encode a surrogate.
            CHECK_FALSE( ( lead == 0xED && next >= 0xA0 ) );
        }
    }
    const auto printable = Gen::Strings( Gen::PrintableCharacters(),
                                         SizeRange{ 1, 8 } );
    for ( const auto& value : Sample( printable, 500, 14 ) ) {
        CHECK( !value.empty() );
        CHECK( value.size() <= 8 );
        CHECK( std::all_of( value.begin(), value.end(), []( char c ) {
            return c >= 0x20 && c < 0x7F;
        } ) );
    }
}

TEST_CASE( "Combinators compose and keep replays stable",
           "[progmasoft][strategy]" ) {
    const auto even = Gen::Integers<int>( 0, 1000 ).Filter(
        []( int value ) { return value % 2 == 0; } );
    for ( int value : Sample( even, 500, 15 ) ) { CHECK( value % 2 == 0 ); }

    const auto sized = Gen::Integers<int>( 0, 6 ).FlatMap( []( int size ) {
        return Gen::Vectors( Gen::Just( size ),
                             SizeRange{ static_cast<std::size_t>( size ),
                                        static_cast<std::size_t>( size ) } );
    } );
    for ( const auto& value : Sample( sized, 500, 16 ) ) {
        CHECK( std::all_of( value.begin(), value.end(), [&]( int element ) {
            return element == static_cast<int>( value.size() );
        } ) );
    }

    const auto mixed = Gen::OneOf( Gen::Just( std::string( "a" ) ),
                                   Gen::Just( std::string( "b" ) ) );
    const auto mixedValues = Sample( mixed, 200, 17 );
    CHECK( std::count( mixedValues.begin(), mixedValues.end(), "a" ) > 0 );
    CHECK( std::count( mixedValues.begin(), mixedValues.end(), "b" ) > 0 );

    const auto weighted = Gen::Frequency<int>( { { 1, Gen::Just( 0 ) },
                                                 { 9, Gen::Just( 1 ) } } );
    const auto weightedValues = Sample( weighted, 2000, 18 );
    CHECK( std::count( weightedValues.begin(), weightedValues.end(), 1 ) >
           std::count( weightedValues.begin(), weightedValues.end(), 0 ) );

    const auto trees = Gen::Recursive(
        Gen::Just( 1 ),
        []( const Strategy<int>& children ) {
            return Gen::Tuples( children, children )
                .Map( []( std::tuple<int, int> pair ) {
                    return std::get<0>( pair ) + std::get<1>( pair );
                } );
        },
        4 );
    for ( int leaves : Sample( trees, 500, 19 ) ) {
        CHECK( leaves >= 1 );
        CHECK( leaves <= 16 ); // depth 4 at most: 2^4 leaves
    }

    const auto never = Gen::Integers<int>( 0, 10 ).Filter(
        []( int ) { return false; }, 5 );
    ChoiceSource source( 1, 50 );
    CHECK_FALSE( never.Draw( source ).has_value() );
}

TEST_CASE( "Replay tokens round-trip and reject malformed text",
           "[progmasoft][strategy]" ) {
    using Progmasoft::Catch3::Detail::DecodeChoices;
    using Progmasoft::Catch3::Detail::EncodeChoices;
    const std::vector<std::uint64_t> choices{
        0, 1, 35, 36, std::numeric_limits<std::uint64_t>::max() };
    const auto token = EncodeChoices( 73, choices );
    CHECK( token == "c3.21.0-1-z-10-3w5e11264sgsf" );
    const auto decoded = DecodeChoices( token );
    REQUIRE( decoded.has_value() );
    CHECK( decoded->Size == 73 );
    CHECK( decoded->Choices == choices );

    const auto empty = DecodeChoices( EncodeChoices( 0, {} ) );
    REQUIRE( empty.has_value() );
    CHECK( empty->Choices.empty() );

    for ( const char* malformed :
          { "", "c3.", "c3.1", "x3.1.2", "c3.1.2-", "c3.1.-2", "c3.2t.1",
            "c3.1.2--3", "c3.1.zzzzzzzzzzzzzzz" } ) {
        CHECK_FALSE( DecodeChoices( malformed ).has_value() );
    }
}

TEST_CASE( "The choice shrinker finds simple failing sequences",
           "[progmasoft][strategy]" ) {
    using Progmasoft::Catch3::Detail::IsSimpler;
    using Progmasoft::Catch3::Detail::ShrinkChoices;
    CHECK( IsSimpler( std::vector<std::uint64_t>{ 9 },
                      std::vector<std::uint64_t>{ 0, 0 } ) );
    CHECK( IsSimpler( std::vector<std::uint64_t>{ 1, 9 },
                      std::vector<std::uint64_t>{ 2, 0 } ) );
    CHECK_FALSE( IsSimpler( std::vector<std::uint64_t>{ 2, 0 },
                            std::vector<std::uint64_t>{ 2, 0 } ) );

    // Fails when the first two choices sum to at least 10.
    const auto sumAtLeastTen = []( const std::vector<std::uint64_t>& choices )
        -> std::optional<std::vector<std::uint64_t>> {
        std::vector<std::uint64_t> consumed = choices;
        consumed.resize( 2, 0 );
        if ( consumed[0] + consumed[1] >= 10 ) { return consumed; }
        return std::nullopt;
    };
    std::size_t calls = 0;
    const auto minimized = ShrinkChoices(
        { 7, 9, 4, 4, 1 }, 10000, sumAtLeastTen, calls );
    // Shortlex: two choices, the first as small as the sum allows.
    CHECK( minimized == std::vector<std::uint64_t>{ 0, 10 } );
    CHECK( calls > 0 );

    std::size_t limitedCalls = 0;
    (void)ShrinkChoices( { 7, 9, 4, 4, 1 }, 3, sumAtLeastTen, limitedCalls );
    CHECK( limitedCalls <= 3 );
}
