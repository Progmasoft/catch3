
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/Generators/Generators.hpp>
#include <Progmasoft/Catch3/TestCaseInfo.hpp>
#include <Progmasoft/Catch3/Internal/TagAliasRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TestSpecParser.hpp>

namespace {
    static constexpr Catch::SourceLineInfo dummySourceLineInfo = CATCH_INTERNAL_LINEINFO;

    static Catch::TestSpec parseAndCreateSpec(std::string const& str) {
        Catch::TagAliasRegistry registry;
        Catch::TestSpecParser parser( registry );

        parser.parse( str );
        auto spec = parser.testSpec();
        REQUIRE( spec.hasFilters() );
        REQUIRE( spec.getInvalidSpecs().empty());

        return spec;
    }

}

TEST_CASE( "Parsing tags with non-alphabetical characters is pass-through",
           "[test-spec][test-spec-parser]" ) {
    auto const& tagString = GENERATE( as<std::string>{},
                                      "[tag with spaces]",
                                      "[I said \"good day\" sir!]" );
    CAPTURE(tagString);

    auto spec = parseAndCreateSpec( tagString );

    Catch::TestCaseInfo testCase(
        "", { "fake test name", tagString }, dummySourceLineInfo );

    REQUIRE( spec.matches( testCase ) );
}

TEST_CASE("Parsed tags are matched case insensitive",
    "[test-spec][test-spec-parser]") {
    auto spec = parseAndCreateSpec( "[CASED tag]" );

    Catch::TestCaseInfo testCase(
        "", { "fake test name", "[cased TAG]" }, dummySourceLineInfo );

    REQUIRE( spec.matches( testCase ) );
}
