// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace {

    class TemporarySnapshotDirectory final {
    public:
        TemporarySnapshotDirectory() {
            static std::atomic<unsigned long long> nextId{ 0 };
            const auto timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            Path = std::filesystem::temp_directory_path() /
                   ( "progmasoft-catch3-snapshot-" +
                     std::to_string( timestamp ) + "-" +
                     std::to_string(
                         nextId.fetch_add( 1, std::memory_order_relaxed ) ) );
            std::filesystem::create_directories( Path );
        }

        ~TemporarySnapshotDirectory() {
            std::error_code ignored;
            std::filesystem::remove_all( Path, ignored );
        }

        std::filesystem::path Path;
    };

} // namespace

TEST_CASE( "Snapshot comparison does not write during ordinary runs",
           "[progmasoft][snapshot]" ) {
    using Progmasoft::Catch3::CompareSnapshot;
    using Progmasoft::Catch3::SnapshotOptions;
    using Progmasoft::Catch3::SnapshotStatus;

    const TemporarySnapshotDirectory directory;
    const auto result =
        CompareSnapshot( "greeting",
                         "hello\nworld\n",
                         SnapshotOptions{ .Directory = directory.Path } );

    CHECK( result.Status == SnapshotStatus::kMismatched );
    CHECK_FALSE( result.Succeeded() );
    CHECK( result.DifferenceLine == 1 );
    CHECK( result.DifferenceColumn == 1 );
    CHECK_FALSE( std::filesystem::exists( result.File ) );
}

TEST_CASE( "Snapshots normalize line endings and report precise differences",
           "[progmasoft][snapshot]" ) {
    using Progmasoft::Catch3::CompareSnapshot;
    using Progmasoft::Catch3::SnapshotOptions;
    using Progmasoft::Catch3::SnapshotStatus;
    using Progmasoft::Catch3::SnapshotUpdateMode;

    const TemporarySnapshotDirectory directory;
    const SnapshotOptions createOptions{
        .Directory = directory.Path,
        .UpdateMode = SnapshotUpdateMode::kCreateMissing,
    };
    const auto created =
        CompareSnapshot( "multiline", "one\ntwo\n", createOptions );
    REQUIRE( created.Status == SnapshotStatus::kCreated );
    CHECK( created.Succeeded() );

    const auto normalized =
        CompareSnapshot( "multiline", "one\r\ntwo\r\n", createOptions );
    CHECK( normalized.Status == SnapshotStatus::kMatched );
    CHECK( normalized.Expected == "one\ntwo\n" );

    const auto mismatch =
        CompareSnapshot( "multiline",
                         "one\ntoo\n",
                         SnapshotOptions{ .Directory = directory.Path } );
    CHECK( mismatch.Status == SnapshotStatus::kMismatched );
    CHECK( mismatch.DifferenceLine == 2 );
    CHECK( mismatch.DifferenceColumn == 2 );
    CHECK( mismatch.Message.find( "line 2, column 2" ) != std::string::npos );
}

TEST_CASE( "Snapshot creation and replacement require explicit update modes",
           "[progmasoft][snapshot]" ) {
    using Progmasoft::Catch3::CompareSnapshot;
    using Progmasoft::Catch3::SnapshotOptions;
    using Progmasoft::Catch3::SnapshotStatus;
    using Progmasoft::Catch3::SnapshotUpdateMode;

    const TemporarySnapshotDirectory directory;
    const SnapshotOptions createMissing{
        .Directory = directory.Path,
        .UpdateMode = SnapshotUpdateMode::kCreateMissing,
    };
    REQUIRE( CompareSnapshot( "answer", "first", createMissing ).Succeeded() );

    const auto protectedMismatch =
        CompareSnapshot( "answer", "second", createMissing );
    CHECK( protectedMismatch.Status == SnapshotStatus::kMismatched );
    CHECK( protectedMismatch.Expected == "first" );

    const SnapshotOptions updateEverything{
        .Directory = directory.Path,
        .UpdateMode = SnapshotUpdateMode::kAlways,
    };
    const auto updated =
        CompareSnapshot( "answer", "second", updateEverything );
    REQUIRE( updated.Status == SnapshotStatus::kUpdated );
    CHECK( updated.Expected == "first" );
    CHECK( updated.Actual == "second" );
    CHECK( CompareSnapshot( "answer", "second", createMissing ).Succeeded() );
}

TEST_CASE( "Snapshot names and I/O failures are returned as structured errors",
           "[progmasoft][snapshot]" ) {
    using Progmasoft::Catch3::CompareSnapshot;
    using Progmasoft::Catch3::SnapshotOptions;
    using Progmasoft::Catch3::SnapshotStatus;
    using Progmasoft::Catch3::SnapshotUpdateMode;

    const TemporarySnapshotDirectory directory;
    const SnapshotOptions options{ .Directory = directory.Path };
    CHECK( CompareSnapshot( "../outside", "text", options ).Status ==
           SnapshotStatus::kInvalidName );
    CHECK( CompareSnapshot( "nested/name", "text", options ).Status ==
           SnapshotStatus::kInvalidName );
    CHECK( CompareSnapshot( "", "text", options ).Status ==
           SnapshotStatus::kInvalidName );
    CHECK( CompareSnapshot( "valid", "text", SnapshotOptions{} ).Status ==
           SnapshotStatus::kInvalidOptions );

    const auto blockingFile = directory.Path / "not-a-directory";
    {
        std::ofstream output( blockingFile, std::ios::binary );
        REQUIRE( output.good() );
        output << "file";
    }
    const auto ioFailure = CompareSnapshot(
        "snapshot",
        "text",
        SnapshotOptions{ .Directory = blockingFile,
                         .UpdateMode = SnapshotUpdateMode::kCreateMissing } );
    CHECK( ioFailure.Status == SnapshotStatus::kIoError );
    CHECK_FALSE( ioFailure.Message.empty() );
}

TEST_CASE( "Snapshot assertion macro reports successful explicit updates",
           "[progmasoft][snapshot]" ) {
    const TemporarySnapshotDirectory directory;
    CATCH3_CHECK_SNAPSHOT(
        "macro",
        "stable value",
        ( Progmasoft::Catch3::SnapshotOptions{
            .Directory = directory.Path,
            .UpdateMode =
                Progmasoft::Catch3::SnapshotUpdateMode::kCreateMissing,
        } ) );
}
