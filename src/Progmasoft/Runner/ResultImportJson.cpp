// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// The JSON reports: Catch3's own and CTRF.
#include <Progmasoft/Runner/Json.hpp>
#include <Progmasoft/Runner/ResultImport.hpp>
#include <Progmasoft/Runner/ResultsJson.hpp>
#include <utility>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        namespace ImportJson
        {
            /// A format of Catch3's own or CTRF, from the parsed document.
            std::optional<ImportFormat>
            FormatOf(const JsonValue &value)
            {
                if (value["format"].AsString() == "catch3-run"
                    || value["format"].AsString() == "catch3-results")
                {
                    return ImportFormat::kCatch3;
                }
                if (value["results"]["tests"].IsArray())
                {
                    return ImportFormat::kCtrf;
                }
                return std::nullopt;
            }

            /// What a test owes the verdict of an earlier run is worked out
            /// again here, from this run's point of view.
            void
            Forget(TestResult &test)
            {
                test.quarantined = false;
                test.history = FailureHistory::kUnknown;
                test.selectionReason.clear();
                test.signature.clear();
                test.analysis.clear();
                test.replayCommand.clear();
                test.predictedDurationMicroseconds = 0;
                test.predictedFailureProbability = 0.0;
            }

            Expected<TestResult>
            ReadLine(const JsonValue &value)
            {
                TestResult test = TestResultFromJson(value);
                Forget(test);
                if (test.name.empty())
                {
                    test.name = test.id;
                }
                if (test.name.empty())
                {
                    return Error{ "a test needs an 'id' or a 'name'" };
                }
                const std::string &outcome = value["outcome"].AsString();
                if (!OutcomeFromName(outcome))
                {
                    return Error{ "'" + test.name + "' has the outcome '"
                                  + outcome + "'" };
                }
                if (test.attempts.empty())
                {
                    Attempt attempt = ImportedAttempt(
                        test.outcome,
                        value["durationMicroseconds"].AsInteger(),
                        value["message"].AsString(),
                        value["details"].AsString(),
                        test.category);
                    attempt.standardOutput = value["standardOutput"].AsString();
                    attempt.standardError = value["standardError"].AsString();
                    test.attempts.push_back(std::move(attempt));
                }
                return test;
            }

            Expected<std::vector<TestResult>>
            ParseLines(std::string_view text)
            {
                std::vector<TestResult> tests;
                std::size_t number = 0;
                bool started = false;
                for (const auto line : ImportLines(text))
                {
                    ++number;
                    if (ImportTrim(line).empty())
                    {
                        continue;
                    }
                    const std::string where = "line " + std::to_string(number);
                    auto value = ParseJson(line);
                    if (!value)
                    {
                        return value.GetError().Context(where);
                    }
                    if (!started)
                    {
                        started = true;
                        if ((*value)["format"].AsString() != "catch3-results")
                        {
                            return Error{ "the first line must name the format "
                                          "catch3-results" };
                        }
                        if ((*value)["version"].AsInteger()
                            > kResultsFormatVersion)
                        {
                            return Error{ "the format is newer than this "
                                          "version of Catch3 reads" };
                        }
                        continue;
                    }
                    auto test = ReadLine(*value);
                    if (!test)
                    {
                        return test.GetError().Context(where);
                    }
                    tests.push_back(std::move(*test));
                }
                return tests;
            }

            Expected<std::vector<TestResult>>
            ParseCatch3(std::string_view text)
            {
                auto whole = ParseJson(text);
                if (!whole)
                {
                    return ParseLines(text);
                }
                if ((*whole)["format"].AsString() != "catch3-run")
                {
                    return ParseLines(text);
                }
                auto run = RunResultFromJson(*whole);
                if (!run)
                {
                    return run.GetError();
                }
                for (auto &test : run->tests)
                {
                    Forget(test);
                }
                return std::move(run->tests);
            }

            std::string
            TextOf(const JsonValue &value)
            {
                if (!value.IsArray())
                {
                    return value.AsString();
                }
                std::string text;
                for (const auto &item : value.Items())
                {
                    text += item.AsString() + "\n";
                }
                return text;
            }

            Outcome
            CtrfOutcome(const std::string &status)
            {
                if (status == "passed")
                {
                    return Outcome::kPassed;
                }
                if (status == "failed")
                {
                    return Outcome::kFailed;
                }
                if (status == "skipped" || status == "pending")
                {
                    return Outcome::kSkipped;
                }
                return Outcome::kNotRun;
            }

            /// A test, or one retry of it: the two share these fields.
            Attempt
            CtrfAttempt(const JsonValue &value)
            {
                Attempt attempt = ImportedAttempt(
                    CtrfOutcome(value["status"].AsString()),
                    ImportedMicroseconds(value["duration"].AsNumber() / 1e3),
                    value["message"].AsString(),
                    value["trace"].AsString());
                attempt.standardOutput = TextOf(value["stdout"]);
                attempt.standardError = TextOf(value["stderr"]);
                return attempt;
            }

            std::string
            CtrfSuite(const JsonValue &suite)
            {
                if (!suite.IsArray())
                {
                    return suite.AsString();
                }
                std::string text;
                for (const auto &item : suite.Items())
                {
                    text += (text.empty() ? "" : " > ") + item.AsString();
                }
                return text;
            }

            Expected<std::vector<TestResult>>
            ParseCtrf(std::string_view text)
            {
                auto document = ParseJson(text);
                if (!document)
                {
                    return document.GetError();
                }
                const JsonValue &tests = (*document)["results"]["tests"];
                if (!tests.IsArray())
                {
                    return Error{ "a CTRF report has results.tests" };
                }
                std::vector<TestResult> results;
                for (const auto &item : tests.Items())
                {
                    const std::string &name = item["name"].AsString();
                    if (name.empty())
                    {
                        return Error{ "a CTRF test has no name" };
                    }
                    TestResult test
                        = ImportedTest(name, CtrfSuite(item["suite"]));
                    test.file = item["filePath"].AsString();
                    test.line
                        = static_cast<std::size_t>(item["line"].AsUnsigned());
                    test.flaky = item["flaky"].AsBoolean();
                    for (const auto &tag : item["tags"].Items())
                    {
                        test.tags.push_back(tag.AsString());
                    }
                    for (const auto &retry : item["retryAttempts"].Items())
                    {
                        test.attempts.push_back(CtrfAttempt(retry));
                    }
                    // The retries may or may not list the last attempt, which
                    // is the test itself.
                    Attempt last = CtrfAttempt(item);
                    if (test.attempts.empty()
                        || test.attempts.back().outcome != last.outcome)
                    {
                        test.attempts.push_back(std::move(last));
                    }
                    results.push_back(std::move(test));
                }
                return results;
            }
        } // namespace ImportJson
    } // namespace

    std::optional<ImportFormat>
    ImportJsonFormat(const JsonValue &value)
    {
        return ImportJson::FormatOf(value);
    }

    Expected<std::vector<TestResult>>
    ParseCatch3Report(std::string_view text)
    {
        return ImportJson::ParseCatch3(text);
    }

    Expected<std::vector<TestResult>>
    ParseCtrf(std::string_view text)
    {
        return ImportJson::ParseCtrf(text);
    }
} // namespace Progmasoft::Catch3::Runner
