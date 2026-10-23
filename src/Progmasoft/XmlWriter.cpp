// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Results.hpp>
#include <Progmasoft/Catch3/XmlWriter.hpp>

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

        void
        WriteEscaped(std::string_view value, std::ostream &output, bool attribute)
        {
            for (std::size_t index = 0; index < value.size();)
            {
                const DecodedCharacter character = DecodeUtf8(value, index);
                if (character.length == 0 || !IsXmlCharacter(character.value))
                {
                    // A character XML cannot hold is replaced, not dropped,
                    // so the reader sees that something was there.
                    output << "&#xFFFD;";
                    ++index;
                    continue;
                }
                if (character.value < 0x80)
                {
                    switch (static_cast<char>(character.value))
                    {
                        case '&':
                            output << "&amp;";
                            break;
                        case '<':
                            output << "&lt;";
                            break;
                        case '>':
                            output << "&gt;";
                            break;
                        case '"':
                            output << (attribute ? "&quot;" : "\"");
                            break;
                        case '\'':
                            output << (attribute ? "&apos;" : "'");
                            break;
                        case '\n':
                            output << (attribute ? "&#10;" : "\n");
                            break;
                        default:
                            output.put(static_cast<char>(character.value));
                    }
                }
                else
                {
                    output.write(value.data() + index,
                                 static_cast<std::streamsize>(character.length));
                }
                index += character.length;
            }
        }

        void
        WriteAttribute(std::ostream &output,
                       std::string_view name,
                       std::string_view value)
        {
            output << ' ' << name << "=\"";
            WriteEscaped(value, output, true);
            output << '"';
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

        void
        WriteIndent(std::ostream &output, unsigned level)
        {
            for (unsigned index = 0; index < level; ++index)
            {
                output << "  ";
            }
        }

        void
        WriteTextElement(std::ostream &output,
                         unsigned indent,
                         std::string_view name,
                         std::string_view value)
        {
            WriteIndent(output, indent);
            output << '<' << name << '>';
            WriteEscaped(value, output, false);
            output << "</" << name << ">\n";
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
        WriteEarlierAttempt(std::ostream &output,
                            const Attempt &attempt,
                            std::string_view element)
        {
            WriteIndent(output, 3);
            output << '<' << element;
            WriteAttribute(output, "message", SummarizeFailure(attempt));
            WriteAttribute(output, "type", CategoryName(attempt.category));
            output << ">\n";
            WriteTextElement(output, 4, "stackTrace", DescribeFailure(attempt));
            if (!attempt.standardOutput.empty())
            {
                WriteTextElement(
                    output, 4, "system-out", Output(attempt, false));
            }
            if (!Output(attempt, true).empty())
            {
                WriteTextElement(output, 4, "system-err", Output(attempt, true));
            }
            WriteIndent(output, 3);
            output << "</" << element << ">\n";
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
        WriteTestCase(std::ostream &output,
                      const TestResult &test,
                      std::string_view suiteName)
        {
            WriteIndent(output, 2);
            output << "<testcase";
            WriteAttribute(output,
                           "classname",
                           test.className.empty() ? suiteName : test.className);
            WriteAttribute(output, "name", test.name);
            WriteAttribute(output, "time", JUnitSeconds(test.DurationMicroseconds()));
            if (!test.file.empty())
            {
                WriteAttribute(output, "file", test.file);
                WriteAttribute(output, "line", std::to_string(test.line));
            }
            output << ">\n";

            const Attempt *last =
                test.attempts.empty() ? nullptr : &test.attempts.back();
            const bool failedRun = IsFailure(test.outcome);
            if (failedRun && last != nullptr)
            {
                const bool error = test.outcome != Outcome::kFailed ||
                                   last->category ==
                                       FailureCategory::kInfrastructure;
                const std::string_view element = error ? "error" : "failure";
                WriteIndent(output, 3);
                output << '<' << element;
                WriteAttribute(output, "message", SummarizeFailure(*last));
                WriteAttribute(output, "type", CategoryName(test.category));
                output << '>';
                std::string details = DescribeFailure(*last);
                for (const auto &finding : test.analysis)
                {
                    details += "analysis: " + finding + "\n";
                }
                if (!test.replayCommand.empty())
                {
                    details += "replay: " + test.replayCommand + "\n";
                }
                WriteEscaped(details, output, false);
                output << "</" << element << ">\n";
            }
            else if (test.outcome == Outcome::kSkipped ||
                     test.outcome == Outcome::kBlocked ||
                     test.outcome == Outcome::kNotRun ||
                     test.outcome == Outcome::kUpToDate)
            {
                WriteIndent(output, 3);
                output << "<skipped";
                WriteAttribute(output, "message", SkipReason(test));
                output << "/>\n";
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
                WriteEarlierAttempt(output,
                                    attempt,
                                    failedRun ? "rerunFailure" : "flakyFailure");
            }
            if (last != nullptr && !last->standardOutput.empty())
            {
                WriteTextElement(output, 3, "system-out", Output(*last, false));
            }
            if (last != nullptr && !Output(*last, true).empty())
            {
                WriteTextElement(output, 3, "system-err", Output(*last, true));
            }
            WriteIndent(output, 2);
            output << "</testcase>\n";
        }
    } // namespace

    bool
    JUnitXmlWriter::Write(const RunResult &run, std::ostream &output)
    {
        const RunSummary summary = Summarize(run);
        const std::size_t errors = summary.crashed + summary.timedOut;
        const std::size_t failures = summary.failed;
        const std::size_t skipped =
            summary.skipped + summary.blocked + summary.notRun +
            summary.upToDate;
        const std::string time = JUnitSeconds(run.wallMicroseconds);
        output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites";
        WriteAttribute(output, "name", run.binary);
        WriteAttribute(output, "tests", std::to_string(summary.tests));
        WriteAttribute(output, "failures", std::to_string(failures));
        WriteAttribute(output, "errors", std::to_string(errors));
        WriteAttribute(output, "skipped", std::to_string(skipped));
        WriteAttribute(output, "time", time);
        output << ">\n";
        WriteIndent(output, 1);
        output << "<testsuite";
        WriteAttribute(output, "name", run.binary);
        WriteAttribute(output, "tests", std::to_string(summary.tests));
        WriteAttribute(output, "failures", std::to_string(failures));
        WriteAttribute(output, "errors", std::to_string(errors));
        WriteAttribute(output, "skipped", std::to_string(skipped));
        WriteAttribute(output, "time", time);
        output << ">\n";
        WriteIndent(output, 2);
        output << "<properties>\n";
        const auto property = [&](std::string_view name,
                                  const std::string &value) {
            WriteIndent(output, 3);
            output << "<property";
            WriteAttribute(output, "name", name);
            WriteAttribute(output, "value", value);
            output << "/>\n";
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
        WriteIndent(output, 2);
        output << "</properties>\n";
        for (const auto &test : run.tests)
        {
            WriteTestCase(output, test, run.binary);
        }
        WriteIndent(output, 1);
        output << "</testsuite>\n</testsuites>\n";
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
