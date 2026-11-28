// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <Progmasoft/Catch3/Message.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Matchers/Matchers.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace Progmasoft::Catch3 {

    /// How the statement of a death test ended, as the parent saw it.
    struct DeathResult final {
        enum class Kind : std::uint8_t {
            /// The process ended inside the statement; `status` says how.
            kEnded,
            /// The statement returned, so the process did not die.
            kReturned,
            /// The statement threw, so the process did not die.
            kThrew,
            /// The child ran the test but never reached the statement.
            kNotReached,
            /// The child could not be started; `errors` says why.
            kNotStarted,
            /// In the child of another death test: not run.
            kSkipped,
        };

        /// How a process ended.
        enum class Ending : std::uint8_t {
            /// It called exit or returned from main; `code` is the status.
            kExited,
            /// A signal ended it (POSIX); `code` is the signal number.
            kSignaled,
            /// A structured exception ended it (Windows); `code` is the
            /// exception code.
            kException,
        };

        Kind kind = Kind::kNotStarted;
        Ending ending = Ending::kExited;
        std::int64_t code = 0;
        /// What the statement wrote to standard error.
        std::string errors;

        /// Whether the process died in the statement: it exited with a
        /// status other than 0, or a signal or an exception ended it.
        [[nodiscard]] bool Died() const noexcept;

        /// A sentence for the report, such as "the statement ended the
        /// process with signal 6 (SIGABRT)".
        [[nodiscard]] std::string Describe() const;
    };

    /// Expects the process to exit with `code`.
    struct ExitedWith final {
        std::int64_t code;

        [[nodiscard]] bool operator()( const DeathResult& result ) const;
        [[nodiscard]] std::string Describe() const;
    };

    /// Expects a signal to end the process (POSIX), such as SIGABRT.
    struct KilledBySignal final {
        int signal;

        [[nodiscard]] bool operator()( const DeathResult& result ) const;
        [[nodiscard]] std::string Describe() const;
    };

    namespace Detail {
        /// Runs `statement` in a child: the test binary runs the current test
        /// again in a process of its own and stops it at this statement. In
        /// that child, this runs the statement and ends the process; another
        /// death statement of the same test is skipped there.
        [[nodiscard]] DeathResult RunDeathStatement(
            const char* file,
            int line,
            const std::function<void()>& statement );
    } // namespace Detail

} // namespace Progmasoft::Catch3

#define CATCH3_INTERNAL_DEATH( result, ... )                                 \
    const ::Progmasoft::Catch3::DeathResult result =                         \
        ::Progmasoft::Catch3::Detail::RunDeathStatement(                     \
            __FILE__, __LINE__, [&]() { __VA_ARGS__; } );                    \
    INFO( result.Describe() )

/// Checks that `statement` ends the process: it exits with a status other
/// than 0, or a signal or an exception ends it. The statement runs in a child
/// process, so the test goes on either way.
#define CATCH3_CHECK_DEATH( ... )                                            \
    do {                                                                     \
        CATCH3_INTERNAL_DEATH( catch3Death, __VA_ARGS__ );                   \
        CHECK( catch3Death.Died() );                                         \
    } while ( false )

/// As CATCH3_CHECK_DEATH, but ends the test case on failure.
#define CATCH3_REQUIRE_DEATH( ... )                                          \
    do {                                                                     \
        CATCH3_INTERNAL_DEATH( catch3Death, __VA_ARGS__ );                   \
        REQUIRE( catch3Death.Died() );                                       \
    } while ( false )

/// As CATCH3_CHECK_DEATH, and what the statement wrote to standard error
/// matches `matcher`, such as ContainsSubstring( "out of range" ).
#define CATCH3_CHECK_DEATH_THAT( statement, matcher )                        \
    do {                                                                     \
        CATCH3_INTERNAL_DEATH( catch3Death, statement );                     \
        CHECK( catch3Death.Died() );                                         \
        if ( catch3Death.kind !=                                             \
             ::Progmasoft::Catch3::DeathResult::Kind::kSkipped ) {           \
            CHECK_THAT( catch3Death.errors, matcher );                       \
        }                                                                    \
    } while ( false )

/// As CATCH3_CHECK_DEATH_THAT, but ends the test case on failure.
#define CATCH3_REQUIRE_DEATH_THAT( statement, matcher )                      \
    do {                                                                     \
        CATCH3_INTERNAL_DEATH( catch3Death, statement );                     \
        REQUIRE( catch3Death.Died() );                                       \
        if ( catch3Death.kind !=                                             \
             ::Progmasoft::Catch3::DeathResult::Kind::kSkipped ) {           \
            REQUIRE_THAT( catch3Death.errors, matcher );                     \
        }                                                                    \
    } while ( false )

/// Checks how `statement` ends the process: `ending` is ExitedWith( code )
/// or KilledBySignal( signal ), or any callable that takes a DeathResult.
/// Unlike CATCH3_CHECK_DEATH, an exit with status 0 can be expected.
#define CATCH3_CHECK_EXIT( statement, ending )                               \
    do {                                                                     \
        CATCH3_INTERNAL_DEATH( catch3Death, statement );                     \
        CHECK( ( catch3Death.kind ==                                         \
                     ::Progmasoft::Catch3::DeathResult::Kind::kSkipped ||    \
                 ( catch3Death.kind ==                                       \
                       ::Progmasoft::Catch3::DeathResult::Kind::kEnded &&    \
                   ( ending )( catch3Death ) ) ) );                          \
    } while ( false )

/// As CATCH3_CHECK_EXIT, but ends the test case on failure.
#define CATCH3_REQUIRE_EXIT( statement, ending )                             \
    do {                                                                     \
        CATCH3_INTERNAL_DEATH( catch3Death, statement );                     \
        REQUIRE( ( catch3Death.kind ==                                       \
                       ::Progmasoft::Catch3::DeathResult::Kind::kSkipped ||  \
                   ( catch3Death.kind ==                                     \
                         ::Progmasoft::Catch3::DeathResult::Kind::kEnded &&  \
                     ( ending )( catch3Death ) ) ) );                        \
    } while ( false )
