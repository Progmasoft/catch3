
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#ifndef CATCH_SESSION_HPP_INCLUDED
#define CATCH_SESSION_HPP_INCLUDED

#include <catch2/internal/catch_noncopyable.hpp>
#include <catch2/internal/catch_config_wchar.hpp>

#include <string>
#include <vector>

namespace Catch {

    constexpr int UnspecifiedErrorExitCode = 1;
    constexpr int NoTestsRunExitCode = 2;
    constexpr int UnmatchedTestSpecExitCode = 3;
    constexpr int AllTestsSkippedExitCode = 4;
    constexpr int InvalidTestSpecExitCode = 5;
    constexpr int TestFailureExitCode = 42;

    /**
     * The entry point of a test binary with its own `main`.
     *
     * It hands the command line to the Catch3 runner, which runs the tests
     * in worker processes started from the same binary. Code in `main`
     * before and after `run` therefore also runs in every worker: global
     * set-up belongs there, or in a listener.
     */
    class Session : Detail::NonCopyable {
    public:

        Session();
        ~Session();

        //! Prints the options of the Catch3 runner.
        void showHelp() const;

        //! Keeps the command line for a later `run()`. It is checked by
        //! `run()`, which reports a mistake and returns 1.
        int applyCommandLine( int argc, char const * const * argv );
    #if defined(CATCH_CONFIG_WCHAR) && defined(_WIN32) && defined(UNICODE)
        int applyCommandLine( int argc, wchar_t const * const * argv );
    #endif

        template<typename CharT>
        int run(int argc, CharT const * const argv[]) {
            int returnCode = applyCommandLine(argc, argv);
            if (returnCode == 0)
                returnCode = run();
            return returnCode;
        }

        //! Runs the tests the command line selects and returns the exit
        //! code: 0, or one of the constants above.
        int run();

    private:
        std::vector<std::string> m_arguments;
    };

} // end namespace Catch

#endif // CATCH_SESSION_HPP_INCLUDED
