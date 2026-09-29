// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
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
