
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <Progmasoft/Catch3/Session.hpp>
#include <Progmasoft/Catch3/Internal/Context.hpp>
#include <Progmasoft/Catch3/Internal/Enforce.hpp>
#include <Progmasoft/Catch3/Internal/WindowsHProxy.hpp>
#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>

#include <Progmasoft/Catch3/Runner.hpp>

namespace Catch {

    Session::Session() {
        static bool alreadyInstantiated = false;
        if( alreadyInstantiated ) {
            CATCH_TRY { CATCH_INTERNAL_ERROR( "Only one instance of Catch::Session can ever be used" ); }
            CATCH_CATCH_ALL { getMutableRegistryHub().registerStartupException(); }
        }
        alreadyInstantiated = true;
    }
    Session::~Session() {
        Catch::cleanUp();
    }

    void Session::showHelp() const {
        Progmasoft::Catch3::PrintHelp( m_arguments.empty() ? std::string( "tests" )
                                                           : m_arguments.front() );
    }

    int Session::applyCommandLine( int argc, char const * const * argv ) {
        m_arguments.assign( argv, argv + argc );
        return 0;
    }

#if defined(CATCH_CONFIG_WCHAR) && defined(_WIN32) && defined(UNICODE)
    int Session::applyCommandLine( int argc, wchar_t const * const * argv ) {
        m_arguments.clear();
        for ( int i = 0; i < argc; ++i ) {
            int bufSize = WideCharToMultiByte( CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr );
            std::string utf8( static_cast<std::size_t>( bufSize ), '\0' );
            WideCharToMultiByte( CP_UTF8, 0, argv[i], -1, &utf8[0], bufSize, nullptr, nullptr );
            if ( !utf8.empty() ) { utf8.pop_back(); } // the terminator
            m_arguments.push_back( utf8 );
        }
        return 0;
    }
#endif

    int Session::run() {
        return Progmasoft::Catch3::RunTests( m_arguments );
    }

} // end namespace Catch
