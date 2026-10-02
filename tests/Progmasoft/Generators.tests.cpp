// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

    using Progmasoft::Catch3::CheckProperty;
    using Progmasoft::Catch3::Element;
    using Progmasoft::Catch3::Integer;
    using Progmasoft::Catch3::PropertyOptions;
    using Progmasoft::Catch3::PropertyStatus;
    using Progmasoft::Catch3::Random;
    using Progmasoft::Catch3::Text;
    using Progmasoft::Catch3::Tuple;
    using Progmasoft::Catch3::Vector;

} // namespace

TEST_CASE( "Vector generators respect size and element bounds",
           "[progmasoft][generator][vector]" ) {
    const auto generator = Vector( Integer<int>( -5, 5 ), 2, 9 );
    Random random( 0x51A7E );
    bool sawMinimum = false;
    bool sawMaximum = false;
    for ( int index = 0; index < 500; ++index ) {
        const std::vector<int> values = generator.Generate( random );
        REQUIRE( values.size() >= 2 );
        REQUIRE( values.size() <= 9 );
        sawMinimum = sawMinimum || values.size() == 2;
        sawMaximum = sawMaximum || values.size() == 9;
        for ( const int value : values ) {
            REQUIRE( value >= -5 );
            REQUIRE( value <= 5 );
        }
    }
    // Both ends of the inclusive size range are reachable.
    CHECK( sawMinimum );
    CHECK( sawMaximum );

    Random first( 77 );
    Random second( 77 );
    CHECK( generator.Generate( first ) == generator.Generate( second ) );
}

TEST_CASE( "Vector shrinking shortens first and never drops below its minimum",
           "[progmasoft][generator][vector]" ) {
    const auto generator = Vector( Integer<int>( 0, 100 ), 2, 16 );
    const std::vector<int> value{ 9, 8, 7, 6, 5 };
    const auto candidates = generator.Shrink( value );

    REQUIRE( candidates.size() >= 4 );
    // Shortest allowed prefix, then the two halves, then single removals.
    CHECK( candidates[0] == std::vector<int>{ 9, 8 } );
    CHECK( candidates[1] == std::vector<int>{ 9, 8, 7 } );
    CHECK( candidates[2] == std::vector<int>{ 7, 6, 5 } );
    CHECK( candidates[3] == std::vector<int>{ 8, 7, 6, 5 } );
    for ( const auto& candidate : candidates ) {
        CHECK( candidate.size() >= 2 );
        CHECK( candidate.size() <= value.size() );
        CHECK( candidate != value );
    }
    // Element simplification keeps the length and changes one position.
    CHECK( std::find( candidates.begin(),
                      candidates.end(),
                      std::vector<int>{ 0, 8, 7, 6, 5 } ) !=
           candidates.end() );

    // A value already at its minimum length only simplifies elements.
    for ( const auto& candidate :
          generator.Shrink( std::vector<int>{ 3, 0 } ) ) {
        CHECK( candidate.size() == 2 );
    }
    CHECK( generator.Shrink( std::vector<int>{ 0, 0 } ).empty() );
}

TEST_CASE( "Vector shrinking offers a bounded number of candidates",
           "[progmasoft][generator][vector]" ) {
    const auto generator = Vector( Integer<int>( 0, 1000 ), 0, 5000 );
    const std::vector<int> large( 4000, 999 );
    const auto candidates = generator.Shrink( large );
    CHECK( candidates.size() ==
           Progmasoft::Catch3::Detail::kMaximumSequenceCandidates );
    CHECK( candidates.front().empty() );
}

TEST_CASE( "Vector properties shrink to a small counterexample",
           "[progmasoft][property][vector]" ) {
    const auto result = CheckProperty(
        Vector( Integer<int>( 0, 100 ), 0, 20 ),
        []( const std::vector<int>& values ) {
            return std::accumulate( values.begin(), values.end(), 0 ) < 10;
        },
        PropertyOptions{ .Trials = 200, .Seed = 0xF00D } );

    REQUIRE( result.Status == PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    // The smallest failing input is one element equal to the threshold.
    CHECK( result.Failure->Counterexample == "{ 10 }" );
}

TEST_CASE( "Vector<bool> values generate, shrink and print",
           "[progmasoft][generator][vector]" ) {
    const auto generator = Vector( Progmasoft::Catch3::Boolean(), 0, 8 );
    const auto result = CheckProperty(
        generator,
        []( const std::vector<bool>& flags ) {
            return std::count( flags.begin(), flags.end(), true ) < 2;
        },
        PropertyOptions{ .Trials = 300, .Seed = 0xB001 } );
    REQUIRE( result.Status == PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->Counterexample == "{ true, true }" );
}

TEST_CASE( "Tuple generators draw components left to right",
           "[progmasoft][generator][tuple]" ) {
    const auto left = Integer<int>( 0, 1000 );
    const auto right = Integer<std::int64_t>( -9, 9 );
    const auto generator = Tuple( left, right, Progmasoft::Catch3::Boolean() );

    Random combined( 0xABCDEF );
    Random manual( 0xABCDEF );
    for ( int index = 0; index < 50; ++index ) {
        const auto value = generator.Generate( combined );
        // The same seed consumed by hand in declaration order.
        const int expectedLeft = left.Generate( manual );
        const std::int64_t expectedRight = right.Generate( manual );
        const bool expectedFlag = ( manual.Next() & 1U ) != 0;
        CHECK( std::get<0>( value ) == expectedLeft );
        CHECK( std::get<1>( value ) == expectedRight );
        CHECK( std::get<2>( value ) == expectedFlag );
    }
}

TEST_CASE( "Tuple shrinking changes one component at a time, first to last",
           "[progmasoft][generator][tuple]" ) {
    const auto generator =
        Tuple( Integer<int>( 0, 100 ), Integer<int>( 0, 100 ) );
    const std::tuple<int, int> value{ 4, 6 };
    const auto candidates = generator.Shrink( value );

    const auto firstCandidates = Integer<int>( 0, 100 ).Shrink( 4 );
    const auto secondCandidates = Integer<int>( 0, 100 ).Shrink( 6 );
    REQUIRE( candidates.size() ==
             firstCandidates.size() + secondCandidates.size() );
    for ( std::size_t index = 0; index < firstCandidates.size(); ++index ) {
        CHECK( candidates[index] ==
               std::tuple<int, int>{ firstCandidates[index], 6 } );
    }
    for ( std::size_t index = 0; index < secondCandidates.size(); ++index ) {
        CHECK( candidates[firstCandidates.size() + index] ==
               std::tuple<int, int>{ 4, secondCandidates[index] } );
    }
    CHECK( generator.Shrink( std::tuple<int, int>{ 0, 0 } ).empty() );
}

TEST_CASE( "Tuple properties minimize every component",
           "[progmasoft][property][tuple]" ) {
    const auto result = CheckProperty(
        Tuple( Integer<int>( 0, 1000 ), Integer<int>( 0, 1000 ) ),
        []( const std::tuple<int, int>& value ) {
            return std::get<0>( value ) < 50 || std::get<1>( value ) < 50;
        },
        PropertyOptions{ .Trials = 500, .Seed = 0x7007 } );

    REQUIRE( result.Status == PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->Counterexample == "{ 50, 50 }" );
}

TEST_CASE( "Element generators pick listed values and shrink toward the front",
           "[progmasoft][generator][element]" ) {
    const auto generator = Element<std::string>( { "none", "low", "high" } );
    Random random( 0xE1E );
    std::vector<int> seen( 3, 0 );
    for ( int index = 0; index < 300; ++index ) {
        const std::string value = generator.Generate( random );
        if ( value == "none" ) {
            ++seen[0];
        } else if ( value == "low" ) {
            ++seen[1];
        } else {
            REQUIRE( value == "high" );
            ++seen[2];
        }
    }
    CHECK( seen[0] > 0 );
    CHECK( seen[1] > 0 );
    CHECK( seen[2] > 0 );

    CHECK( generator.Shrink( "none" ).empty() );
    CHECK( generator.Shrink( "low" ) == std::vector<std::string>{ "none" } );
    CHECK( generator.Shrink( "high" ) ==
           std::vector<std::string>{ "none", "low" } );
    // A value outside the list has no simpler listed form.
    CHECK( generator.Shrink( "other" ).empty() );

    const auto single = Element<int>( { 7 } );
    CHECK( single.Generate( random ) == 7 );
}

TEST_CASE( "Text generators stay inside their alphabet and length range",
           "[progmasoft][generator][text]" ) {
    const auto generator = Text( "abc", 1, 6 );
    Random random( 0x7E87 );
    for ( int index = 0; index < 300; ++index ) {
        const std::string value = generator.Generate( random );
        REQUIRE( value.size() >= 1 );
        REQUIRE( value.size() <= 6 );
        CHECK( value.find_first_not_of( "abc" ) == std::string::npos );
    }

    const std::string printable = Progmasoft::Catch3::PrintableAscii();
    CHECK( printable.size() == 95 );
    CHECK( printable.front() == 'a' );
    std::string sorted = printable;
    std::sort( sorted.begin(), sorted.end() );
    CHECK( std::adjacent_find( sorted.begin(), sorted.end() ) ==
           sorted.end() );
    CHECK( sorted.front() == ' ' );
    CHECK( sorted.back() == '~' );
}

TEST_CASE( "Text properties shrink to the shortest simplest string",
           "[progmasoft][property][text]" ) {
    const auto result = CheckProperty(
        Text( "abcxyz", 0, 24 ),
        []( const std::string& value ) {
            return value.find( 'x' ) == std::string::npos;
        },
        PropertyOptions{ .Trials = 300, .Seed = 0x7E47 } );

    REQUIRE( result.Status == PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->Counterexample == "\"x\"" );

    // Characters simplify toward the front of the alphabet.
    const auto candidates = Text( "abc", 2, 4 ).Shrink( "cb" );
    CHECK( candidates ==
           std::vector<std::string>{ "ab", "bb", "ca" } );
}

TEST_CASE( "A reported replay seed reproduces exactly that trial",
           "[progmasoft][property][replay]" ) {
    const auto generator =
        Tuple( Integer<int>( 0, 1000 ), Vector( Integer<int>( 0, 9 ), 0, 6 ) );
    const auto predicate =
        []( const std::tuple<int, std::vector<int>>& value ) {
            return std::get<0>( value ) < 600 ||
                   std::get<1>( value ).size() < 2;
        };

    const auto original = CheckProperty(
        generator, predicate, PropertyOptions{ .Trials = 500, .Seed = 0x5EED } );
    REQUIRE( original.Status == PropertyStatus::kFailed );
    REQUIRE( original.Failure.has_value() );
    // The failure was not on the first trial, so replay skips real work.
    REQUIRE( original.Failure->Trial > 1 );
    CHECK( original.Describe().find(
               "replay: set PropertyOptions::ReplayTrialSeed to " +
               std::to_string( original.Failure->TrialSeed ) ) !=
           std::string::npos );

    // A different root seed and trial count prove that only the replay seed
    // decides what runs.
    const auto replayed = CheckProperty(
        generator,
        predicate,
        PropertyOptions{ .Trials = 3,
                         .Seed = 1,
                         .ReplayTrialSeed = original.Failure->TrialSeed } );
    REQUIRE( replayed.Status == PropertyStatus::kFailed );
    REQUIRE( replayed.Failure.has_value() );
    CHECK( replayed.TrialsRun == 1 );
    CHECK( replayed.Failure->Trial == 1 );
    CHECK( replayed.Failure->TrialSeed == original.Failure->TrialSeed );
    CHECK( replayed.Failure->Counterexample ==
           original.Failure->Counterexample );
    CHECK( replayed.Failure->ShrinkSteps == original.Failure->ShrinkSteps );

    // Replaying a seed whose trial passes reports one passing trial.
    const auto passing = CheckProperty(
        Integer<int>( 0, 10 ),
        []( int value ) { return value <= 10; },
        PropertyOptions{ .Trials = 50, .ReplayTrialSeed = 12345 } );
    CHECK( passing.Succeeded() );
    CHECK( passing.TrialsRun == 1 );
}

TEST_CASE( "Counterexamples print nested structures without Catch2 macros",
           "[progmasoft][property][printing]" ) {
    using Progmasoft::Catch3::Detail::StringifyPropertyValue;

    CHECK( StringifyPropertyValue( std::vector<int>{} ) == "{}" );
    CHECK( StringifyPropertyValue( std::vector<int>{ 1, 2, 3 } ) ==
           "{ 1, 2, 3 }" );
    CHECK( StringifyPropertyValue( std::tuple<>{} ) == "{}" );
    CHECK( StringifyPropertyValue( std::pair<int, bool>{ 4, false } ) ==
           "{ 4, false }" );
    CHECK( StringifyPropertyValue(
               std::tuple<int, std::vector<bool>, std::string>{
                   7, { true, false }, "text" } ) ==
           "{ 7, { true, false }, \"text\" }" );
    CHECK( StringifyPropertyValue(
               std::vector<std::tuple<int, int>>{ { 1, 2 }, { 3, 4 } } ) ==
           "{ { 1, 2 }, { 3, 4 } }" );
}

TEST_CASE( "Generator factories reject impossible configurations",
           "[progmasoft][generator]" ) {
    CHECK_THROWS_AS( Vector( Integer<int>( 0, 1 ), 3, 2 ),
                     std::invalid_argument );
    CHECK_THROWS_AS(
        Vector( Integer<int>( 0, 1 ),
                0,
                Progmasoft::Catch3::Detail::kMaximumSequenceLength + 1 ),
        std::invalid_argument );
    CHECK_THROWS_AS( Element( std::vector<int>{} ), std::invalid_argument );
    CHECK_THROWS_AS( Text( "", 0, 4 ), std::invalid_argument );
    CHECK_THROWS_AS( Text( "ab", 5, 4 ), std::invalid_argument );
    CHECK_THROWS_AS(
        Text( "ab",
              0,
              Progmasoft::Catch3::Detail::kMaximumSequenceLength + 1 ),
        std::invalid_argument );
}
