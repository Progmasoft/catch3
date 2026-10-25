
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/Benchmark/Detail/BenchmarkFunction.hpp>

namespace Catch {
    namespace Benchmark {
        namespace Detail {
            struct do_nothing {
                void operator()() const {}
            };

            BenchmarkFunction::callable::~callable() = default;
            BenchmarkFunction::BenchmarkFunction():
                f( new model<do_nothing>{ {} } ){}
        } // namespace Detail
    } // namespace Benchmark
} // namespace Catch
