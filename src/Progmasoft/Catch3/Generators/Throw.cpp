
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/Generators/GeneratorException.hpp>
#include <Progmasoft/Catch3/Generators/Throw.hpp>
#include <Progmasoft/Catch3/Internal/Enforce.hpp>

namespace Catch {
    namespace Generators {
        namespace Detail {

            [[noreturn]]
            void throw_generator_exception( char const* msg ) {
                Catch::throw_exception( GeneratorException{ msg } );
            }

        } // namespace Detail
    } // namespace Generators
} // namespace Catch
