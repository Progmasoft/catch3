
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
// Adapted from donated nonius code.

#pragma once

#include <Progmasoft/Catch3/Benchmark/Clock.hpp>
#include <Progmasoft/Catch3/Benchmark/OutlierClassification.hpp>

namespace Catch {
    namespace Benchmark {
        struct EnvironmentEstimate {
            FDuration mean;
            OutlierClassification outliers;
        };
        struct Environment {
            EnvironmentEstimate clock_resolution;
            EnvironmentEstimate clock_cost;
        };
    } // namespace Benchmark
} // namespace Catch

