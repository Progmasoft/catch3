// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <string>
#include <utility>
#include <vector>

namespace
{
    int
    Next(int &calls)
    {
        ++calls;
        return calls;
    }
} // namespace

TEST_CASE("Comparison assertions pass when the relation holds",
          "[progmasoft][comparisons]")
{
    CHECK_EQ(2 + 2, 4);
    CHECK_NE(1, 2);
    CHECK_LT(1, 2);
    CHECK_LE(2, 2);
    CHECK_GT(3, 2);
    CHECK_GE(3, 3);
    REQUIRE_EQ(std::string("catch"), "catch");
    REQUIRE_NE(std::string("catch"), "catch3");
    REQUIRE_LT(1.5, 2.0);
    REQUIRE_LE('a', 'a');
    REQUIRE_GT(10u, 9u);
    REQUIRE_GE((std::vector<int>{ 1, 2 }.size()), 2u);
}

TEST_CASE("An argument with a comma goes in parentheses",
          "[progmasoft][comparisons]")
{
    REQUIRE_EQ((std::pair<int, int>{ 1, 2 }), (std::pair<int, int>{ 1, 2 }));
}

TEST_CASE("Each argument is evaluated once", "[progmasoft][comparisons]")
{
    int calls = 0;
    CHECK_EQ(Next(calls), 1);
    REQUIRE_LT(Next(calls), 3);
    REQUIRE_EQ(calls, 2);
}

TEST_CASE("The operator binds to each whole argument",
          "[progmasoft][comparisons]")
{
    // Without parentheses around each side, 1 + 1 == 2 * 1 would still hold,
    // but a < b ? x : y would not parse as intended.
    const bool flag = true;
    REQUIRE_EQ(flag ? 1 : 2, 1);
    REQUIRE_EQ(6 & 3, 2);
}

TEST_CASE("CHECK_EQ fails when the values differ",
          "[progmasoft][comparisons][!shouldfail]")
{
    CHECK_EQ(1, 2);
}

TEST_CASE("CHECK_NE fails when the values are equal",
          "[progmasoft][comparisons][!shouldfail]")
{
    CHECK_NE(2, 2);
}

TEST_CASE("CHECK_LT and CHECK_GT fail on equal values",
          "[progmasoft][comparisons][!shouldfail]")
{
    CHECK_LT(2, 2);
    CHECK_GT(2, 2);
}

TEST_CASE("CHECK_LE and CHECK_GE fail on the wrong side",
          "[progmasoft][comparisons][!shouldfail]")
{
    CHECK_LE(3, 2);
    CHECK_GE(2, 3);
}

TEST_CASE("REQUIRE_EQ stops the test case when it fails",
          "[progmasoft][comparisons][!shouldfail]")
{
    REQUIRE_EQ(std::string("a"), "b");
    FAIL("REQUIRE_EQ did not stop the test case");
}
