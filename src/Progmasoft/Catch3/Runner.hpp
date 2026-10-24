// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Progmasoft::Catch3
{
    /// Runs the tests of this binary as `arguments` say and returns the exit
    /// code. `arguments` starts with the program, as `argv` does.
    ///
    /// `Catch::Session::run` and the `main` of `Catch2::Catch2WithMain` call
    /// this; a test binary with its own `main` may call it directly.
    [[nodiscard]] int
    RunTests(const std::vector<std::string> &arguments);

    /// Prints the options of the runner to standard output.
    void
    PrintHelp(const std::string &programName);

    /// The directory the running test may keep temporary files in.
    ///
    /// Every attempt of every test gets its own, so tests that run at the
    /// same time in different workers never share a file by accident. The
    /// runner also points TMPDIR, TMP and TEMP at it while the test runs, so
    /// `std::filesystem::temp_directory_path()` and `tmpfile` use it too.
    /// It is removed after a passing attempt and kept after a failing one,
    /// for inspection. Outside a test it is the system temporary directory.
    [[nodiscard]] std::filesystem::path
    TestTemporaryDirectory();
} // namespace Progmasoft::Catch3
