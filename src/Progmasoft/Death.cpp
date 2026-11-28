// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <Progmasoft/Catch3/Death.hpp>

#include <Progmasoft/Catch3/Interfaces/IResultCapture.hpp>
#include <Progmasoft/Catch3/Internal/CompilerCapabilities.hpp>
#include <Progmasoft/Runner/Hash.hpp>
#include <Progmasoft/Runner/Process.hpp>

#include <array>
#include <cctype>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string_view>
#include <utility>

namespace Progmasoft::Catch3 {

    namespace {

        /// The variable that turns a run of the test binary into the child of
        /// one death statement; its value names the statement.
        constexpr const char* kStatementVariable = "CATCH3_DEATH_STATEMENT";

        // What the child writes to standard error around the statement. Each
        // starts a line, so text the statement wrote before cannot join it.
        constexpr std::string_view kReached = "\n[catch3-death:reached]\n";
        constexpr std::string_view kReturned = "\n[catch3-death:returned]\n";
        constexpr std::string_view kThrew = "\n[catch3-death:threw]\n";

        /// The status the child ends with when the statement returned or
        /// threw; the markers above, not this number, say what happened.
        constexpr int kChildSurvived = 0;

        /// Names each death statement of the current test by its place and
        /// by how often the test has reached that place, so that a statement
        /// in a loop or a section gets the same name in the child as in the
        /// parent.
        std::string NameStatement( const char* file, int line ) {
            // How often the current test reached each place, by a hash of the
            // place. The table holds plain numbers, so it needs no destructor
            // at exit; a test with more death statements than it has rows
            // counts the extra ones from the last row, which still names each
            // one the same way in the child as in the parent.
            struct Seen final {
                std::uint64_t place;
                unsigned count;
            };
            static thread_local std::uint64_t test = 0;
            static thread_local std::array<Seen, 64> seen{};
            static thread_local std::size_t used = 0;

            const std::uint64_t current = Runner::Fnv1a64(
                ::Catch::getResultCapture().getCurrentTestName() );
            if ( current != test ) {
                test = current;
                used = 0;
            }
            std::string place =
                std::string( file ) + ":" + std::to_string( line );
            const std::uint64_t key = Runner::Fnv1a64( place );
            std::size_t row = 0;
            while ( row < used && seen[row].place != key ) {
                ++row;
            }
            if ( row == used ) {
                if ( used < seen.size() ) {
                    ++used;
                } else {
                    row = seen.size() - 1;
                }
                seen[row] = Seen{ key, 0 };
            }
            const unsigned count = ++seen[row].count;
            return place + "#" + std::to_string( count );
        }

        /// A test spec that matches exactly this name: every character
        /// that is not a letter or a digit is escaped with a backslash.
        std::string ExactSpec( const std::string& name ) {
            std::string spec;
            spec.reserve( name.size() * 2 );
            for ( const char character : name ) {
                if ( !std::isalnum( static_cast<unsigned char>( character ) ) ) {
                    spec += '\\';
                }
                spec += character;
            }
            return spec;
        }

        void WriteError( std::string_view text ) {
            std::fwrite( text.data(), 1, text.size(), stderr );
            std::fflush( stderr );
        }

        /// Gives the fatal signals back their default actions. The test
        /// runner reports a crash from a handler of its own; in the child the
        /// statement must end the process the way it would outside a test,
        /// and nothing may write after it.
        void RestoreDefaultEndings() {
#if defined( _WIN32 )
#    if defined( _MSC_VER )
            // abort() ends the process without a dialog or a report.
            _set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
#    endif
#else
            for ( const int signal : { SIGABRT, SIGBUS, SIGFPE, SIGILL,
                                       SIGINT, SIGSEGV, SIGTERM } ) {
                std::signal( signal, SIG_DFL );
            }
#endif
        }

        /// The child's side: run the statement and end the process.
        [[noreturn]] void RunInChild( const std::function<void()>& statement ) {
            RestoreDefaultEndings();
            std::fflush( stdout );
            WriteError( kReached );
            // Reaching the end means the statement did not end the process.
            bool threw = false;
            CATCH_TRY {
                statement();
            } CATCH_CATCH_ALL {
                threw = true;
            }
            WriteError( threw ? kThrew : kReturned );
            std::_Exit( kChildSurvived );
        }

        DeathResult::Ending EndingOf( const Runner::ExitStatus& status ) {
            switch ( status.kind ) {
            case Runner::ExitStatus::Kind::kSignaled:
                return DeathResult::Ending::kSignaled;
            case Runner::ExitStatus::Kind::kException:
                return DeathResult::Ending::kException;
            case Runner::ExitStatus::Kind::kExited:
                break;
            }
            return DeathResult::Ending::kExited;
        }

        /// What the child wrote, with every CR LF as LF: on Windows the
        /// standard error of the child is in text mode, so each LF it wrote
        /// arrives as CR LF, and neither the markers nor a matcher would
        /// find the text the statement wrote.
        std::string WithLineFeeds( const std::string& text ) {
            std::string result;
            result.reserve( text.size() );
            for ( std::size_t i = 0; i < text.size(); ++i ) {
                if ( text[i] == '\r' && i + 1 < text.size() &&
                     text[i + 1] == '\n' ) {
                    continue;
                }
                result.push_back( text[i] );
            }
            return result;
        }

        /// The parent's side: read what the child wrote and how it ended.
        DeathResult Interpret( const Runner::CapturedRun& run ) {
            DeathResult result;
            const std::string errors = WithLineFeeds( run.errors );
            const std::size_t reached = errors.find( kReached );
            if ( reached == std::string::npos ) {
                result.kind = DeathResult::Kind::kNotReached;
                result.errors = errors;
                return result;
            }
            std::string after = errors.substr( reached + kReached.size() );
            const auto endsWith = [&after]( std::string_view marker ) {
                return after.size() >= marker.size() &&
                       std::string_view( after ).substr(
                           after.size() - marker.size() ) == marker;
            };
            if ( endsWith( kReturned ) ) {
                result.kind = DeathResult::Kind::kReturned;
                after.resize( after.size() - kReturned.size() );
            } else if ( endsWith( kThrew ) ) {
                result.kind = DeathResult::Kind::kThrew;
                after.resize( after.size() - kThrew.size() );
            } else {
                result.kind = DeathResult::Kind::kEnded;
                result.ending = EndingOf( run.status );
                result.code = run.status.code;
            }
            result.errors = std::move( after );
            return result;
        }

    } // namespace

    bool DeathResult::Died() const noexcept {
        if ( kind == Kind::kSkipped ) {
            return true;
        }
        return kind == Kind::kEnded &&
               !( ending == Ending::kExited && code == 0 );
    }

    namespace {

        //! `text`, and what the child wrote to standard error after `lead`
        //! when it wrote anything.
        std::string WithErrors( std::string text,
                                const char* lead,
                                const std::string& errors ) {
            if ( !errors.empty() ) {
                text += lead;
                text += errors;
            }
            return text;
        }

        //! How the runner names the way a child ended.
        Runner::ExitStatus::Kind StatusKind( DeathResult::Ending ending ) {
            switch ( ending ) {
            case DeathResult::Ending::kSignaled:
                return Runner::ExitStatus::Kind::kSignaled;
            case DeathResult::Ending::kException:
                return Runner::ExitStatus::Kind::kException;
            case DeathResult::Ending::kExited:
                break;
            }
            return Runner::ExitStatus::Kind::kExited;
        }

    } // namespace

    std::string DeathResult::Describe() const {
        switch ( kind ) {
        case Kind::kEnded: {
            Runner::ExitStatus status;
            status.kind = StatusKind( ending );
            status.code = code;
            return WithErrors( "the statement ended the process with " +
                                   status.Describe(),
                               "; it wrote to standard error:\n",
                               errors );
        }
        case Kind::kReturned:
            return WithErrors(
                "the statement returned instead of ending the process",
                "; it wrote to standard error:\n",
                errors );
        case Kind::kThrew:
            return "the statement threw an exception instead of ending the "
                   "process";
        case Kind::kNotReached:
            return WithErrors(
                "the child process ran the test but did not reach the "
                "statement; the test must reach it the same way every time",
                "; standard error:\n",
                errors );
        case Kind::kNotStarted:
            return "the child process could not be started: " + errors;
        case Kind::kSkipped:
            return "not run in the child of another death statement";
        }
        return {};
    }

    bool ExitedWith::operator()( const DeathResult& result ) const {
        return result.kind == DeathResult::Kind::kEnded &&
               result.ending == DeathResult::Ending::kExited &&
               result.code == code;
    }

    std::string ExitedWith::Describe() const {
        return "exit code " + std::to_string( code );
    }

    bool KilledBySignal::operator()( const DeathResult& result ) const {
        return result.kind == DeathResult::Kind::kEnded &&
               result.ending == DeathResult::Ending::kSignaled &&
               result.code == signal;
    }

    std::string KilledBySignal::Describe() const {
        return "signal " + std::to_string( signal );
    }

    namespace Detail {

        DeathResult RunDeathStatement( const char* file,
                                       int line,
                                       const std::function<void()>& statement ) {
            const std::string name = NameStatement( file, line );
            if ( Runner::HasEnvironment( kStatementVariable ) ) {
                if ( Runner::ReadEnvironment( kStatementVariable ) == name ) {
                    RunInChild( statement );
                }
                DeathResult skipped;
                skipped.kind = DeathResult::Kind::kSkipped;
                return skipped;
            }

            Runner::ChildOptions child;
            child.arguments = {
                Runner::CurrentExecutable( nullptr ).string(),
                "--in-process",
                "--no-config",
                "--no-history",
                "--allow-no-tests",
                ExactSpec( ::Catch::getResultCapture().getCurrentTestName() ),
            };
            child.environment = {
                { kStatementVariable, name },
                // What Bazel hands a test is for the parent only: the child
                // writes no report, reads no filter and joins no shard.
                { "XML_OUTPUT_FILE", "" },
                { "TESTBRIDGE_TEST_ONLY", "" },
                { "TEST_SHARD_INDEX", "" },
                { "TEST_TOTAL_SHARDS", "" },
                { "TEST_SHARD_STATUS_FILE", "" },
                { "TEST_PREMATURE_EXIT_FILE", "" },
            };
            auto run = Runner::RunAndCapture( child );
            if ( !run ) {
                DeathResult failed;
                failed.kind = DeathResult::Kind::kNotStarted;
                failed.errors = run.GetError().Message();
                return failed;
            }
            return Interpret( *run );
        }

    } // namespace Detail

} // namespace Progmasoft::Catch3
