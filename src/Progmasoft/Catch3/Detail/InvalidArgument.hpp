// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace Progmasoft::Catch3::Detail
{

    // An invalid generator contract cannot be represented by a value-returning
    // factory. Preserve the exception API for normal clients and fail loudly
    // when a caller deliberately compiles without language exceptions.
    [[noreturn]] static inline void
    RejectInvalidArgument(const char *message)
    {
#if defined(__cpp_exceptions)
        throw std::invalid_argument(message);
#else
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        std::abort();
#endif
    }

} // namespace Progmasoft::Catch3::Detail
