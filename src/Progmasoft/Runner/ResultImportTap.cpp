// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// The Test Anything Protocol, versions 12 to 14.
#include <Progmasoft/Runner/ResultImport.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <utility>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        namespace ImportTap
        {
            std::string
            Lowered(std::string_view text)
            {
                std::string lowered{ text };
                std::transform(lowered.begin(),
                               lowered.end(),
                               lowered.begin(),
                               [](unsigned char character) {
                                   return static_cast<char>(
                                       std::tolower(character));
                               });
                return lowered;
            }

            struct Line final
            {
                bool ok = false;
                std::string description;
                std::string directive;
                std::string reason;
            };

            /// Where the `#` that starts a directive is: `\#` is part of the
            /// description.
            std::size_t
            DirectiveStart(std::string_view text)
            {
                for (std::size_t index = 0; index < text.size(); ++index)
                {
                    if (text[index] == '\\')
                    {
                        ++index;
                    }
                    else if (text[index] == '#')
                    {
                        return index;
                    }
                }
                return std::string_view::npos;
            }

            std::string
            Unescaped(std::string_view text)
            {
                std::string result;
                for (std::size_t index = 0; index < text.size(); ++index)
                {
                    if (text[index] == '\\' && index + 1 < text.size())
                    {
                        ++index;
                    }
                    result.push_back(text[index]);
                }
                return result;
            }

            /// Reads a test line; nothing when `text` is not one.
            std::optional<Line>
            ReadLine(std::string_view text)
            {
                Line line;
                if (ImportStartsWith(text, "not ok"))
                {
                    text.remove_prefix(6);
                }
                else if (ImportStartsWith(text, "ok"))
                {
                    line.ok = true;
                    text.remove_prefix(2);
                }
                else
                {
                    return std::nullopt;
                }
                if (!text.empty() && text.front() != ' '
                    && text.front() != '\t')
                {
                    return std::nullopt;
                }
                text = ImportTrim(text);
                while (!text.empty()
                       && std::isdigit(static_cast<unsigned char>(text.front()))
                              != 0)
                {
                    text.remove_prefix(1);
                }
                text = ImportTrim(text);
                if (ImportStartsWith(text, "- "))
                {
                    text.remove_prefix(2);
                }
                const auto directive = DirectiveStart(text);
                line.description
                    = Unescaped(ImportTrim(text.substr(0, directive)));
                if (directive != std::string_view::npos)
                {
                    const std::string_view rest
                        = ImportTrim(text.substr(directive + 1));
                    const auto end = rest.find_first_of(" \t:");
                    const std::string word = Lowered(rest.substr(0, end));
                    // Harnesses take the longer spellings of SKIP, and only
                    // the word TODO.
                    if (word == "skip" || word == "skipped"
                        || word == "skipping" || word == "todo")
                    {
                        line.directive = word.substr(0, 4);
                        if (end != std::string_view::npos)
                        {
                            std::string_view reason = rest.substr(end);
                            reason.remove_prefix(reason.front() == ':' ? 1 : 0);
                            line.reason = std::string{ ImportTrim(reason) };
                        }
                    }
                }
                return line;
            }

            Outcome
            OutcomeOf(const Line &line)
            {
                if (line.directive == "skip")
                {
                    return Outcome::kSkipped;
                }
                if (line.ok)
                {
                    return Outcome::kPassed;
                }
                return line.directive == "todo" ? Outcome::kFailedAsExpected
                                                : Outcome::kFailed;
            }

            std::string
            Unquoted(std::string_view text)
            {
                text = ImportTrim(text);
                if (text.size() >= 2 && text.front() == text.back()
                    && (text.front() == '"' || text.front() == '\''))
                {
                    text.remove_prefix(1);
                    text.remove_suffix(1);
                }
                return std::string{ text };
            }

            /// A YAML block after a test: its text goes into the failure, and
            /// the two keys that tools agree on are read.
            void
            ApplyDiagnostics(TestResult &test, const std::string &block)
            {
                Attempt &attempt = test.attempts.back();
                for (const auto raw : ImportLines(block))
                {
                    const std::string_view line = ImportTrim(raw);
                    if (ImportStartsWith(line, "duration_ms:"))
                    {
                        const std::string value{ line.substr(12) };
                        attempt.durationMicroseconds = ImportedMicroseconds(
                            std::strtod(value.c_str(), nullptr) / 1e3);
                    }
                    else if (ImportStartsWith(line, "message:")
                             && !attempt.assertions.empty())
                    {
                        attempt.assertions.front().message
                            = Unquoted(line.substr(8));
                    }
                }
                if (!attempt.assertions.empty())
                {
                    attempt.assertions.front().expansion += block;
                }
            }

            Expected<std::vector<TestResult>>
            Parse(std::string_view text, const std::string &source)
            {
                std::vector<TestResult> tests;
                std::vector<std::string> subtests;
                std::map<std::string, std::size_t> seen;
                std::optional<std::size_t> planned;
                std::size_t topLevel = 0;
                std::string bailOut;
                bool anything = false;
                std::optional<std::string> block;
                for (const auto raw : ImportLines(text))
                {
                    const std::string_view line = ImportTrim(raw);
                    if (block)
                    {
                        if (line == "...")
                        {
                            if (!tests.empty())
                            {
                                ApplyDiagnostics(tests.back(), *block);
                            }
                            block.reset();
                        }
                        else
                        {
                            block->append(raw).push_back('\n');
                        }
                        continue;
                    }
                    if (line == "---")
                    {
                        block.emplace();
                        continue;
                    }
                    std::size_t indent = 0;
                    for (const char character : raw)
                    {
                        if (character != ' ' && character != '\t')
                        {
                            break;
                        }
                        indent += character == '\t' ? 4 : 1;
                    }
                    const std::size_t depth = indent / 4;
                    if (ImportStartsWith(line, "# Subtest:"))
                    {
                        subtests.resize(depth);
                        subtests.emplace_back(ImportTrim(line.substr(10)));
                        continue;
                    }
                    if (ImportStartsWith(line, "Bail out!"))
                    {
                        anything = true;
                        bailOut = std::string{ line };
                        break;
                    }
                    if (ImportStartsWith(line, "1..") && depth == 0)
                    {
                        anything = true;
                        const std::string count{ line.substr(3) };
                        planned = static_cast<std::size_t>(
                            std::strtoull(count.c_str(), nullptr, 10));
                        continue;
                    }
                    if (line.size() > 1 && line.front() == '#' && !tests.empty()
                        && !tests.back().attempts.back().assertions.empty())
                    {
                        // Comments after a failure usually explain it.
                        auto &failure
                            = tests.back().attempts.back().assertions.front();
                        failure.expansion
                            += std::string{ ImportTrim(line.substr(1)) } + "\n";
                        continue;
                    }
                    const auto parsed = ReadLine(line);
                    if (!parsed)
                    {
                        continue;
                    }
                    anything = true;
                    topLevel += depth == 0 ? 1 : 0;
                    std::string parent = source;
                    for (std::size_t level = 0;
                         level < std::min(depth, subtests.size());
                         ++level)
                    {
                        parent
                            += (parent.empty() ? "" : " > ") + subtests[level];
                    }
                    std::string name = parsed->description;
                    if (name.empty())
                    {
                        name = "test "
                               + std::to_string(++seen["unnamed\n" + parent]);
                    }
                    // TAP has no retries: a name that comes back in one
                    // stream is another test.
                    const std::size_t repeat = ++seen[parent + "\n" + name];
                    if (repeat > 1)
                    {
                        name += " #" + std::to_string(repeat);
                    }
                    TestResult test
                        = ImportedTest(std::move(name), std::move(parent));
                    test.attempts.push_back(
                        ImportedAttempt(OutcomeOf(*parsed), 0, parsed->reason));
                    tests.push_back(std::move(test));
                }
                if (!anything)
                {
                    return Error{ "there are no TAP results in it" };
                }
                // A stream that ends early must not read as a run that passed.
                std::string incomplete;
                if (!bailOut.empty())
                {
                    incomplete = bailOut;
                }
                else if (planned && *planned != topLevel)
                {
                    incomplete = "planned " + std::to_string(*planned)
                                 + " test(s), reported "
                                 + std::to_string(topLevel);
                }
                if (!incomplete.empty())
                {
                    TestResult test = ImportedTest("the TAP stream", source);
                    test.attempts.push_back(
                        ImportedAttempt(Outcome::kFailed,
                                        0,
                                        std::move(incomplete),
                                        {},
                                        FailureCategory::kInfrastructure));
                    tests.push_back(std::move(test));
                }
                return tests;
            }
        } // namespace ImportTap
    } // namespace

    bool
    IsTapTestLine(std::string_view line)
    {
        return ImportTap::ReadLine(line).has_value();
    }

    Expected<std::vector<TestResult>>
    ParseTap(std::string_view text, const std::string &source)
    {
        return ImportTap::Parse(text, source);
    }
} // namespace Progmasoft::Catch3::Runner
