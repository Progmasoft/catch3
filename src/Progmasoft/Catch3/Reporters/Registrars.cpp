
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/Reporters/Registrars.hpp>

#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>
#include <Progmasoft/Catch3/Internal/CompilerCapabilities.hpp>

namespace Catch {
    namespace Detail {

        void registerListenerImpl( Detail::unique_ptr<EventListenerFactory> listenerFactory ) {
            getMutableRegistryHub().registerListener( CATCH_MOVE(listenerFactory) );
        }


    } // namespace Detail
} // namespace Catch
