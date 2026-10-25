
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

/**\file
 * Registers custom listener that does not ask for captured stdout/err
 *
 * Running the binary with this listener, and asking for multiple _capturing_
 * reporters (one would be sufficient, but that would also mean depending on
 * implementation details inside Catch2's handling of listeners), we check that
 * nothing is written to stdout, because listeners should not be considered in
 * whether the stdout should be passed-through or not.
 */

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Reporters/EventListener.hpp>
#include <Progmasoft/Catch3/Reporters/Registrars.hpp>

#include <iostream>

namespace {
	class NonCapturingListener : public Catch::EventListenerBase {
    public:
        NonCapturingListener( Catch::IConfig const* config ):
            EventListenerBase( config ) {
            m_preferences.shouldRedirectStdOut = false;
            std::cerr << "X24 - NonCapturingListener initialized.\n";
		}
	};
}

CATCH_REGISTER_LISTENER( NonCapturingListener )

TEST_CASE( "Writes to stdout" ) {
	std::cout << "X24 - FooBarBaz\n";
}
