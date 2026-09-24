// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <iosfwd>
#include <string>

namespace Progmasoft::Catch3 {

    struct TestRunResult;

    /// Emits deterministic JUnit-style XML without timestamps or global state.
    class JUnitXmlWriter final {
    public:
        static void Write( const TestRunResult& result, std::ostream& output );
        [[nodiscard]] static std::string
        ToString( const TestRunResult& result );
    };

} // namespace Progmasoft::Catch3
