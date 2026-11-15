// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// Tests linked with @progmasoft_catch3//:catch3_main, which brings main.
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <string>

TEST_CASE("A test runs from the published module", "[module]")
{
    const std::string name = "progmasoft_catch3";
    REQUIRE(name.size() == 17);
    CHECK(name.rfind("progmasoft_", 0) == 0);
}
