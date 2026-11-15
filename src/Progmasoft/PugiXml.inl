// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// pugixml, compiled header-only into XmlWriter.cpp and ResultImportXml.cpp
// and nowhere else. Its namespace is renamed, so Catch3 cannot collide with a copy of pugixml that
// the tests themselves link. It is vendored code: it is read as a system
// header, outside the warnings Catch3 builds with.
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC system_header
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#    pragma warning(push, 0)
#endif

#define pugi Progmasoft_Catch3_Pugi
#define PUGIXML_HEADER_ONLY
#define PUGIXML_NO_EXCEPTIONS
#include <pugixml.hpp>

#if defined(_MSC_VER) && !defined(__clang__)
#    pragma warning(pop)
#endif
