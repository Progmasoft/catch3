// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Message.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>

#include <Progmasoft/Catch3/Interfaces/IResultCapture.hpp>

#include <Progmasoft/Catch3/ForAll.hpp>
#include <Progmasoft/Catch3/Property.hpp>
#include <Progmasoft/Catch3/Snapshot.hpp>
#include <Progmasoft/Catch3/Strategy/Context.hpp>

#include <string>

/// Runs a property and reports its replay seed and minimized counterexample.
#define CATCH3_CHECK_PROPERTY( ... )                            \
    do {                                                        \
        const auto catch3PropertyResult =                       \
            ::Progmasoft::Catch3::CheckProperty( __VA_ARGS__ ); \
        INFO( catch3PropertyResult.Describe() );                \
        CHECK( catch3PropertyResult.Succeeded() );              \
    } while ( false )

/// Compares a value with a named snapshot and reports the first difference.
#define CATCH3_CHECK_SNAPSHOT( name, actual, options )                      \
    do {                                                                    \
        const auto catch3SnapshotResult =                                   \
            ::Progmasoft::Catch3::CompareSnapshot( name, actual, options ); \
        INFO( catch3SnapshotResult.Message );                               \
        CHECK( catch3SnapshotResult.Succeeded() );                          \
    } while ( false )

/// Runs ForAll( [options,] strategy..., body ) under the name of the
/// current test case, which keys the example database, and reports the
/// result through Catch2: the minimized counterexample, its notes and its
/// replay token on failure.
#define CATCH3_CHECK_FOR_ALL( ... )                                          \
    do {                                                                     \
        const ::Progmasoft::Catch3::Detail::PropertyNameScope catch3Name(    \
            ::Catch::getResultCapture().getCurrentTestName() + ":" +         \
            std::to_string( __LINE__ ) );                                    \
        const auto catch3ForAllResult =                                      \
            ::Progmasoft::Catch3::ForAll( __VA_ARGS__ );                     \
        INFO( catch3ForAllResult.Describe() );                               \
        CHECK( catch3ForAllResult.Succeeded() );                             \
    } while ( false )

/// As CATCH3_CHECK_FOR_ALL, but ends the test case on failure.
#define CATCH3_REQUIRE_FOR_ALL( ... )                                        \
    do {                                                                     \
        const ::Progmasoft::Catch3::Detail::PropertyNameScope catch3Name(    \
            ::Catch::getResultCapture().getCurrentTestName() + ":" +         \
            std::to_string( __LINE__ ) );                                    \
        const auto catch3ForAllResult =                                      \
            ::Progmasoft::Catch3::ForAll( __VA_ARGS__ );                     \
        INFO( catch3ForAllResult.Describe() );                               \
        REQUIRE( catch3ForAllResult.Succeeded() );                           \
    } while ( false )
