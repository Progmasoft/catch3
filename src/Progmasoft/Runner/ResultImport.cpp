// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <Progmasoft/Runner/History.hpp>
#include <Progmasoft/Runner/Json.hpp>
#include <Progmasoft/Runner/ResultImport.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <map>
#include <utility>

namespace Progmasoft::Catch3::Runner
{
    std::string_view
    ImportTrim(std::string_view text)
    {
        const auto blank = [](char character) {
            return std::isspace(static_cast<unsigned char>(character)) != 0;
        };
        while (!text.empty() && blank(text.front()))
        {
            text.remove_prefix(1);
        }
        while (!text.empty() && blank(text.back()))
        {
            text.remove_suffix(1);
        }
        return text;
    }

    bool
    ImportStartsWith(std::string_view text, std::string_view prefix)
    {
        return text.substr(0, prefix.size()) == prefix;
    }

    std::vector<std::string_view>
    ImportLines(std::string_view text)
    {
        std::vector<std::string_view> lines;
        while (!text.empty())
        {
            const auto end = text.find('\n');
            auto line = text.substr(0, end);
            if (!line.empty() && line.back() == '\r')
            {
                line.remove_suffix(1);
            }
            lines.push_back(line);
            if (end == std::string_view::npos)
            {
                break;
            }
            text.remove_prefix(end + 1);
        }
        return lines;
    }

    namespace
    {
        namespace ImportText
        {
            /// Without a byte order mark and the white space before the
            /// first character that counts.
            std::string_view
            Contents(std::string_view text)
            {
                if (ImportStartsWith(text, "\xEF\xBB\xBF"))
                {
                    text.remove_prefix(3);
                }
                return ImportTrim(text);
            }

            /// The name of the root element of an XML document.
            std::string_view
            RootElement(std::string_view text)
            {
                while (!text.empty())
                {
                    const auto open = text.find('<');
                    if (open == std::string_view::npos
                        || open + 1 >= text.size())
                    {
                        return {};
                    }
                    text.remove_prefix(open + 1);
                    if (text.front() == '?' || text.front() == '!')
                    {
                        const auto close = text.find('>');
                        if (close == std::string_view::npos)
                        {
                            return {};
                        }
                        text.remove_prefix(close + 1);
                        continue;
                    }
                    return text.substr(0, text.find_first_of(" \t\r\n/>"));
                }
                return {};
            }
        } // namespace ImportText
    } // namespace

    std::optional<ImportFormat>
    DetectImportFormat(std::string_view text)
    {
        const std::string_view contents = ImportText::Contents(text);
        if (contents.empty())
        {
            return std::nullopt;
        }
        if (contents.front() == '<')
        {
            const std::string_view root = ImportText::RootElement(contents);
            if (root == "testsuites" || root == "testsuite")
            {
                return ImportFormat::kJUnit;
            }
            if (root == "assemblies" || root == "assembly")
            {
                return ImportFormat::kXUnit;
            }
            if (root == "TestRun")
            {
                return ImportFormat::kTrx;
            }
            return std::nullopt;
        }
        if (contents.front() == '{')
        {
            auto whole = ParseJson(contents);
            if (!whole)
            {
                // JSON Lines: the first line says what the others are.
                whole = ParseJson(contents.substr(0, contents.find('\n')));
            }
            return whole ? ImportJsonFormat(*whole) : std::nullopt;
        }
        for (const auto raw : ImportLines(contents))
        {
            const std::string_view line = ImportTrim(raw);
            if (line.empty() || line.front() == '#')
            {
                continue;
            }
            if (ImportStartsWith(line, "TAP version")
                || ImportStartsWith(line, "1..") || IsTapTestLine(line))
            {
                return ImportFormat::kTap;
            }
            break;
        }
        return std::nullopt;
    }

    Expected<std::vector<TestResult>>
    ParseImport(std::string_view text, ImportFormat format)
    {
        const std::string_view contents = ImportText::Contents(text);
        switch (format)
        {
            case ImportFormat::kAuto:
                break;
            case ImportFormat::kCatch3:
                return ParseCatch3Report(contents);
            case ImportFormat::kJUnit:
                return ParseJUnit(contents);
            case ImportFormat::kXUnit:
                return ParseXUnit(contents);
            case ImportFormat::kTrx:
                return ParseTrx(contents);
            case ImportFormat::kTap:
                return ParseTap(contents);
            case ImportFormat::kCtrf:
                return ParseCtrf(contents);
        }
        const auto detected = DetectImportFormat(contents);
        if (!detected)
        {
            return Error{ "its format is not one Catch3 knows; "
                          "--import-format names it" };
        }
        return ParseImport(contents, *detected);
    }

    std::vector<TestResult>
    MergeImported(std::vector<TestResult> results)
    {
        std::vector<TestResult> tests;
        std::map<std::string, std::size_t> positions;
        for (auto &result : results)
        {
            if (result.id.empty())
            {
                result.id = History::KeyOf(result);
            }
            const auto [position, added]
                = positions.emplace(result.id, tests.size());
            if (added)
            {
                tests.push_back(std::move(result));
                continue;
            }
            TestResult &test = tests[position->second];
            test.flaky = test.flaky || result.flaky;
            std::move(result.attempts.begin(),
                      result.attempts.end(),
                      std::back_inserter(test.attempts));
        }
        for (auto &test : tests)
        {
            std::size_t passes = 0;
            std::size_t failures = 0;
            for (std::size_t index = 0; index < test.attempts.size(); ++index)
            {
                Attempt &attempt = test.attempts[index];
                attempt.number = index + 1;
                passes += attempt.outcome == Outcome::kPassed ? 1 : 0;
                failures += IsFailure(attempt.outcome) ? 1 : 0;
            }
            if (test.attempts.empty())
            {
                continue;
            }
            test.outcome = test.attempts.back().outcome;
            test.category = test.attempts.back().category;
            if (passes > 0 && failures > 0)
            {
                test.outcome = Outcome::kPassed;
                test.category = FailureCategory::kNone;
                test.flaky = true;
                test.analysis.push_back(
                    "flaky: failed " + std::to_string(failures) + " of "
                    + std::to_string(test.attempts.size()) + " attempts");
            }
        }
        return tests;
    }

    TestResult
    ImportedTest(std::string name, std::string className)
    {
        TestResult test;
        test.name = std::move(name);
        test.className = std::move(className);
        test.id = History::KeyOf(test);
        return test;
    }

    Attempt
    ImportedAttempt(Outcome outcome,
                    std::int64_t durationMicroseconds,
                    std::string message,
                    std::string details,
                    FailureCategory category)
    {
        Attempt attempt;
        attempt.outcome = outcome;
        attempt.durationMicroseconds
            = std::max<std::int64_t>(durationMicroseconds, 0);
        if (outcome == Outcome::kSkipped)
        {
            attempt.standardOutput = std::move(message);
            return attempt;
        }
        if (outcome != Outcome::kFailedAsExpected && !IsFailure(outcome))
        {
            return attempt;
        }
        if (category == FailureCategory::kNone)
        {
            category = outcome == Outcome::kCrashed ? FailureCategory::kCrash
                       : outcome == Outcome::kTimedOut
                           ? FailureCategory::kTimeout
                           : FailureCategory::kAssertion;
        }
        if (message.empty())
        {
            message = details.substr(0, details.find('\n'));
        }
        attempt.category = category;
        attempt.assertionsFailed = 1;
        AssertionRecord assertion;
        assertion.message = std::move(message);
        assertion.expansion = std::move(details);
        assertion.category = category;
        assertion.allowed = outcome == Outcome::kFailedAsExpected;
        attempt.assertions.push_back(std::move(assertion));
        return attempt;
    }

    std::int64_t
    ImportedMicroseconds(double seconds)
    {
        // About thirty years; a longer duration is not a duration.
        constexpr double kLongest = 1e9;
        if (!std::isfinite(seconds) || seconds <= 0.0 || seconds > kLongest)
        {
            return 0;
        }
        return static_cast<std::int64_t>(std::llround(seconds * 1e6));
    }
} // namespace Progmasoft::Catch3::Runner
