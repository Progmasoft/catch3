// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// yyjson, compiled into Json.cpp and nowhere else. Every function of its API
// is static here, so the library adds no symbol to Catch3 and cannot collide
// with a copy of yyjson that the tests themselves link. It is vendored code:
// it is read as a system header, outside the warnings Catch3 builds with.
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC system_header
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#    pragma warning(push, 0)
#endif

#define yyjson_api static
#include <yyjson.c>

#if defined(_MSC_VER) && !defined(__clang__)
#    pragma warning(pop)
#endif
