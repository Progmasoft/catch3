
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Internal/UniquePtr.hpp>
#include <Progmasoft/Catch3/Internal/StringRef.hpp>

#include <string>

namespace Catch {

    class IConfig;
    class IEventListener;
    using IEventListenerPtr = Detail::unique_ptr<IEventListener>;

    class EventListenerFactory {
    public:
        virtual ~EventListenerFactory(); // = default
        virtual IEventListenerPtr create( IConfig const* config ) const = 0;
        //! Return a meaningful name for the listener, e.g. its type name
        virtual StringRef getName() const = 0;
        //! Return listener's description if available
        virtual std::string getDescription() const = 0;
    };
} // namespace Catch

