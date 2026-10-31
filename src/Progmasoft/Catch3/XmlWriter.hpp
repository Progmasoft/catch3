// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <iosfwd>
#include <string>

namespace Progmasoft::Catch3
{
    struct RunResult;

    /// Writes a run as JUnit XML without timestamps or global state, so the
    /// same run gives the same bytes.
    ///
    /// A failed test gets `<failure>`, a crash, timeout or infrastructure
    /// problem `<error>`, and earlier failed attempts `<rerunFailure>`. A
    /// test that failed and then passed is a success with `<flakyFailure>`
    /// elements, the convention of Maven Surefire that Jenkins, GitLab and
    /// most CI dashboards read. Skipped, blocked and not-run tests are
    /// `<skipped>` with the reason as the message.
    class JUnitXmlWriter final
    {
    public:
        /// Returns false when the stream failed.
        [[nodiscard]] static bool
        Write(const RunResult &run, std::ostream &output);

        [[nodiscard]] static std::string
        ToString(const RunResult &run);
    };
} // namespace Progmasoft::Catch3
