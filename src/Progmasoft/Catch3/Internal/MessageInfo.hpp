
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Internal/DeprecationMacro.hpp>
#include <Progmasoft/Catch3/Internal/ResultType.hpp>
#include <Progmasoft/Catch3/Internal/SourceLineInfo.hpp>
#include <Progmasoft/Catch3/Internal/StringRef.hpp>

#include <string>

namespace Catch {

    struct MessageInfo {
        MessageInfo(    StringRef _macroName,
                        SourceLineInfo const& _lineInfo,
                        ResultWas::OfType _type );

        StringRef macroName;
        std::string message;
        SourceLineInfo lineInfo;
        ResultWas::OfType type;
        // The "ID" of the message, used to know when to remove it from reporter context.
        unsigned int sequence;

        CATCH_DEPRECATED( "Explicitly use the 'sequence' member instead" )
        bool operator == (MessageInfo const& other) const {
            return sequence == other.sequence;
        }
        CATCH_DEPRECATED( "Explicitly use the 'sequence' member instead" )
        bool operator < (MessageInfo const& other) const {
            return sequence < other.sequence;
        }
    };

} // end namespace Catch

