
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

/**\file
 * Test that the captured stdout/err in (partial) testCaseEnded events
 * is correct (e.g. that the partial test case event does not get accumulated
 * output).
 *
 * This is done by having a single test case that is entered multiple
 * times through generator, and a capturing listener that knows what it
 * should expect captured from the test case. It counts what differs and
 * prints the count when the run ends.
 */


#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>
#include <string>

namespace {
class TestListener : public Catch::EventListenerBase {
    std::string stdOutString( uint64_t iter ){
        return "stdout " + std::to_string( iter ) + '\n';
    }
    std::string stdErrString(uint64_t iter) {
        return "stderr " + std::to_string( iter ) + '\n';
    }

    int m_errors = 0;

public:
    TestListener( Catch::IConfig const* config ):
        EventListenerBase( config ) {
        m_preferences.shouldRedirectStdOut = true;
    }

    void testCasePartialEnded( Catch::TestCaseStats const& stats,
                               uint64_t iter ) override {
        if ( stats.stdOut != stdOutString( iter ) ) {
            ++m_errors;
        }
        if ( stats.stdErr != stdErrString( iter ) ) {
            ++m_errors;
        }
    }

    void testCaseEnded( Catch::TestCaseStats const& stats ) override {
        if ( stats.stdOut != "stdout 0\nstdout 1\nstdout 2\nstdout 3\nstdout 4\nstdout 5\n" ) {
            ++m_errors;
        }
        if ( stats.stdErr != "stderr 0\nstderr 1\nstderr 2\nstderr 3\nstderr 4\nstderr 5\n" ) {
            ++m_errors;
        }
    }

    void testRunEnded( Catch::TestRunStats const& ) override {
        std::cerr << "X27 - listener saw " << m_errors << " error(s)\n";
    }
};
} // namespace

CATCH_REGISTER_LISTENER( TestListener )

TEST_CASE( "repeatedly entered test case" ) {
    auto i = GENERATE( range(0, 6) );
    std::cout << "stdout " << i << '\n';
    // Switch between writing to std::cerr and std::clog just to make sure
    // both are properly captured and redirected.
    ( ( i % 2 == 0 ) ? std::cerr : std::clog ) << "stderr " << i << '\n';
}
