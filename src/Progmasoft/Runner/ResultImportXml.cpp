// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// The XML reports: JUnit, xUnit.net v2 and Visual Studio's TRX.
#include <Progmasoft/Runner/ResultImport.hpp>
#include <cstdlib>
#include <map>
#include <string_view>
#include <utility>

#include "../PugiXml.inl"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        namespace ImportXml
        {
            Expected<bool>
            Load(pugi::xml_document &document, std::string_view text)
            {
                const pugi::xml_parse_result parsed
                    = document.load_buffer(text.data(), text.size());
                if (!parsed)
                {
                    return Error{ std::string{ "it is not valid XML: " }
                                  + parsed.description() + " at byte "
                                  + std::to_string(parsed.offset) };
                }
                return true;
            }

            double
            NumberOf(const char *text)
            {
                std::string digits;
                for (; *text != '\0'; ++text)
                {
                    if (*text != ',')
                    {
                        digits.push_back(*text);
                    }
                }
                return std::strtod(digits.c_str(), nullptr);
            }

            std::string
            TextOf(pugi::xml_node node)
            {
                return node.text().get();
            }

            /// `error` and `failure` of JUnit and what Surefire writes for
            /// retries, which have the same shape.
            Attempt
            JUnitFailure(pugi::xml_node node, FailureCategory category)
            {
                std::string details = TextOf(node);
                if (const auto trace = node.child("stackTrace"))
                {
                    details = TextOf(trace);
                }
                Attempt attempt
                    = ImportedAttempt(Outcome::kFailed,
                                      ImportedMicroseconds(NumberOf(
                                          node.attribute("time").value())),
                                      node.attribute("message").value(),
                                      std::move(details),
                                      category);
                attempt.assertions.front().macro
                    = node.attribute("type").value();
                attempt.standardOutput = TextOf(node.child("system-out"));
                attempt.standardError = TextOf(node.child("system-err"));
                return attempt;
            }

            bool
            NameIs(pugi::xml_node node, std::string_view name)
            {
                return name == node.name();
            }

            bool
            NameEndsWithError(pugi::xml_node node)
            {
                return NameIs(node, "flakyError") || NameIs(node, "rerunError");
            }

            bool
            NotRunStatus(pugi::xml_node testCase)
            {
                const std::string_view status
                    = testCase.attribute("status").value();
                const std::string_view result
                    = testCase.attribute("result").value();
                return status == "notrun" || status == "skipped"
                       || status == "disabled" || result == "skipped"
                       || result == "suppressed";
            }

            void
            ReadJUnitCase(pugi::xml_node testCase,
                          pugi::xml_node suite,
                          std::vector<TestResult> &tests)
            {
                TestResult test
                    = ImportedTest(testCase.attribute("name").value(),
                                   testCase.attribute("classname").value());
                test.file = testCase.attribute("file").value();
                for (auto outer = suite; test.file.empty() && outer;
                     outer = outer.parent())
                {
                    test.file = outer.attribute("file").value();
                }
                test.line = testCase.attribute("line").as_ullong();

                std::vector<Attempt> failures;
                std::optional<Attempt> skipped;
                for (const auto child : testCase.children())
                {
                    if (NameIs(child, "flakyFailure")
                        || NameIs(child, "rerunFailure")
                        || NameEndsWithError(child))
                    {
                        test.attempts.push_back(JUnitFailure(
                            child,
                            NameEndsWithError(child)
                                ? FailureCategory::kUnexpectedException
                                : FailureCategory::kAssertion));
                    }
                    else if (NameIs(child, "failure"))
                    {
                        failures.push_back(
                            JUnitFailure(child, FailureCategory::kAssertion));
                    }
                    else if (NameIs(child, "error"))
                    {
                        failures.push_back(JUnitFailure(
                            child,
                            FailureCategory::kUnexpectedException));
                    }
                    else if (NameIs(child, "skipped"))
                    {
                        std::string reason = child.attribute("message").value();
                        skipped = ImportedAttempt(
                            Outcome::kSkipped,
                            0,
                            reason.empty() ? TextOf(child) : std::move(reason));
                    }
                }

                const std::int64_t duration = ImportedMicroseconds(
                    NumberOf(testCase.attribute("time").value()));
                Attempt verdict;
                if (!failures.empty())
                {
                    // Several failures are one attempt with several findings.
                    verdict = std::move(failures.front());
                    for (std::size_t index = 1; index < failures.size();
                         ++index)
                    {
                        auto &record = failures[index].assertions.front();
                        verdict.assertions.push_back(std::move(record));
                        ++verdict.assertionsFailed;
                    }
                    verdict.durationMicroseconds = duration;
                }
                else if (skipped)
                {
                    verdict = std::move(*skipped);
                }
                else if (NotRunStatus(testCase))
                {
                    verdict = ImportedAttempt(Outcome::kSkipped, 0);
                }
                else
                {
                    verdict = ImportedAttempt(Outcome::kPassed, duration);
                }
                if (verdict.standardOutput.empty())
                {
                    verdict.standardOutput
                        = TextOf(testCase.child("system-out"));
                }
                if (verdict.standardError.empty())
                {
                    verdict.standardError
                        = TextOf(testCase.child("system-err"));
                }
                test.attempts.push_back(std::move(verdict));
                tests.push_back(std::move(test));
            }

            void
            ReadJUnitSuite(pugi::xml_node suite, std::vector<TestResult> &tests)
            {
                for (const auto child : suite.children())
                {
                    if (NameIs(child, "testcase"))
                    {
                        ReadJUnitCase(child, suite, tests);
                    }
                    else if (NameIs(child, "testsuite"))
                    {
                        ReadJUnitSuite(child, tests);
                    }
                }
            }

            void
            ReadXUnitTest(pugi::xml_node node, std::vector<TestResult> &tests)
            {
                TestResult test = ImportedTest(node.attribute("name").value(),
                                               node.attribute("type").value());
                test.file = node.attribute("source-file").value();
                test.line = node.attribute("source-line").as_ullong();
                for (const auto trait : node.child("traits").children("trait"))
                {
                    test.tags.push_back(
                        std::string{ trait.attribute("name").value() } + "="
                        + trait.attribute("value").value());
                }
                const std::string_view result
                    = node.attribute("result").value();
                const std::int64_t duration = ImportedMicroseconds(
                    NumberOf(node.attribute("time").value()));
                Attempt attempt;
                if (result == "Pass")
                {
                    attempt = ImportedAttempt(Outcome::kPassed, duration);
                }
                else if (result == "Fail")
                {
                    const auto failure = node.child("failure");
                    attempt
                        = ImportedAttempt(Outcome::kFailed,
                                          duration,
                                          TextOf(failure.child("message")),
                                          TextOf(failure.child("stack-trace")));
                    attempt.assertions.front().macro
                        = failure.attribute("exception-type").value();
                }
                else if (result == "Skip")
                {
                    attempt = ImportedAttempt(Outcome::kSkipped,
                                              0,
                                              TextOf(node.child("reason")));
                }
                else
                {
                    attempt = ImportedAttempt(Outcome::kNotRun, 0);
                }
                if (attempt.standardOutput.empty())
                {
                    attempt.standardOutput = TextOf(node.child("output"));
                }
                test.attempts.push_back(std::move(attempt));
                tests.push_back(std::move(test));
            }

            /// A failure outside any test, such as in a fixture.
            void
            ReadXUnitErrors(pugi::xml_node assembly,
                            std::vector<TestResult> &tests)
            {
                for (const auto error :
                     assembly.child("errors").children("error"))
                {
                    const auto failure = error.child("failure");
                    TestResult test = ImportedTest(
                        std::string{ error.attribute("type").value() } + " "
                            + error.attribute("name").value(),
                        assembly.attribute("name").value());
                    test.attempts.push_back(
                        ImportedAttempt(Outcome::kFailed,
                                        0,
                                        TextOf(failure.child("message")),
                                        TextOf(failure.child("stack-trace")),
                                        FailureCategory::kInfrastructure));
                    tests.push_back(std::move(test));
                }
            }

            void
            ReadXUnitAssembly(pugi::xml_node assembly,
                              std::vector<TestResult> &tests)
            {
                ReadXUnitErrors(assembly, tests);
                for (const auto child : assembly.children())
                {
                    if (NameIs(child, "test"))
                    {
                        ReadXUnitTest(child, tests);
                    }
                    else if (NameIs(child, "collection"))
                    {
                        for (const auto test : child.children("test"))
                        {
                            ReadXUnitTest(test, tests);
                        }
                    }
                }
            }

            /// `[days.]hours:minutes:seconds[.fraction]`.
            std::int64_t
            TrxDuration(std::string_view text)
            {
                double seconds = 0.0;
                std::string field;
                const auto addField = [&] {
                    seconds = seconds * 60.0 + NumberOf(field.c_str());
                    field.clear();
                };
                double days = 0.0;
                for (std::size_t index = 0; index < text.size(); ++index)
                {
                    if (text[index] == ':')
                    {
                        addField();
                    }
                    else if (text[index] == '.'
                             && text.find(':', index) != std::string_view::npos)
                    {
                        days = NumberOf(field.c_str());
                        field.clear();
                    }
                    else
                    {
                        field.push_back(text[index]);
                    }
                }
                addField();
                return ImportedMicroseconds(seconds + days * 86400.0);
            }

            struct TrxOutcome final
            {
                Outcome outcome = Outcome::kNotRun;
                FailureCategory category = FailureCategory::kNone;
            };

            TrxOutcome
            TrxOutcomeOf(std::string_view outcome)
            {
                if (outcome == "Passed" || outcome == "PassedButRunAborted"
                    || outcome == "Warning" || outcome == "Completed")
                {
                    return { Outcome::kPassed, FailureCategory::kNone };
                }
                if (outcome == "Failed")
                {
                    return { Outcome::kFailed, FailureCategory::kAssertion };
                }
                if (outcome == "Error")
                {
                    return { Outcome::kFailed,
                             FailureCategory::kUnexpectedException };
                }
                if (outcome == "Timeout")
                {
                    return { Outcome::kTimedOut, FailureCategory::kTimeout };
                }
                if (outcome == "Aborted")
                {
                    return { Outcome::kCrashed, FailureCategory::kCrash };
                }
                if (outcome == "Disconnected")
                {
                    return { Outcome::kCrashed,
                             FailureCategory::kInfrastructure };
                }
                if (outcome == "NotExecuted" || outcome == "Inconclusive"
                    || outcome == "NotRunnable")
                {
                    return { Outcome::kSkipped, FailureCategory::kNone };
                }
                return {};
            }

            /// The class of a test: the type without its assembly.
            std::string
            TrxClass(std::string_view className)
            {
                return std::string{ className.substr(0, className.find(',')) };
            }

            void
            ReadTrxResult(pugi::xml_node result,
                          const std::map<std::string, std::string> &classes,
                          std::vector<TestResult> &tests)
            {
                // A data-driven test: the rows are what ran.
                if (const auto inner = result.child("InnerResults"))
                {
                    for (const auto row : inner.children())
                    {
                        ReadTrxResult(row, classes, tests);
                    }
                    return;
                }
                const auto found
                    = classes.find(result.attribute("testId").value());
                TestResult test = ImportedTest(
                    result.attribute("testName").value(),
                    found == classes.end() ? std::string{} : found->second);
                const auto [outcome, category]
                    = TrxOutcomeOf(result.attribute("outcome").value());
                const auto output = result.child("Output");
                const auto error = output.child("ErrorInfo");
                std::string message = TextOf(error.child("Message"));
                Attempt attempt = ImportedAttempt(
                    outcome,
                    TrxDuration(result.attribute("duration").value()),
                    outcome == Outcome::kSkipped
                        ? TextOf(output.child("StdOut"))
                        : std::move(message),
                    TextOf(error.child("StackTrace")),
                    category);
                if (outcome != Outcome::kSkipped)
                {
                    attempt.standardOutput = TextOf(output.child("StdOut"));
                }
                attempt.standardError = TextOf(output.child("StdErr"));
                test.attempts.push_back(std::move(attempt));
                tests.push_back(std::move(test));
            }
        } // namespace ImportXml
    } // namespace

    Expected<std::vector<TestResult>>
    ParseJUnit(std::string_view text)
    {
        pugi::xml_document document;
        if (auto loaded = ImportXml::Load(document, text); !loaded)
        {
            return loaded.GetError();
        }
        const pugi::xml_node root = document.document_element();
        std::vector<TestResult> tests;
        if (ImportXml::NameIs(root, "testsuites")
            || ImportXml::NameIs(root, "testsuite"))
        {
            ImportXml::ReadJUnitSuite(root, tests);
            return tests;
        }
        return Error{ "a JUnit report has testsuites or testsuite as its root, "
                      "not "
                      + std::string{ root.name() } };
    }

    Expected<std::vector<TestResult>>
    ParseXUnit(std::string_view text)
    {
        pugi::xml_document document;
        if (auto loaded = ImportXml::Load(document, text); !loaded)
        {
            return loaded.GetError();
        }
        const pugi::xml_node root = document.document_element();
        std::vector<TestResult> tests;
        if (ImportXml::NameIs(root, "assembly"))
        {
            ImportXml::ReadXUnitAssembly(root, tests);
            return tests;
        }
        if (ImportXml::NameIs(root, "assemblies"))
        {
            for (const auto assembly : root.children("assembly"))
            {
                ImportXml::ReadXUnitAssembly(assembly, tests);
            }
            return tests;
        }
        return Error{ "an xUnit report has assemblies or assembly as its root, "
                      "not "
                      + std::string{ root.name() } };
    }

    Expected<std::vector<TestResult>>
    ParseTrx(std::string_view text)
    {
        pugi::xml_document document;
        if (auto loaded = ImportXml::Load(document, text); !loaded)
        {
            return loaded.GetError();
        }
        const pugi::xml_node root = document.document_element();
        if (!ImportXml::NameIs(root, "TestRun"))
        {
            return Error{ "a TRX report has TestRun as its root, not "
                          + std::string{ root.name() } };
        }
        std::map<std::string, std::string> classes;
        for (const auto definition : root.child("TestDefinitions").children())
        {
            classes[definition.attribute("id").value()] = ImportXml::TrxClass(
                definition.child("TestMethod").attribute("className").value());
        }
        std::vector<TestResult> tests;
        for (const auto result : root.child("Results").children())
        {
            ImportXml::ReadTrxResult(result, classes, tests);
        }
        return tests;
    }
} // namespace Progmasoft::Catch3::Runner
