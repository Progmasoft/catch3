
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
// Adapted from donated nonius code.

#pragma once

#include <Progmasoft/Catch3/Benchmark/Detail/CompleteInvoke.hpp>
#include <Progmasoft/Catch3/Benchmark/Detail/Timing.hpp>
#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>

namespace Catch {
    namespace Benchmark {
        namespace Detail {
            template <typename Clock, typename Fun, typename... Args>
            TimingOf<Fun, Args...> measure(Fun&& fun, Args&&... args) {
                auto start = Clock::now();
                auto&& r = Detail::complete_invoke(CATCH_FORWARD(fun), CATCH_FORWARD(args)...);
                auto end = Clock::now();
                auto delta = end - start;
                return { delta, CATCH_FORWARD(r), 1 };
            }
        } // namespace Detail
    } // namespace Benchmark
} // namespace Catch

