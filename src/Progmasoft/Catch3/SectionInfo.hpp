
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>
#include <Progmasoft/Catch3/Internal/SourceLineInfo.hpp>
#include <Progmasoft/Catch3/Internal/StringRef.hpp>
#include <Progmasoft/Catch3/Totals.hpp>

#include <string>

namespace Catch {

    struct SectionInfo {
        // The last argument is ignored, so that people can write
        // SECTION("ShortName", "Proper description that is long") and
        // still use the `-c` flag comfortably.
        SectionInfo( SourceLineInfo const& _lineInfo, std::string _name,
                    const char* const = nullptr ):
            name(CATCH_MOVE(_name)),
            lineInfo(_lineInfo)
            {}

        std::string name;
        SourceLineInfo lineInfo;
    };

    struct SectionEndInfo {
        SectionInfo sectionInfo;
        Counts prevAssertions;
        double durationInSeconds;
    };

} // end namespace Catch

