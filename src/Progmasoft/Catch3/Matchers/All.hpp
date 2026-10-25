
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
/** \file
 * This is a convenience header for Catch2's Matcher support. It includes
 * **all** of Catch2 headers related to matchers.
 *
 * Generally the Catch2 users should use specific includes they need,
 * but this header can be used instead for ease-of-experimentation, or
 * just plain convenience, at the cost of increased compilation times.
 *
 * When a new header is added to either the `matchers` folder, or to
 * the corresponding internal subfolder, it should be added here.
 */

#pragma once

#include <Progmasoft/Catch3/Matchers/ContainerProperties.hpp>
#include <Progmasoft/Catch3/Matchers/Contains.hpp>
#include <Progmasoft/Catch3/Matchers/Exception.hpp>
#include <Progmasoft/Catch3/Matchers/FloatingPoint.hpp>
#include <Progmasoft/Catch3/Matchers/Internal/Impl.hpp>
#include <Progmasoft/Catch3/Matchers/Matchers.hpp>
#include <Progmasoft/Catch3/Matchers/Predicate.hpp>
#include <Progmasoft/Catch3/Matchers/Quantifiers.hpp>
#include <Progmasoft/Catch3/Matchers/RangeEquals.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Catch3/Matchers/Templated.hpp>
#include <Progmasoft/Catch3/Matchers/Vector.hpp>

