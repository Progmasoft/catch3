// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// Tests linked with @progmasoft_catch3//:catch3_main, which brings main.
#include <Progmasoft/Catch3/Generators/Generators.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <string>

namespace
{
    std::string
    Repeat(const std::string &text, int count)
    {
        std::string result;
        for (int i = 0; i < count; ++i)
        {
            result += text;
        }
        return result;
    }
} // namespace

TEST_CASE("Generated values reach the test", "[module]")
{
    const int count = GENERATE(0, 1, 3);
    const std::string repeated = Repeat("ab", count);
    REQUIRE(repeated.size() == static_cast<std::size_t>(2 * count));
    CHECK(repeated.find("ba") == (count > 1 ? 1U : std::string::npos));
}
