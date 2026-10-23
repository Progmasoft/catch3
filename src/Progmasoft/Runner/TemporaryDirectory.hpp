// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <array>
#include <optional>
#include <string>

namespace Progmasoft::Catch3::Runner
{
    /// Gives the test that runs while it lives a temporary directory of its
    /// own: it creates the directory, points TMPDIR, TMP and TEMP at it and
    /// makes `TestTemporaryDirectory` return it. The variables get their
    /// previous values back afterwards; the directory stays, because the
    /// runner decides whether to keep it.
    class TemporaryDirectoryScope final
    {
    public:
        explicit TemporaryDirectoryScope(const std::string &directory);
        TemporaryDirectoryScope(const TemporaryDirectoryScope &) = delete;
        TemporaryDirectoryScope &
        operator=(const TemporaryDirectoryScope &) = delete;
        ~TemporaryDirectoryScope();

    private:
        bool active_ = false;
        std::array<std::optional<std::string>, 3> previous_;
    };
} // namespace Progmasoft::Catch3::Runner
