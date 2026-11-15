// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// Tests linked with @progmasoft_catch3//:catch3, with a main of their own.
#include <Progmasoft/Catch3/Session.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>

TEST_CASE("A test runs under its own main", "[module]")
{
    REQUIRE(1 + 1 == 2);
}

int
main(int argc, char **argv)
{
    Catch::Session session;
    return session.run(argc, argv);
}
