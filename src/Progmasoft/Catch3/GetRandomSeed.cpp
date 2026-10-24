
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/GetRandomSeed.hpp>

#include <Progmasoft/Catch3/Internal/Context.hpp>
#include <Progmasoft/Catch3/Config.hpp>

namespace Catch {
    std::uint32_t getSeed() {
        return getCurrentContext().getConfig()->rngSeed();
    }
}
