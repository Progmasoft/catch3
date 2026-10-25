
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
/** \file
 * This is a convenience header for Catch2's event listener support. It
 * includes the headers a listener is written with.
 *
 * The reporters of Catch2 were replaced by the reports of the Catch3
 * runner; listeners remain, and run in every worker process.
 */

#pragma once

#include <Progmasoft/Catch3/Reporters/EventListener.hpp>
#include <Progmasoft/Catch3/Reporters/Multi.hpp>
#include <Progmasoft/Catch3/Reporters/Registrars.hpp>

