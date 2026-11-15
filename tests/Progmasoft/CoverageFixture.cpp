// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// A test binary built with coverage instrumentation, for the integration
// tests of `--coverage`: each test reaches lines the other does not. The
// markers at the ends of lines let the tests find those lines.

#include <Progmasoft/Catch3/TestMacros.hpp>

namespace
{
    int
    AddOne(int value)
    {
        return value + 1; // coverage: add-one
    }

    int
    SubtractOne(int value)
    {
        return value - 1; // coverage: subtract-one
    }

    int
    NonNegative(int value)
    {
        if (value < 0) // coverage: branch
        {
            return 0;
        }
        return value;
    }
} // namespace

TEST_CASE("Adds one", "[coverage]")
{
    CHECK(AddOne(1) == 2);
    CHECK(NonNegative(AddOne(1)) == 2);
}

TEST_CASE("Subtracts one", "[coverage]")
{
    CHECK(SubtractOne(1) == 0);
}
