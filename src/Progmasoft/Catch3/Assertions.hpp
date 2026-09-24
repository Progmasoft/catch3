// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Progmasoft/Catch3/Property.hpp>

/// Runs a property and reports its replay seed and minimized counterexample.
#define CATCH3_CHECK_PROPERTY( ... )                            \
    do {                                                        \
        const auto catch3PropertyResult =                       \
            ::Progmasoft::Catch3::CheckProperty( __VA_ARGS__ ); \
        INFO( catch3PropertyResult.Describe() );                \
        CHECK( catch3PropertyResult.Succeeded() );              \
    } while ( false )
