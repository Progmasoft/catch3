
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

/**\file
 * Test that a listener receives the events of a test run in the process
 * that runs the tests: the start and the end of the run, and the start and
 * the end of an assertion.
 *
 * We only do this for a subset of the events, as doing all of them would
 * be annoying, and we can assume that their implementation is roughly
 * the same, and thus if few work, all work.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>

namespace {

    class TestListener : public Catch::EventListenerBase {
        bool m_testRunStarting = false;
        bool m_assertionStarting = false;
        bool m_assertionEnded = false;

    public:
        TestListener( Catch::IConfig const* config ):
            EventListenerBase( config ) {
            m_preferences.shouldReportAllAssertions = true;
        }

        void testRunStarting( Catch::TestRunInfo const& ) override {
            m_testRunStarting = true;
        }

        void assertionStarting( Catch::AssertionInfo const& ) override {
            if ( !m_testRunStarting ) {
                std::cerr << "X28 - ERROR: assertion before the run started\n";
            }
            m_assertionStarting = true;
        }

        void assertionEnded( Catch::AssertionStats const& ) override {
            if ( !m_assertionStarting ) {
                std::cerr << "X28 - ERROR: assertion ended before it started\n";
            }
            m_assertionEnded = true;
        }

        void testRunEnded( Catch::TestRunStats const& ) override {
            if ( m_testRunStarting && m_assertionStarting && m_assertionEnded ) {
                std::cerr << "X28 - listener received every event\n";
            } else {
                std::cerr << "X28 - ERROR: an event is missing\n";
            }
        }
    };

} // end unnamed namespace

CATCH_REGISTER_LISTENER( TestListener )

TEST_CASE( "Dummy test case" ) { REQUIRE( 1 == 1 ); }
