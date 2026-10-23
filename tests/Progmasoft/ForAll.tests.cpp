// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    using Progmasoft::Catch3::ForAll;
    using Progmasoft::Catch3::PropertyOptions;
    using Progmasoft::Catch3::PropertyStatus;
    using Progmasoft::Catch3::SizeRange;
    namespace Gen = Progmasoft::Catch3::Gen;

    PropertyOptions Quick( std::size_t trials = 200 ) {
        PropertyOptions options;
        options.Trials = trials;
        options.Seed = 0xCA7C3;
        options.MaxShrinkSteps = 5000;
        return options;
    }

} // namespace

TEST_CASE( "ForAll passes and counts its trials",
           "[progmasoft][forall]" ) {
    const auto result = ForAll( Quick( 150 ),
                                Gen::Integers<int>( -100, 100 ),
                                Gen::Integers<int>( -100, 100 ),
                                []( int left, int right ) {
                                    return left + right == right + left;
                                } );
    CHECK( result.Status == PropertyStatus::kPassed );
    CHECK( result.TrialsRun == 150 );
    CHECK( result.Describe() == "property passed: 150 trials" );
}

TEST_CASE( "ForAll shrinks integers to the boundary of the failure",
           "[progmasoft][forall]" ) {
    const auto result = ForAll(
        Quick(), Gen::Integers<int>(), []( int value ) { return value <= 1000; } );
    REQUIRE( result.Status == PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->Counterexample == "1001 (0x3e9)" );
    CHECK( result.Failure->Reason == "property returned false" );
    CHECK( result.Failure->Replay.rfind( "c3.", 0 ) == 0 );
}

TEST_CASE( "ForAll shrinks collections to their essential element",
           "[progmasoft][forall]" ) {
    const auto result = ForAll(
        Quick(),
        Gen::Vectors( Gen::Integers<int>( 0, 100 ) ),
        []( const std::vector<int>& values ) {
            return std::none_of( values.begin(), values.end(),
                                 []( int value ) { return value >= 10; } );
        } );
    REQUIRE( result.Status == PropertyStatus::kFailed );
    CHECK( result.Failure->Counterexample == "{ 10 }" );
}

TEST_CASE( "ForAll shrinks through dependent and filtered strategies",
           "[progmasoft][forall]" ) {
    const auto dependent =
        Gen::Integers<int>( 0, 20 ).FlatMap( []( int size ) {
            return Gen::Vectors(
                Gen::Integers<int>( 0, 100 ),
                SizeRange{ static_cast<std::size_t>( size ),
                           static_cast<std::size_t>( size ) } );
        } );
    const auto sized = ForAll( Quick(), dependent,
                               []( const std::vector<int>& values ) {
                                   return values.size() < 3;
                               } );
    REQUIRE( sized.Status == PropertyStatus::kFailed );
    CHECK( sized.Failure->Counterexample == "{ 0, 0, 0 }" );

    const auto even = Gen::Integers<int>( 0, 100000 ).Filter(
        []( int value ) { return value % 2 == 0; } );
    const auto filtered =
        ForAll( Quick(), even, []( int value ) { return value <= 100; } );
    INFO( filtered.Describe() );
    REQUIRE( filtered.Status == PropertyStatus::kFailed );
    CHECK( filtered.Failure->Counterexample == "102" );

    const auto mapped = ForAll(
        Quick(),
        Gen::Integers<int>( 0, 1000 ).Map( []( int value ) {
            return std::to_string( value );
        } ),
        []( const std::string& text ) { return text.size() < 3; } );
    REQUIRE( mapped.Status == PropertyStatus::kFailed );
    CHECK( mapped.Failure->Counterexample == "\"100\"" );
}

TEST_CASE( "ForAll shrinks several arguments together",
           "[progmasoft][forall]" ) {
    const auto result = ForAll( Quick(),
                                Gen::Integers<int>( 0, 1000 ),
                                Gen::Integers<int>( 0, 1000 ),
                                []( int left, int right ) {
                                    return left + right < 100;
                                } );
    REQUIRE( result.Status == PropertyStatus::kFailed );
    CHECK( result.Failure->Counterexample == "{ 0, 100 }" );
}

TEST_CASE( "A replay token reproduces exactly the reported input",
           "[progmasoft][forall]" ) {
    const auto body = []( const std::vector<int>& values ) {
        return std::accumulate( values.begin(), values.end(), 0 ) < 50;
    };
    const auto strategy = Gen::Vectors( Gen::Integers<int>( 0, 30 ) );
    const auto first = ForAll( Quick(), strategy, body );
    REQUIRE( first.Status == PropertyStatus::kFailed );

    auto options = Quick();
    options.Replay = first.Failure->Replay;
    const auto replayed = ForAll( options, strategy, body );
    REQUIRE( replayed.Status == PropertyStatus::kFailed );
    CHECK( replayed.TrialsRun == 1 );
    CHECK( replayed.Failure->Counterexample == first.Failure->Counterexample );
    CHECK( replayed.Failure->Replay == first.Failure->Replay );
    CHECK( first.Describe().find( "replay: set PropertyOptions::Replay to \"" +
                                  first.Failure->Replay + "\"" ) !=
           std::string::npos );

    // The same options give the same report.
    CHECK( ForAll( Quick(), strategy, body ).Describe() == first.Describe() );

    options.Replay = "not a token";
    CHECK( ForAll( options, strategy, body ).Status ==
           PropertyStatus::kInvalidConfiguration );
}

TEST_CASE( "Property bodies assert, assume and note without exceptions",
           "[progmasoft][forall]" ) {
    const auto failing = ForAll(
        Quick(), Gen::Integers<int>( 0, 1000 ), []( int value ) {
            Progmasoft::Catch3::Note( "half", value / 2 );
            CATCH3_PROPERTY_ASSERT( value < 500 );
        } );
    REQUIRE( failing.Status == PropertyStatus::kFailed );
    CHECK( failing.Failure->Counterexample == "500 (0x1f4)" );
    CHECK( failing.Failure->Reason.find(
               "CATCH3_PROPERTY_ASSERT( value < 500 )" ) == 0 );
    CHECK( failing.Failure->Notes == std::vector<std::string>{ "half = 250" } );

    const auto assumed = ForAll(
        Quick(), Gen::Integers<int>( 0, 9 ), []( int value ) {
            CATCH3_PROPERTY_ASSUME( value % 2 == 0 );
            CATCH3_PROPERTY_ASSERT( value % 2 == 0 );
        } );
    CHECK( assumed.Status == PropertyStatus::kPassed );
    CHECK( assumed.Discarded > 0 );

    auto strict = Quick( 50 );
    strict.MaxDiscardRatio = 1;
    const auto gaveUp = ForAll( strict, Gen::Integers<int>( 0, 9 ),
                                []( int ) { CATCH3_PROPERTY_ASSUME( false ); } );
    CHECK( gaveUp.Status == PropertyStatus::kGaveUp );
    CHECK( gaveUp.Describe().find( "property gave up" ) == 0 );
}

TEST_CASE( "Exceptions from bodies fail and from strategies error",
           "[progmasoft][forall]" ) {
    const auto throwing = ForAll(
        Quick(), Gen::Integers<int>( 0, 100 ), []( int value ) -> bool {
            if ( value > 10 ) { throw std::runtime_error( "too big" ); }
            return true;
        } );
    REQUIRE( throwing.Status == PropertyStatus::kFailed );
    CHECK( throwing.Failure->Counterexample == "11" );
    CHECK( throwing.Failure->Reason ==
           "property threw std::exception: too big" );

    const auto broken = ForAll(
        Quick(),
        Gen::Integers<int>( 0, 10 ).Map( []( int ) -> int {
            throw std::runtime_error( "cannot build" );
        } ),
        []( int ) { return true; } );
    CHECK( broken.Status == PropertyStatus::kErrored );
    CHECK( broken.Error == "strategy threw std::exception: cannot build" );
}

TEST_CASE( "Classify and Cover report and enforce statistics",
           "[progmasoft][forall]" ) {
    auto options = Quick( 400 );
    options.ShowStatistics = true;
    const auto covered = ForAll(
        options, Gen::Integers<int>( -100, 100 ), []( int value ) {
            Progmasoft::Catch3::Classify( value == 0, "zero" );
            Progmasoft::Catch3::Cover( 20.0, value < 0, "negative" );
            return true;
        } );
    CHECK( covered.Status == PropertyStatus::kPassed );
    CHECK( covered.Describe().find( "negative:" ) != std::string::npos );

    const auto uncovered = ForAll(
        Quick( 400 ), Gen::Integers<int>( 0, 1000 ), []( int value ) {
            Progmasoft::Catch3::Cover( 50.0, value == 7, "seven" );
            return true;
        } );
    CHECK( uncovered.Status == PropertyStatus::kInsufficientCoverage );
    CHECK( uncovered.Describe().find( "required 50%" ) != std::string::npos );
}

TEST_CASE( "Targeted search runs after the regular trials",
           "[progmasoft][forall]" ) {
    auto options = Quick( 100 );
    options.TargetSteps = 300;
    const auto result = ForAll(
        options, Gen::Integers<std::int64_t>( 0, 1LL << 40 ), []( std::int64_t value ) {
            Progmasoft::Catch3::Target( static_cast<double>( value ) );
            return value < ( 1LL << 39 ) + ( 1LL << 38 );
        } );
    // Climbing towards the largest values must reach the failing region.
    REQUIRE( result.Status == PropertyStatus::kFailed );
    CHECK( result.Failure->Counterexample == "824633720832 (0xc000000000)" );
}

TEST_CASE( "The example database replays a stored failure first",
           "[progmasoft][forall]" ) {
    const auto directory = std::filesystem::temp_directory_path() /
                           "catch3-property-database-test";
    std::filesystem::remove_all( directory );
    auto options = Quick();
    options.DatabasePath = directory.string();
    options.Name = "database test";
    int threshold = 200;
    const auto body = [&]( int value ) { return value < threshold; };

    const auto first = ForAll( options, Gen::Integers<int>( 0, 1000 ), body );
    REQUIRE( first.Status == PropertyStatus::kFailed );
    CHECK( first.Failure->Counterexample == "200" );
    CHECK( !std::filesystem::is_empty( directory ) );

    // A new seed would need its own trials; the stored example fails at
    // once.
    options.Seed = 0xD1FFE7;
    const auto second = ForAll( options, Gen::Integers<int>( 0, 1000 ), body );
    REQUIRE( second.Status == PropertyStatus::kFailed );
    CHECK( second.TrialsRun == 1 );
    CHECK( second.Failure->Counterexample == "200" );

    // Once fixed, the example passes and is dropped.
    threshold = 2000;
    const auto fixed = ForAll( options, Gen::Integers<int>( 0, 1000 ), body );
    CHECK( fixed.Status == PropertyStatus::kPassed );
    CHECK( std::filesystem::is_empty( directory ) );
    std::filesystem::remove_all( directory );
}

TEST_CASE( "Relations check differential, metamorphic and round-trip oracles",
           "[progmasoft][forall][relations]" ) {
    using namespace Progmasoft::Catch3;
    const auto sorted = []( std::vector<int> values ) {
        std::sort( values.begin(), values.end() );
        return values;
    };
    // A "fast" sort that loses duplicates.
    const auto lossy = []( const std::vector<int>& values ) {
        const std::set<int> unique( values.begin(), values.end() );
        return std::vector<int>( unique.begin(), unique.end() );
    };
    const auto differential = ForAll(
        Quick(), Gen::Vectors( Gen::Integers<int>( 0, 50 ) ),
        Equivalent( sorted, lossy ) );
    REQUIRE( differential.Status == PropertyStatus::kFailed );
    CHECK( differential.Failure->Counterexample == "{ 0, 0 }" );
    CHECK( differential.Failure->Notes ==
           std::vector<std::string>{ "reference: { 0, 0 }",
                                     "candidate: { 0 }" } );

    const auto sum = []( const std::vector<int>& values ) {
        return std::accumulate( values.begin(), values.end(), 0LL );
    };
    const auto reversed = []( std::vector<int> values ) {
        std::reverse( values.begin(), values.end() );
        return values;
    };
    CHECK( ForAll( Quick(), Gen::Vectors( Gen::Integers<int>( -9, 9 ) ),
                   Metamorphic( sum, reversed, std::equal_to<>() ) )
               .Succeeded() );

    const auto encode = []( int value ) { return std::to_string( value ); };
    const auto decode = []( const std::string& text ) {
        return std::stoi( text );
    };
    CHECK( ForAll( Quick(), Gen::Integers<int>(), RoundTrip( encode, decode ) )
               .Succeeded() );
    CHECK( ForAll( Quick(), Gen::Vectors( Gen::Integers<int>( 0, 9 ) ),
                   Idempotent( sorted ) )
               .Succeeded() );
}

TEST_CASE( "CATCH3_CHECK_FOR_ALL reports through Catch2",
           "[progmasoft][forall]" ) {
    CATCH3_CHECK_FOR_ALL( Gen::Integers<int>( 0, 100 ),
                          []( int value ) { return value >= 0; } );
    CATCH3_REQUIRE_FOR_ALL( Quick( 20 ),
                            Gen::Strings( Gen::PrintableCharacters() ),
                            []( const std::string& text ) {
                                return text.size() == text.length();
                            } );
}
