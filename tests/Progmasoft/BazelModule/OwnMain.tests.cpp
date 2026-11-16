// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// Tests linked with @progmasoft_catch3//:catch3, with a main of their own.
#include <Progmasoft/Catch3/Session.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <algorithm>
#include <vector>

TEST_CASE("Sections run against a fresh fixture", "[module]")
{
    std::vector<int> values{ 3, 1, 2 };

    SECTION("sorting orders the values")
    {
        std::sort(values.begin(), values.end());
        REQUIRE(std::is_sorted(values.begin(), values.end()));
        CHECK(values.front() < values.back());
    }
    SECTION("each section starts from the same values")
    {
        CHECK_FALSE(std::is_sorted(values.begin(), values.end()));
    }
}

int
main(int argc, char **argv)
{
    Catch::Session session;
    return session.run(argc, argv);
}
