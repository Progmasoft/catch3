
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/TranslateException.hpp>
#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>

namespace Catch {
    namespace Detail {
        void registerTranslatorImpl(
            Detail::unique_ptr<IExceptionTranslator>&& translator ) {
            getMutableRegistryHub().registerTranslator(
                CATCH_MOVE( translator ) );
        }
    } // namespace Detail
} // namespace Catch
