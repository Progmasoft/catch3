
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
/** \file
 * This is a convenience header for Catch2's interfaces. It includes
 * **all** of Catch2 headers related to interfaces.
 *
 * Generally the Catch2 users should use specific includes they need,
 * but this header can be used instead for ease-of-experimentation, or
 * just plain convenience, at the cost of somewhat increased compilation
 * times.
 *
 * When a new header is added to either the `interfaces` folder, or to
 * the corresponding internal subfolder, it should be added here.
 */


#pragma once

#include <Progmasoft/Catch3/Interfaces/EventListenerFactory.hpp>
#include <Progmasoft/Catch3/Interfaces/IConfig.hpp>
#include <Progmasoft/Catch3/Interfaces/IEventListener.hpp>
#include <Progmasoft/Catch3/Interfaces/IExceptionTranslator.hpp>
#include <Progmasoft/Catch3/Interfaces/IGeneratorTracker.hpp>
#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>
#include <Progmasoft/Catch3/Interfaces/IResultCapture.hpp>
#include <Progmasoft/Catch3/Interfaces/ITagAliasRegistry.hpp>
#include <Progmasoft/Catch3/Interfaces/ITestCaseRegistry.hpp>
#include <Progmasoft/Catch3/Interfaces/ITestInvoker.hpp>

