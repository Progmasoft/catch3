// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Results.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Expected.hpp"
#include "Json.hpp"
#include "Options.hpp"

namespace Progmasoft::Catch3::Runner
{
    /// The format of a report, from its contents; nothing when none fits.
    [[nodiscard]] std::optional<ImportFormat>
    DetectImportFormat(std::string_view text);

    /// The results a report holds, in its order. A test the report names
    /// twice comes back twice; `MergeImported` makes one test of them.
    /// `source` names the report, for a format whose tests have no identity
    /// of their own.
    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseImport(std::string_view text,
                ImportFormat format,
                const std::string &source = {});

    /// One test per identifier: results that share one are attempts at that
    /// test, in the order given, and the verdict is worked out over them. A
    /// test that both failed and passed is flaky, as it is in a run of the
    /// runner.
    [[nodiscard]] std::vector<TestResult>
    MergeImported(std::vector<TestResult> results);

    /// A result without attempts yet.
    [[nodiscard]] TestResult
    ImportedTest(std::string name, std::string className);

    /// One attempt as a report describes it. A failure gets an assertion
    /// record with `message` and `details`, so the analysis treats it as it
    /// treats a failure of Catch3's own; `category` defaults by outcome.
    [[nodiscard]] Attempt
    ImportedAttempt(Outcome outcome,
                    std::int64_t durationMicroseconds,
                    std::string message = {},
                    std::string details = {},
                    FailureCategory category = FailureCategory::kNone);

    /// Seconds as a report gives them; what makes no sense as a duration
    /// counts as zero.
    [[nodiscard]] std::int64_t
    ImportedMicroseconds(double seconds);

    [[nodiscard]] std::string_view
    ImportTrim(std::string_view text);

    [[nodiscard]] bool
    ImportStartsWith(std::string_view text, std::string_view prefix);

    /// The lines of a text, without their line endings.
    [[nodiscard]] std::vector<std::string_view>
    ImportLines(std::string_view text);

    /// A format of Catch3's own or CTRF, from a parsed document.
    [[nodiscard]] std::optional<ImportFormat>
    ImportJsonFormat(const JsonValue &value);

    /// Whether a line reports a test in TAP.
    [[nodiscard]] bool
    IsTapTestLine(std::string_view line);

    /// A `--json` report or catch3-results lines.
    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseCatch3Report(std::string_view text);

    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseCtrf(std::string_view text);

    /// `source` becomes the class of its tests: TAP names them only within
    /// one stream.
    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseTap(std::string_view text, const std::string &source);

    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseJUnit(std::string_view text);

    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseXUnit(std::string_view text);

    [[nodiscard]] Expected<std::vector<TestResult>>
    ParseTrx(std::string_view text);
} // namespace Progmasoft::Catch3::Runner
