// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <Progmasoft/Catch3/Death.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

using Catch::Matchers::ContainsSubstring;
using Progmasoft::Catch3::DeathResult;
using Progmasoft::Catch3::ExitedWith;
using Progmasoft::Catch3::KilledBySignal;
using Progmasoft::Catch3::Detail::RunDeathStatement;

namespace {
    [[noreturn]] void FailWithMessage( const char* message ) {
        std::fputs( message, stderr );
        std::abort();
    }

    int Checked( int value ) {
        if ( value < 0 ) {
            std::fputs( "negative value\n", stderr );
            std::exit( 3 );
        }
        return value;
    }
} // namespace

TEST_CASE( "A statement that aborts dies", "[progmasoft][death]" ) {
    CATCH3_CHECK_DEATH( std::abort() );
    CATCH3_REQUIRE_DEATH( FailWithMessage( "invariant broken\n" ) );
}

TEST_CASE( "What the statement writes to standard error is matched",
           "[progmasoft][death]" ) {
    CATCH3_CHECK_DEATH_THAT( FailWithMessage( "index 7 out of range\n" ),
                             ContainsSubstring( "out of range" ) );
}

TEST_CASE( "How the process ends can be expected", "[progmasoft][death]" ) {
    CATCH3_CHECK_EXIT( static_cast<void>( Checked( -1 ) ), ExitedWith{ 3 } );
    CATCH3_CHECK_EXIT( std::exit( 0 ), ExitedWith{ 0 } );
#if !defined( _WIN32 )
    CATCH3_CHECK_EXIT( std::raise( SIGTERM ), KilledBySignal{ SIGTERM } );
#endif
}

TEST_CASE( "A statement that does not end the process is reported",
           "[progmasoft][death]" ) {
    int value = 0;
    const DeathResult returned =
        RunDeathStatement( __FILE__, __LINE__, [&] { value = Checked( 5 ); } );
    CHECK( returned.kind == DeathResult::Kind::kReturned );
    CHECK_FALSE( returned.Died() );
    CHECK_THAT( returned.Describe(), ContainsSubstring( "returned" ) );
    // The statement ran in the child; this process is untouched.
    CHECK( value == 0 );

    const DeathResult threw = RunDeathStatement(
        __FILE__, __LINE__, [] { throw std::runtime_error( "boom" ); } );
    CHECK( threw.kind == DeathResult::Kind::kThrew );
    CHECK_FALSE( threw.Died() );

    const DeathResult clean =
        RunDeathStatement( __FILE__, __LINE__, [] { std::exit( 0 ); } );
    CHECK( clean.kind == DeathResult::Kind::kEnded );
    CHECK_FALSE( clean.Died() );
    CHECK( ExitedWith{ 0 }( clean ) );
}

TEST_CASE( "Death statements in a loop are told apart", "[progmasoft][death]" ) {
    for ( int code = 1; code <= 3; ++code ) {
        CATCH3_CHECK_EXIT( std::exit( code ), ExitedWith{ code } );
    }
}

TEST_CASE( "Death statements in sections run in their section",
           "[progmasoft][death]" ) {
    SECTION( "the first" ) {
        CATCH3_CHECK_EXIT( std::exit( 4 ), ExitedWith{ 4 } );
    }
    SECTION( "the second" ) {
        CATCH3_CHECK_EXIT( std::exit( 5 ), ExitedWith{ 5 } );
    }
}
