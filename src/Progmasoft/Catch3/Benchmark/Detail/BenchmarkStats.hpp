
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Benchmark/Clock.hpp>
#include <Progmasoft/Catch3/Benchmark/Estimate.hpp>
#include <Progmasoft/Catch3/Benchmark/OutlierClassification.hpp>
// The fwd decl & default specialization needs to be seen by VS2017 before
// BenchmarkStats itself, or VS2017 will report compilation error.
#include <Progmasoft/Catch3/Benchmark/Detail/BenchmarkStatsFwd.hpp>

#include <string>
#include <vector>

namespace Catch {

    struct BenchmarkInfo {
        std::string name;
        double estimatedDuration;
        int iterations;
        unsigned int samples;
        unsigned int resamples;
        double clockResolution;
        double clockCost;
    };

    // We need to keep template parameter for backwards compatibility,
    // but we also do not want to use the template paraneter.
    template <class Dummy>
    struct BenchmarkStats {
        BenchmarkInfo info;

        std::vector<Benchmark::FDuration> samples;
        Benchmark::Estimate<Benchmark::FDuration> mean;
        Benchmark::Estimate<Benchmark::FDuration> standardDeviation;
        Benchmark::OutlierClassification outliers;
        double outlierVariance;
    };


} // end namespace Catch

