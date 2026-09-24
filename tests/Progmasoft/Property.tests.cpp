// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE( "Progmasoft random sequences are stable and bounded",
           "[progmasoft][random]" ) {
    using Progmasoft::Catch3::Random;

    Random random( 0 );
    CHECK( random.Next() == 0xE220A8397B1DCDAFULL );
    CHECK( random.Next() == 0x6E789E6AA1B965F4ULL );

    Random bounded( 0xA55A );
    for ( int index = 0; index < 1000; ++index ) {
        CHECK( bounded.Below( 17 ) < 17 );
        CHECK( bounded.Between<std::int32_t>( -31, 47 ) >= -31 );
        CHECK( bounded.Between<std::int32_t>( -31, 47 ) <= 47 );
    }

    CHECK_THROWS_AS( bounded.Between<std::int32_t>( 2, 1 ),
                     std::invalid_argument );
}

TEST_CASE( "Progmasoft random supports full-width signed intervals",
           "[progmasoft][random]" ) {
    using Progmasoft::Catch3::Random;

    Random random( 0x12345678 );
    for ( int index = 0; index < 100; ++index ) {
        const std::int64_t value =
            random.Between( std::numeric_limits<std::int64_t>::lowest(),
                            std::numeric_limits<std::int64_t>::max() );
        CHECK( value <= std::numeric_limits<std::int64_t>::max() );
        CHECK( value >= std::numeric_limits<std::int64_t>::lowest() );
    }

    const std::uint64_t first = Random::DeriveSeed( 99, 0 );
    CHECK( first == Random::DeriveSeed( 99, 0 ) );
    CHECK( first != Random::DeriveSeed( 99, 1 ) );
}

TEST_CASE( "Integer generators preserve bounds and useful shrink order",
           "[progmasoft][generator]" ) {
    using Progmasoft::Catch3::Integer;
    using Progmasoft::Catch3::Random;

    const auto generator = Integer<int>( -100, 100 );
    Random random( 0xBADC0DE );
    for ( int index = 0; index < 100; ++index ) {
        const int value = generator.Generate( random );
        CHECK( value >= -100 );
        CHECK( value <= 100 );
    }

    const std::vector<int> positiveShrinks =
        Integer<int>( -100, 100 ).Shrink( 6 );
    REQUIRE( !positiveShrinks.empty() );
    CHECK( positiveShrinks.front() == 0 );
    CHECK( std::ranges::find( positiveShrinks, 5 ) != positiveShrinks.end() );

    const std::vector<int> negativeShrinks = generator.Shrink( -6 );
    CHECK( negativeShrinks.front() == 0 );
    CHECK( std::ranges::find( negativeShrinks, -5 ) != negativeShrinks.end() );

    CHECK_THROWS_AS( Integer<int>( 5, -5 ), std::invalid_argument );
}

TEST_CASE( "Boolean generator shrinks true to false",
           "[progmasoft][generator]" ) {
    using Progmasoft::Catch3::Boolean;
    using Progmasoft::Catch3::Random;

    const auto generator = Boolean();
    Random random( 0xB001 );
    CHECK( generator.Shrink( true ) == std::vector<bool>{ false } );
    CHECK( generator.Shrink( false ).empty() );
    for ( int index = 0; index < 100; ++index ) {
        (void)generator.Generate( random );
    }
}

TEST_CASE( "Properties count trials and reproduce their seed",
           "[progmasoft][property]" ) {
    using Progmasoft::Catch3::CheckProperty;
    using Progmasoft::Catch3::Integer;
    using Progmasoft::Catch3::PropertyOptions;

    int checks = 0;
    const PropertyOptions options{
        .Trials = 64, .Seed = 0xC0FFEE, .MaxShrinkSteps = 100 };
    const auto property = [&]( int value ) {
        ++checks;
        return value >= -100 && value <= 100;
    };

    const auto first =
        CheckProperty( Integer<int>( -100, 100 ), property, options );
    const auto second =
        CheckProperty( Integer<int>( -100, 100 ), property, options );

    CHECK( first.Succeeded() );
    CHECK( first.TrialsRun == options.Trials );
    CHECK( checks == static_cast<int>( options.Trials * 2 ) );
    CHECK( first.Describe() == second.Describe() );
}

TEST_CASE( "Property failures shrink to and report a replayable counterexample",
           "[progmasoft][property]" ) {
    using Progmasoft::Catch3::CheckProperty;
    using Progmasoft::Catch3::MakeGenerator;
    using Progmasoft::Catch3::PropertyOptions;

    const auto generator =
        MakeGenerator<int>( []( Progmasoft::Catch3::Random& ) { return -100; },
                            []( const int& value ) {
                                if ( value == -100 ) {
                                    return std::vector<int>{ -50, -1, 0 };
                                }
                                if ( value == -50 ) {
                                    return std::vector<int>{ -10, -1 };
                                }
                                if ( value == -10 ) {
                                    return std::vector<int>{ -3, -1 };
                                }
                                if ( value == -3 ) {
                                    return std::vector<int>{ -1, 0 };
                                }
                                return std::vector<int>{ 0 };
                            } );

    const auto result = CheckProperty(
        generator,
        []( int value ) { return value >= 0; },
        PropertyOptions{ .Trials = 4, .Seed = 0xFACE, .MaxShrinkSteps = 20 } );

    CHECK_FALSE( result.Succeeded() );
    CHECK( result.Status == Progmasoft::Catch3::PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->Counterexample == "-1" );
    CHECK( result.Failure->RootSeed == 0xFACE );
    CHECK( result.Failure->TrialSeed ==
           Progmasoft::Catch3::Random::DeriveSeed( 0xFACE, 0 ) );
    CHECK( result.Failure->ShrinkSteps > 0 );
    CHECK( result.Describe().find( "counterexample: -1" ) !=
           std::string::npos );
    CHECK( result.Describe().find( "replay seed" ) != std::string::npos );
}

TEST_CASE( "Property shrink work is bounded", "[progmasoft][property]" ) {
    using Progmasoft::Catch3::CheckProperty;
    using Progmasoft::Catch3::MakeGenerator;
    using Progmasoft::Catch3::PropertyOptions;

    const auto generator =
        MakeGenerator<int>( []( Progmasoft::Catch3::Random& ) { return 100; },
                            []( const int& value ) {
                                return value > 1 ? std::vector<int>{ value - 1 }
                                                 : std::vector<int>{};
                            } );
    const auto result = CheckProperty(
        generator,
        []( int ) { return false; },
        PropertyOptions{ .Trials = 1, .Seed = 4, .MaxShrinkSteps = 3 } );

    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->ShrinkSteps == 3 );
    CHECK( result.Failure->Counterexample == "97" );
}

TEST_CASE(
    "Property errors distinguish invalid configuration and generator failures",
    "[progmasoft][property]" ) {
    using Progmasoft::Catch3::CheckProperty;
    using Progmasoft::Catch3::Generator;
    using Progmasoft::Catch3::PropertyOptions;
    using Progmasoft::Catch3::PropertyStatus;

    const Generator<int> throwingGenerator(
        []( Progmasoft::Catch3::Random& ) -> int {
            throw std::runtime_error( "sample unavailable" );
        },
        []( const int& ) { return std::vector<int>{}; } );
    const auto generatorError =
        CheckProperty( throwingGenerator, []( int ) { return true; } );
    CHECK( generatorError.Status == PropertyStatus::kErrored );
    CHECK( generatorError.Describe().find( "sample unavailable" ) !=
           std::string::npos );

    const Generator<int> constantGenerator(
        []( Progmasoft::Catch3::Random& ) { return 1; },
        []( const int& ) { return std::vector<int>{}; } );
    const auto invalidOptions = CheckProperty(
        constantGenerator,
        []( int ) { return true; },
        PropertyOptions{ .Trials = 0, .Seed = 1, .MaxShrinkSteps = 1 } );
    CHECK( invalidOptions.Status == PropertyStatus::kInvalidConfiguration );
}

TEST_CASE( "Predicate exceptions are ordinary reproducible property failures",
           "[progmasoft][property]" ) {
    using Progmasoft::Catch3::CheckProperty;
    using Progmasoft::Catch3::Generator;
    using Progmasoft::Catch3::PropertyStatus;

    const Generator<int> generator(
        []( Progmasoft::Catch3::Random& ) { return 7; },
        []( const int& ) { return std::vector<int>{}; } );
    const auto result = CheckProperty( generator, []( int ) -> bool {
        throw std::runtime_error( "predicate failed" );
    } );

    CHECK( result.Status == PropertyStatus::kFailed );
    REQUIRE( result.Failure.has_value() );
    CHECK( result.Failure->Reason.find( "predicate failed" ) !=
           std::string::npos );
    CHECK( result.Failure->Counterexample == "7" );
}

TEST_CASE( "Catch3 property assertions report successful runs",
           "[progmasoft][property]" ) {
    CATCH3_CHECK_PROPERTY(
        Progmasoft::Catch3::Integer<int>( 1, 100 ),
        []( int value ) { return value > 0; },
        Progmasoft::Catch3::PropertyOptions{
            .Trials = 25, .Seed = 42, .MaxShrinkSteps = 10 } );
}
