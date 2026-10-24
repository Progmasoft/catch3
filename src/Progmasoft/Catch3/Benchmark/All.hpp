
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
/** \file
 * This is a convenience header for Catch2's benchmarking. It includes
 * **all** of Catch2 headers related to benchmarking.
 *
 * Generally the Catch2 users should use specific includes they need,
 * but this header can be used instead for ease-of-experimentation, or
 * just plain convenience, at the cost of (significantly) increased
 * compilation times.
 *
 * When a new header is added to either the `benchmark` folder, or to
 * the corresponding internal (detail) subfolder, it should be added here.
 */

#pragma once

#include <Progmasoft/Catch3/Benchmark/Benchmark.hpp>
#include <Progmasoft/Catch3/Benchmark/Chronometer.hpp>
#include <Progmasoft/Catch3/Benchmark/Clock.hpp>
#include <Progmasoft/Catch3/Benchmark/Constructor.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Analyse.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/BenchmarkFunction.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/BenchmarkStats.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/BenchmarkStatsFwd.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/CompleteInvoke.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/EstimateClock.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Measure.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Repeat.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/RunForAtLeast.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Stats.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Timing.hpp>
#include <Progmasoft/Catch3/Benchmark/Environment.hpp>
#include <Progmasoft/Catch3/Benchmark/Estimate.hpp>
#include <Progmasoft/Catch3/Benchmark/ExecutionPlan.hpp>
#include <Progmasoft/Catch3/Benchmark/Optimizer.hpp>
#include <Progmasoft/Catch3/Benchmark/OutlierClassification.hpp>
#include <Progmasoft/Catch3/Benchmark/SampleAnalysis.hpp>

