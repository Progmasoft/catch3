
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/UserConfig.hpp>

#if defined( CATCH_CONFIG_THREAD_SAFE_ASSERTIONS )
#define CATCH_INTERNAL_THREAD_LOCAL thread_local
#else
#define CATCH_INTERNAL_THREAD_LOCAL
#endif

