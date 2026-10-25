
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
// Adapted from donated nonius code.

#pragma once

#include <chrono>

namespace Catch {
    namespace Benchmark {
        using IDuration = std::chrono::nanoseconds;
        using FDuration = std::chrono::duration<double, std::nano>;

        template <typename Clock>
        using TimePoint = typename Clock::time_point;

        using default_clock = std::chrono::steady_clock;
    } // namespace Benchmark
} // namespace Catch

