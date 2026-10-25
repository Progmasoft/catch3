
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
// Adapted from donated nonius code.

#pragma once

#include <Progmasoft/Catch3/Benchmark/Clock.hpp>
#include <Progmasoft/Catch3/Benchmark/SampleAnalysis.hpp>


namespace Catch {
    class IConfig;

    namespace Benchmark {
        namespace Detail {
            SampleAnalysis analyse(const IConfig &cfg, FDuration* first, FDuration* last);
        } // namespace Detail
    } // namespace Benchmark
} // namespace Catch

