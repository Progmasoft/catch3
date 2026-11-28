// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

/// @file
/// Assertions that compare two values: `CHECK_EQ( a, b )` and `REQUIRE_EQ( a, b
/// )` with `NE`, `LT`, `LE`, `GT` and `GE` beside `EQ`. Each one is `CHECK( a
/// == b )` or `REQUIRE( a == b )` with the operator named by the macro: both
/// values are shown when it fails, and the assertion is reported as written,
/// `REQUIRE_EQ( a, b )`. An argument that holds a comma outside parentheses,
/// such as `std::pair<int, int>{ 1, 2 }`, goes in parentheses.
///
/// They follow the configuration of `CHECK` and `REQUIRE`: with
/// `CATCH_CONFIG_PREFIX_ALL` they are `CATCH_CHECK_EQ` and the like, and with
/// `CATCH_CONFIG_DISABLE` they do nothing.

#include <Progmasoft/Catch3/Internal/TestMacroImpl.hpp>

/// The assertion `macroName( lhs, rhs )`: decomposes `(lhs) op (rhs)` as
/// `REQUIRE` decomposes its expression, so that both values are captured, and
/// reports the text the user wrote.
#define CATCH3_INTERNAL_COMPARE(macroName, resultDisposition, op, lhs, rhs)    \
    do                                                                         \
    { /* NOLINT(bugprone-infinite-loop) */                                     \
        CATCH_INTERNAL_IGNORE_BUT_WARN((lhs)op(rhs));                          \
        Catch::AssertionHandler catchAssertionHandler(                         \
            macroName##_catch_sr,                                              \
            CATCH_INTERNAL_LINEINFO,                                           \
            CATCH_INTERNAL_STRINGIFY(lhs) ", " CATCH_INTERNAL_STRINGIFY(rhs),  \
            resultDisposition);                                                \
        INTERNAL_CATCH_TRY                                                     \
        {                                                                      \
            CATCH_INTERNAL_START_WARNINGS_SUPPRESSION                          \
            CATCH_INTERNAL_SUPPRESS_PARENTHESES_WARNINGS                       \
            catchAssertionHandler.handleExpr(                                  \
                Catch::Decomposer()                                            \
                <= (lhs)op(rhs)); /* NOLINT(bugprone-chained-comparison) */    \
            CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION                           \
        }                                                                      \
        INTERNAL_CATCH_CATCH(catchAssertionHandler)                            \
        catchAssertionHandler.complete();                                      \
    } while (INTERNAL_CATCH_TEST_LOOP_CONDITION((lhs)op(rhs)))

#if defined(CATCH_CONFIG_PREFIX_ALL) && !defined(CATCH_CONFIG_DISABLE)

#    define CATCH_CHECK_EQ(lhs, rhs)                                           \
        CATCH3_INTERNAL_COMPARE("CATCH_CHECK_EQ",                              \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                ==,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_CHECK_NE(lhs, rhs)                                           \
        CATCH3_INTERNAL_COMPARE("CATCH_CHECK_NE",                              \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                !=,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_CHECK_LT(lhs, rhs)                                           \
        CATCH3_INTERNAL_COMPARE("CATCH_CHECK_LT",                              \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                <,                                             \
                                lhs,                                           \
                                rhs)
#    define CATCH_CHECK_LE(lhs, rhs)                                           \
        CATCH3_INTERNAL_COMPARE("CATCH_CHECK_LE",                              \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                <=,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_CHECK_GT(lhs, rhs)                                           \
        CATCH3_INTERNAL_COMPARE("CATCH_CHECK_GT",                              \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                >,                                             \
                                lhs,                                           \
                                rhs)
#    define CATCH_CHECK_GE(lhs, rhs)                                           \
        CATCH3_INTERNAL_COMPARE("CATCH_CHECK_GE",                              \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                >=,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_REQUIRE_EQ(lhs, rhs)                                         \
        CATCH3_INTERNAL_COMPARE("CATCH_REQUIRE_EQ",                            \
                                Catch::ResultDisposition::Normal,              \
                                ==,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_REQUIRE_NE(lhs, rhs)                                         \
        CATCH3_INTERNAL_COMPARE("CATCH_REQUIRE_NE",                            \
                                Catch::ResultDisposition::Normal,              \
                                !=,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_REQUIRE_LT(lhs, rhs)                                         \
        CATCH3_INTERNAL_COMPARE("CATCH_REQUIRE_LT",                            \
                                Catch::ResultDisposition::Normal,              \
                                <,                                             \
                                lhs,                                           \
                                rhs)
#    define CATCH_REQUIRE_LE(lhs, rhs)                                         \
        CATCH3_INTERNAL_COMPARE("CATCH_REQUIRE_LE",                            \
                                Catch::ResultDisposition::Normal,              \
                                <=,                                            \
                                lhs,                                           \
                                rhs)
#    define CATCH_REQUIRE_GT(lhs, rhs)                                         \
        CATCH3_INTERNAL_COMPARE("CATCH_REQUIRE_GT",                            \
                                Catch::ResultDisposition::Normal,              \
                                >,                                             \
                                lhs,                                           \
                                rhs)
#    define CATCH_REQUIRE_GE(lhs, rhs)                                         \
        CATCH3_INTERNAL_COMPARE("CATCH_REQUIRE_GE",                            \
                                Catch::ResultDisposition::Normal,              \
                                >=,                                            \
                                lhs,                                           \
                                rhs)

#elif defined(CATCH_CONFIG_PREFIX_ALL) && defined(CATCH_CONFIG_DISABLE)

#    define CATCH_CHECK_EQ(lhs, rhs) (void)(0)
#    define CATCH_CHECK_NE(lhs, rhs) (void)(0)
#    define CATCH_CHECK_LT(lhs, rhs) (void)(0)
#    define CATCH_CHECK_LE(lhs, rhs) (void)(0)
#    define CATCH_CHECK_GT(lhs, rhs) (void)(0)
#    define CATCH_CHECK_GE(lhs, rhs) (void)(0)
#    define CATCH_REQUIRE_EQ(lhs, rhs) (void)(0)
#    define CATCH_REQUIRE_NE(lhs, rhs) (void)(0)
#    define CATCH_REQUIRE_LT(lhs, rhs) (void)(0)
#    define CATCH_REQUIRE_LE(lhs, rhs) (void)(0)
#    define CATCH_REQUIRE_GT(lhs, rhs) (void)(0)
#    define CATCH_REQUIRE_GE(lhs, rhs) (void)(0)

#elif !defined(CATCH_CONFIG_PREFIX_ALL) && !defined(CATCH_CONFIG_DISABLE)

#    define CHECK_EQ(lhs, rhs)                                                 \
        CATCH3_INTERNAL_COMPARE("CHECK_EQ",                                    \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                ==,                                            \
                                lhs,                                           \
                                rhs)
#    define CHECK_NE(lhs, rhs)                                                 \
        CATCH3_INTERNAL_COMPARE("CHECK_NE",                                    \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                !=,                                            \
                                lhs,                                           \
                                rhs)
#    define CHECK_LT(lhs, rhs)                                                 \
        CATCH3_INTERNAL_COMPARE("CHECK_LT",                                    \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                <,                                             \
                                lhs,                                           \
                                rhs)
#    define CHECK_LE(lhs, rhs)                                                 \
        CATCH3_INTERNAL_COMPARE("CHECK_LE",                                    \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                <=,                                            \
                                lhs,                                           \
                                rhs)
#    define CHECK_GT(lhs, rhs)                                                 \
        CATCH3_INTERNAL_COMPARE("CHECK_GT",                                    \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                >,                                             \
                                lhs,                                           \
                                rhs)
#    define CHECK_GE(lhs, rhs)                                                 \
        CATCH3_INTERNAL_COMPARE("CHECK_GE",                                    \
                                Catch::ResultDisposition::ContinueOnFailure,   \
                                >=,                                            \
                                lhs,                                           \
                                rhs)
#    define REQUIRE_EQ(lhs, rhs)                                               \
        CATCH3_INTERNAL_COMPARE("REQUIRE_EQ",                                  \
                                Catch::ResultDisposition::Normal,              \
                                ==,                                            \
                                lhs,                                           \
                                rhs)
#    define REQUIRE_NE(lhs, rhs)                                               \
        CATCH3_INTERNAL_COMPARE("REQUIRE_NE",                                  \
                                Catch::ResultDisposition::Normal,              \
                                !=,                                            \
                                lhs,                                           \
                                rhs)
#    define REQUIRE_LT(lhs, rhs)                                               \
        CATCH3_INTERNAL_COMPARE("REQUIRE_LT",                                  \
                                Catch::ResultDisposition::Normal,              \
                                <,                                             \
                                lhs,                                           \
                                rhs)
#    define REQUIRE_LE(lhs, rhs)                                               \
        CATCH3_INTERNAL_COMPARE("REQUIRE_LE",                                  \
                                Catch::ResultDisposition::Normal,              \
                                <=,                                            \
                                lhs,                                           \
                                rhs)
#    define REQUIRE_GT(lhs, rhs)                                               \
        CATCH3_INTERNAL_COMPARE("REQUIRE_GT",                                  \
                                Catch::ResultDisposition::Normal,              \
                                >,                                             \
                                lhs,                                           \
                                rhs)
#    define REQUIRE_GE(lhs, rhs)                                               \
        CATCH3_INTERNAL_COMPARE("REQUIRE_GE",                                  \
                                Catch::ResultDisposition::Normal,              \
                                >=,                                            \
                                lhs,                                           \
                                rhs)

#elif !defined(CATCH_CONFIG_PREFIX_ALL) && defined(CATCH_CONFIG_DISABLE)

#    define CHECK_EQ(lhs, rhs) (void)(0)
#    define CHECK_NE(lhs, rhs) (void)(0)
#    define CHECK_LT(lhs, rhs) (void)(0)
#    define CHECK_LE(lhs, rhs) (void)(0)
#    define CHECK_GT(lhs, rhs) (void)(0)
#    define CHECK_GE(lhs, rhs) (void)(0)
#    define REQUIRE_EQ(lhs, rhs) (void)(0)
#    define REQUIRE_NE(lhs, rhs) (void)(0)
#    define REQUIRE_LT(lhs, rhs) (void)(0)
#    define REQUIRE_LE(lhs, rhs) (void)(0)
#    define REQUIRE_GT(lhs, rhs) (void)(0)
#    define REQUIRE_GE(lhs, rhs) (void)(0)

#endif
