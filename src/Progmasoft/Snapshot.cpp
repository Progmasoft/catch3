// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Snapshot.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <system_error>

#if defined( _WIN32 )
#    if !defined( NOMINMAX )
#        define NOMINMAX
#    endif
#    include <Windows.h>
#endif
#include <utility>

namespace Progmasoft::Catch3 {

    namespace {

        bool IsSafeSnapshotName( std::string_view name ) noexcept {
            if ( name.empty() || name == "." || name == ".." ) { return false; }
            for ( const unsigned char character : name ) {
                const bool isLetter =
                    ( character >= 'a' && character <= 'z' ) ||
                    ( character >= 'A' && character <= 'Z' );
                const bool isDigit = character >= '0' && character <= '9';
                if ( !isLetter && !isDigit && character != '_' &&
                     character != '-' && character != '.' ) {
                    return false;
                }
            }
            return true;
        }

        std::string NormalizeSnapshotText( std::string_view text,
                                           bool normalizeLineEndings ) {
            if ( !normalizeLineEndings ) { return std::string( text ); }

            std::string normalized;
            normalized.reserve( text.size() );
            for ( std::size_t index = 0; index < text.size(); ++index ) {
                if ( text[index] == '\r' ) {
                    if ( index + 1 < text.size() && text[index + 1] == '\n' ) {
                        ++index;
                    }
                    normalized.push_back( '\n' );
                } else {
                    normalized.push_back( text[index] );
                }
            }
            return normalized;
        }

        void LocateDifference( SnapshotResult& result ) noexcept {
            const std::size_t commonLength =
                result.Expected.size() < result.Actual.size()
                    ? result.Expected.size()
                    : result.Actual.size();
            std::size_t index = 0;
            while ( index < commonLength &&
                    result.Expected[index] == result.Actual[index] ) {
                ++index;
            }

            result.DifferenceLine = 1;
            result.DifferenceColumn = 1;
            for ( std::size_t offset = 0; offset < index; ++offset ) {
                if ( result.Actual[offset] == '\n' ) {
                    ++result.DifferenceLine;
                    result.DifferenceColumn = 1;
                } else {
                    ++result.DifferenceColumn;
                }
            }
        }

        bool WriteAtomically( const std::filesystem::path& destination,
                              std::string_view contents,
                              bool replaceExisting,
                              std::error_code& error ) {
            // Write beside the destination so replacement stays on one volume
            // and readers never observe a partially written snapshot.
            static std::atomic<std::uint64_t> nextTemporaryFile{ 0 };
            const auto clockValue =
                std::chrono::steady_clock::now().time_since_epoch().count();
            const auto sequence =
                nextTemporaryFile.fetch_add( 1, std::memory_order_relaxed );
            std::filesystem::path temporary = destination;
            temporary += ".tmp." + std::to_string( clockValue ) + "." +
                         std::to_string( sequence );

            {
                std::ofstream output( temporary,
                                      std::ios::binary | std::ios::trunc );
                if ( !output ) {
                    error = std::make_error_code( std::errc::io_error );
                    return false;
                }
                output.write( contents.data(),
                              static_cast<std::streamsize>( contents.size() ) );
                output.flush();
                if ( !output ) {
                    error = std::make_error_code( std::errc::io_error );
                    output.close();
                    std::error_code ignored;
                    std::filesystem::remove( temporary, ignored );
                    return false;
                }
            }

            if ( replaceExisting ) {
#if defined( _WIN32 )
                if ( MoveFileExW( temporary.c_str(),
                                  destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING |
                                      MOVEFILE_WRITE_THROUGH ) == 0 ) {
                    error = std::error_code( static_cast<int>( GetLastError() ),
                                             std::system_category() );
                }
#else
                std::filesystem::rename( temporary, destination, error );
#endif
            } else {
                // A hard-link create is atomic and refuses a concurrent file.
                std::filesystem::create_hard_link(
                    temporary, destination, error );
                std::error_code ignored;
                std::filesystem::remove( temporary, ignored );
            }
            if ( error ) {
                std::error_code ignored;
                std::filesystem::remove( temporary, ignored );
                return false;
            }
            return true;
        }

    } // namespace

    bool SnapshotResult::Succeeded() const noexcept {
        return Status == SnapshotStatus::kMatched ||
               Status == SnapshotStatus::kCreated ||
               Status == SnapshotStatus::kUpdated;
    }

    SnapshotResult CompareSnapshot( std::string_view name,
                                    std::string_view actual,
                                    const SnapshotOptions& options ) {
        SnapshotResult result;
        if ( !IsSafeSnapshotName( name ) ) {
            result.Status = SnapshotStatus::kInvalidName;
            result.Message =
                "snapshot name must be a non-empty ASCII filename component";
            return result;
        }
        if ( options.Directory.empty() ||
             ( options.UpdateMode != SnapshotUpdateMode::kNever &&
               options.UpdateMode != SnapshotUpdateMode::kCreateMissing &&
               options.UpdateMode != SnapshotUpdateMode::kAlways ) ) {
            result.Status = SnapshotStatus::kInvalidOptions;
            result.Message = "snapshot directory or update mode is invalid";
            return result;
        }

        result.File = options.Directory / ( std::string( name ) + ".snap" );
        result.Actual =
            NormalizeSnapshotText( actual, options.NormalizeLineEndings );

        std::error_code error;
        const std::filesystem::file_status fileStatus =
            std::filesystem::symlink_status( result.File, error );
        if ( error && error != std::errc::no_such_file_or_directory ) {
            result.Status = SnapshotStatus::kIoError;
            result.Message =
                "could not inspect snapshot file: " + error.message();
            return result;
        }

        const bool exists = !error && fileStatus.type() !=
                                          std::filesystem::file_type::not_found;
        if ( exists && std::filesystem::is_symlink( fileStatus ) ) {
            result.Status = SnapshotStatus::kIoError;
            result.Message = "snapshot files must not be symbolic links";
            return result;
        }
        if ( exists && !std::filesystem::is_regular_file( fileStatus ) ) {
            result.Status = SnapshotStatus::kIoError;
            result.Message = "snapshot path is not a regular file";
            return result;
        }

        if ( !exists ) {
            if ( options.UpdateMode == SnapshotUpdateMode::kNever ) {
                result.Status = SnapshotStatus::kMismatched;
                result.Message = "snapshot file does not exist";
                LocateDifference( result );
                return result;
            }

            std::filesystem::create_directories( options.Directory, error );
            if ( error || !WriteAtomically(
                              result.File, result.Actual, false, error ) ) {
                result.Status = SnapshotStatus::kIoError;
                result.Message =
                    "could not create snapshot file: " + error.message();
                return result;
            }
            result.Status = SnapshotStatus::kCreated;
            result.Message = "snapshot created";
            return result;
        }

        std::ifstream input( result.File, std::ios::binary );
        if ( !input ) {
            result.Status = SnapshotStatus::kIoError;
            result.Message = "could not open snapshot file for reading";
            return result;
        }
        const std::string fileContents{ std::istreambuf_iterator<char>( input ),
                                        std::istreambuf_iterator<char>() };
        if ( input.bad() ) {
            result.Status = SnapshotStatus::kIoError;
            result.Message = "could not read snapshot file";
            return result;
        }
        input.close();
        if ( input.fail() ) {
            result.Status = SnapshotStatus::kIoError;
            result.Message = "could not close snapshot file after reading";
            return result;
        }
        result.Expected =
            NormalizeSnapshotText( fileContents, options.NormalizeLineEndings );

        if ( result.Expected == result.Actual ) {
            result.Status = SnapshotStatus::kMatched;
            result.Message = "snapshot matched";
            return result;
        }

        LocateDifference( result );
        if ( options.UpdateMode == SnapshotUpdateMode::kAlways ) {
            if ( !WriteAtomically( result.File, result.Actual, true, error ) ) {
                result.Status = SnapshotStatus::kIoError;
                result.Message =
                    "could not update snapshot file: " + error.message();
                return result;
            }
            result.Status = SnapshotStatus::kUpdated;
            result.Message = "snapshot updated";
            return result;
        }

        result.Status = SnapshotStatus::kMismatched;
        result.Message = "snapshot differs at line " +
                         std::to_string( result.DifferenceLine ) + ", column " +
                         std::to_string( result.DifferenceColumn );
        return result;
    }

} // namespace Progmasoft::Catch3
