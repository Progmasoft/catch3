
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Internal/StringRef.hpp>

namespace Catch {
    namespace Detail {
        //! Provides case-insensitive `op<` semantics when called
        struct CaseInsensitiveLess {
            bool operator()( StringRef lhs,
                             StringRef rhs ) const;
        };

        //! Provides case-insensitive `op==` semantics when called
        struct CaseInsensitiveEqualTo {
            bool operator()( StringRef lhs,
                             StringRef rhs ) const;
        };

    } // namespace Detail
} // namespace Catch

