
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

namespace Catch {

    bool isnan(float f);
    bool isnan(double d);

    float nextafter(float x, float y);
    double nextafter(double x, double y);

}

