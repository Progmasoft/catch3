// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <Progmasoft/Catch3/Results.hpp>
#include <Progmasoft/Catch3/XmlWriter.hpp>

#include "PugiXml.inl"

#include <ostream>
#include <sstream>
#include <string_view>

namespace Progmasoft::Catch3
{
    namespace
    {
        struct DecodedCharacter final
        {
            char32_t value = 0;
            std::size_t length = 0;
        };

        DecodedCharacter
        DecodeUtf8(std::string_view text, std::size_t offset) noexcept
        {
            const auto first = static_cast<unsigned char>(text[offset]);
            if (first < 0x80)
            {
                return {first, 1};
            }
            std::size_t length = 0;
            char32_t value = 0;
            char32_t minimum = 0;
            if (first >= 0xC2 && first <= 0xDF)
            {
                length = 2;
                value = first & 0x1F;
                minimum = 0x80;
            }
            else if (first >= 0xE0 && first <= 0xEF)
            {
                length = 3;
                value = first & 0x0F;
                minimum = 0x800;
            }
            else if (first >= 0xF0 && first <= 0xF4)
            {
                length = 4;
                value = first & 0x07;
                minimum = 0x10000;
            }
            else
            {
                return {0xFFFD, 0};
            }
            if (offset + length > text.size())
            {
                return {0xFFFD, 0};
            }
            for (std::size_t index = 1; index < length; ++index)
            {
                const auto continuation =
                    static_cast<unsigned char>(text[offset + index]);
                if ((continuation & 0xC0) != 0x80)
                {
                    return {0xFFFD, 0};
                }
                value = static_cast<char32_t>((value << 6) |
                                              (continuation & 0x3F));
            }
            if (value < minimum || value > 0x10FFFF ||
                (value >= 0xD800 && value <= 0xDFFF))
            {
                return {0xFFFD, 0};
            }
            return {value, length};
        }

        bool
        IsXmlCharacter(char32_t value) noexcept
        {
            return value == 0x09 || value == 0x0A || value == 0x0D ||
                   (value >= 0x20 && value <= 0xD7FF) ||
                   (value >= 0xE000 && value <= 0xFFFD) ||
                   (value >= 0x10000 && value <= 0x10FFFF);
        }

        /// The text as XML can hold it: every byte that is not part of a
        /// well-formed character XML allows becomes U+FFFD, replaced and not
        /// dropped, so the reader sees that something was there. pugixml
        /// escapes the rest.
        std::string
        XmlText(std::string_view value)
        {
            std::string result;
            result.reserve(value.size());
            for (std::size_t index = 0; index < value.size();)
            {
                const DecodedCharacter character = DecodeUtf8(value, index);
                if (character.length == 0 || !IsXmlCharacter(character.value))
                {
                    result += "\xEF\xBF\xBD";
                    ++index;
                    continue;
                }
                result.append(value.data() + index, character.length);
                index += character.length;
            }
            return result;
        }

        void
        AddAttribute(pugi::xml_node node,
                     const char *name,
                     std::string_view value)
        {
            node.append_attribute(name).set_value(XmlText(value).c_str());
        }

        void
        AddText(pugi::xml_node node, std::string_view value)
        {
            node.text().set(XmlText(value).c_str());
        }

        void
        AddTextElement(pugi::xml_node parent,
                       const char *name,
                       std::string_view value)
        {
            AddText(parent.append_child(name), value);
        }

        std::string
        JUnitSeconds(std::int64_t microseconds)
        {
            const bool negative = microseconds < 0;
            const auto magnitude = negative ? -microseconds : microseconds;
            std::string fraction = std::to_string(magnitude % 1000000);
            fraction.insert(0, 6 - fraction.size(), '0');
            return (negative ? "-" : "") + std::to_string(magnitude / 1000000) +
                   "." + fraction;
        }

        std::string
        Output(const Attempt &attempt, bool errors)
        {
            if (errors)
            {
                return attempt.standardError + attempt.processOutput;
            }
            return attempt.standardOutput;
        }

        /// An earlier attempt: `<rerunFailure>` before a final failure,
        /// `<flakyFailure>` before a final pass.
        void
        AddEarlierAttempt(pugi::xml_node testCase,
                          const Attempt &attempt,
                          const char *element)
        {
            pugi::xml_node node = testCase.append_child(element);
            AddAttribute(node, "message", SummarizeFailure(attempt));
            AddAttribute(node, "type", CategoryName(attempt.category));
            AddTextElement(node, "stackTrace", DescribeFailure(attempt));
            if (!attempt.standardOutput.empty())
            {
                AddTextElement(node, "system-out", Output(attempt, false));
            }
            if (!Output(attempt, true).empty())
            {
                AddTextElement(node, "system-err", Output(attempt, true));
            }
        }

        std::string
        SkipReason(const TestResult &test)
        {
            switch (test.outcome)
            {
                case Outcome::kBlocked:
                    return test.analysis.empty() ? "a dependency did not pass"
                                                 : test.analysis.front();
                case Outcome::kNotRun:
                    return "the run stopped before this test";
                case Outcome::kUpToDate:
                    return "passed before; nothing it depends on changed";
                default:
                    return "skipped";
            }
        }

        void
        AddTestCase(pugi::xml_node suite,
                    const TestResult &test,
                    std::string_view suiteName)
        {
            pugi::xml_node node = suite.append_child("testcase");
            AddAttribute(node,
                         "classname",
                         test.className.empty() ? suiteName : test.className);
            AddAttribute(node, "name", test.name);
            AddAttribute(node, "time", JUnitSeconds(test.DurationMicroseconds()));
            if (!test.file.empty())
            {
                AddAttribute(node, "file", test.file);
                AddAttribute(node, "line", std::to_string(test.line));
            }

            const Attempt *last =
                test.attempts.empty() ? nullptr : &test.attempts.back();
            const bool failedRun = IsFailure(test.outcome);
            if (failedRun && last != nullptr)
            {
                const bool error = test.outcome != Outcome::kFailed ||
                                   last->category ==
                                       FailureCategory::kInfrastructure;
                pugi::xml_node failure =
                    node.append_child(error ? "error" : "failure");
                AddAttribute(failure, "message", SummarizeFailure(*last));
                AddAttribute(failure, "type", CategoryName(test.category));
                std::string details = DescribeFailure(*last);
                for (const auto &finding : test.analysis)
                {
                    details += "analysis: " + finding + "\n";
                }
                if (!test.replayCommand.empty())
                {
                    details += "replay: " + test.replayCommand + "\n";
                }
                AddText(failure, details);
            }
            else if (test.outcome == Outcome::kSkipped ||
                     test.outcome == Outcome::kBlocked ||
                     test.outcome == Outcome::kNotRun ||
                     test.outcome == Outcome::kUpToDate)
            {
                AddAttribute(
                    node.append_child("skipped"), "message", SkipReason(test));
            }
            // Every earlier failed attempt, so retries stay visible.
            for (std::size_t index = 0; index + 1 < test.attempts.size();
                 ++index)
            {
                const Attempt &attempt = test.attempts[index];
                if (!IsFailure(attempt.outcome))
                {
                    continue;
                }
                AddEarlierAttempt(node,
                                  attempt,
                                  failedRun ? "rerunFailure" : "flakyFailure");
            }
            if (last != nullptr && !last->standardOutput.empty())
            {
                AddTextElement(node, "system-out", Output(*last, false));
            }
            if (last != nullptr && !Output(*last, true).empty())
            {
                AddTextElement(node, "system-err", Output(*last, true));
            }
        }

        void
        AddCounts(pugi::xml_node node,
                  const RunResult &run,
                  const RunSummary &summary)
        {
            AddAttribute(node, "name", run.binary);
            AddAttribute(node, "tests", std::to_string(summary.tests));
            AddAttribute(node, "failures", std::to_string(summary.failed));
            AddAttribute(node,
                         "errors",
                         std::to_string(summary.crashed + summary.timedOut));
            AddAttribute(node,
                         "skipped",
                         std::to_string(summary.skipped + summary.blocked +
                                        summary.notRun + summary.upToDate));
            AddAttribute(node, "time", JUnitSeconds(run.wallMicroseconds));
        }
    } // namespace

    bool
    JUnitXmlWriter::Write(const RunResult &run, std::ostream &output)
    {
        const RunSummary summary = Summarize(run);
        pugi::xml_document document;
        pugi::xml_node declaration =
            document.append_child(pugi::node_declaration);
        declaration.append_attribute("version").set_value("1.0");
        declaration.append_attribute("encoding").set_value("UTF-8");

        pugi::xml_node suites = document.append_child("testsuites");
        AddCounts(suites, run, summary);
        pugi::xml_node suite = suites.append_child("testsuite");
        AddCounts(suite, run, summary);
        pugi::xml_node properties = suite.append_child("properties");
        const auto property = [&](const char *name, const std::string &value) {
            pugi::xml_node node = properties.append_child("property");
            AddAttribute(node, "name", name);
            AddAttribute(node, "value", value);
        };
        property("random-seed", std::to_string(run.seed));
        property("workers", std::to_string(run.workers));
        if (run.shardCount > 1)
        {
            property("shard",
                     std::to_string(run.shardIndex) + "/" +
                         std::to_string(run.shardCount));
        }
        property("flaky", std::to_string(summary.flaky));
        property("quarantined", std::to_string(summary.quarantined));
        for (const auto &test : run.tests)
        {
            AddTestCase(suite, test, run.binary);
        }

        document.save(output, "  ", pugi::format_indent, pugi::encoding_utf8);
        return static_cast<bool>(output);
    }

    std::string
    JUnitXmlWriter::ToString(const RunResult &run)
    {
        std::ostringstream output;
        static_cast<void>(Write(run, output));
        return output.str();
    }
} // namespace Progmasoft::Catch3
