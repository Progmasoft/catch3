
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
// Adapted from donated nonius code.

#pragma once

#include <Progmasoft/Catch3/Benchmark/Clock.hpp>
#include <Progmasoft/Catch3/Benchmark/Chronometer.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Measure.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/CompleteInvoke.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Timing.hpp>
#include <Progmasoft/Catch3/Internal/Meta.hpp>
#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>

#include <type_traits>

namespace Catch {
    namespace Benchmark {
        namespace Detail {
            template <typename Clock, typename Fun>
            TimingOf<Fun, int> measure_one(Fun&& fun, int iters, std::false_type) {
                return Detail::measure<Clock>(fun, iters);
            }
            template <typename Clock, typename Fun>
            TimingOf<Fun, Chronometer> measure_one(Fun&& fun, int iters, std::true_type) {
                Detail::ChronometerModel<Clock> meter;
                auto&& result = Detail::complete_invoke(fun, Chronometer(meter, iters));

                return { meter.elapsed(), CATCH_MOVE(result), iters };
            }

            template <typename Clock, typename Fun>
            using run_for_at_least_argument_t = std::conditional_t<is_callable<Fun(Chronometer)>::value, Chronometer, int>;


            [[noreturn]]
            void throw_optimized_away_error();

            template <typename Clock, typename Fun>
            TimingOf<Fun, run_for_at_least_argument_t<Clock, Fun>>
                run_for_at_least(IDuration how_long,
                                 const int initial_iterations,
                                 Fun&& fun) {
                auto iters = initial_iterations;
                while (iters < (1 << 30)) {
                    auto&& Timing = measure_one<Clock>(fun, iters, is_callable<Fun(Chronometer)>());

                    if (Timing.elapsed >= how_long) {
                        return { Timing.elapsed, CATCH_MOVE(Timing.result), iters };
                    }
                    iters *= 2;
                }
                throw_optimized_away_error();
            }
        } // namespace Detail
    } // namespace Benchmark
} // namespace Catch

