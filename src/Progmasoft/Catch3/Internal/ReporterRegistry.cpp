
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/Interfaces/EventListenerFactory.hpp>
#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>
#include <Progmasoft/Catch3/Internal/ReporterRegistry.hpp>

namespace Catch {
    struct ReporterRegistry::ReporterRegistryImpl {
        std::vector<Detail::unique_ptr<EventListenerFactory>> listeners;
    };

    ReporterRegistry::ReporterRegistry():
        m_impl( Detail::make_unique<ReporterRegistryImpl>() ) {}

    ReporterRegistry::~ReporterRegistry() = default;

    void ReporterRegistry::registerListener(
        Detail::unique_ptr<EventListenerFactory> factory ) {
        m_impl->listeners.push_back( CATCH_MOVE( factory ) );
    }

    std::vector<Detail::unique_ptr<EventListenerFactory>> const&
    ReporterRegistry::getListeners() const {
        return m_impl->listeners;
    }
} // namespace Catch
