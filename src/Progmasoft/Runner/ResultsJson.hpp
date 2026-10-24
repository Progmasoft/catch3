// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include "Json.hpp"

#include <Progmasoft/Catch3/Results.hpp>

namespace Progmasoft::Catch3::Runner
{
    /// The version of the JSON shapes below. It is written into every
    /// report and protocol stream; a reader refuses a newer major version.
    inline constexpr int kResultsFormatVersion = 1;

    [[nodiscard]] JsonValue
    ToJson(const AssertionRecord &assertion);

    [[nodiscard]] AssertionRecord
    AssertionFromJson(const JsonValue &value);

    [[nodiscard]] JsonValue
    ToJson(const BenchmarkRecord &benchmark);

    [[nodiscard]] BenchmarkRecord
    BenchmarkFromJson(const JsonValue &value);

    [[nodiscard]] JsonValue
    ToJson(const SectionRecord &section);

    [[nodiscard]] SectionRecord
    SectionFromJson(const JsonValue &value);

    [[nodiscard]] JsonValue
    ToJson(const Attempt &attempt);

    [[nodiscard]] Attempt
    AttemptFromJson(const JsonValue &value);

    [[nodiscard]] JsonValue
    ToJson(const TestResult &test);

    [[nodiscard]] TestResult
    TestResultFromJson(const JsonValue &value);

    /// The whole run, with a summary: the `--json` report.
    [[nodiscard]] JsonValue
    ToJson(const RunResult &run);

    [[nodiscard]] Expected<RunResult>
    RunResultFromJson(const JsonValue &value);
} // namespace Progmasoft::Catch3::Runner
