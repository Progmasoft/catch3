// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include "Analysis.hpp"

#include "Hash.hpp"

#include <algorithm>
#include <cctype>
#include <map>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        /// Replaces what differs between two runs of the same failure:
        /// numbers, hexadecimal addresses and the temporary directory.
        std::string
        Normalise(std::string_view text)
        {
            std::string result;
            bool inNumber = false;
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                const char character = text[index];
                const bool digit =
                    std::isdigit(static_cast<unsigned char>(character)) != 0;
                const bool hexLetter =
                    inNumber &&
                    std::isxdigit(static_cast<unsigned char>(character)) != 0;
                if (digit || hexLetter ||
                    (inNumber && (character == 'x' || character == '.')))
                {
                    if (!inNumber)
                    {
                        result.push_back('#');
                    }
                    inNumber = true;
                    continue;
                }
                inNumber = false;
                result.push_back(character);
            }
            return result;
        }

        const Attempt *
        FirstFailure(const TestResult &test)
        {
            for (const auto &attempt : test.attempts)
            {
                if (IsFailure(attempt.outcome))
                {
                    return &attempt;
                }
            }
            return nullptr;
        }

        std::string
        Seconds(std::int64_t microseconds)
        {
            const auto tenths = (microseconds + 50000) / 100000;
            return std::to_string(tenths / 10) + "." +
                   std::to_string(tenths % 10) + " s";
        }

        std::string
        OpenSections(const Attempt &attempt)
        {
            std::vector<std::string> open;
            for (const auto &section : attempt.sections)
            {
                if (section.durationMicroseconds < 0)
                {
                    open.resize(section.depth);
                    open.push_back(section.name);
                }
            }
            std::string text;
            for (const auto &name : open)
            {
                text += text.empty() ? "" : " / ";
                text += name;
            }
            return text;
        }

        std::string
        QuoteArgument(const std::string &argument)
        {
            if (argument.find_first_of(" \t\"'\\$`&|;<>()[]*?~!#") ==
                    std::string::npos &&
                !argument.empty())
            {
                return argument;
            }
            std::string quoted = "\"";
            for (const char character : argument)
            {
                if (character == '"' || character == '\\' || character == '$' ||
                    character == '`')
                {
                    quoted.push_back('\\');
                }
                quoted.push_back(character);
            }
            quoted.push_back('"');
            return quoted;
        }
    } // namespace

    std::string
    QuoteTestSpec(const std::string &name)
    {
        // Catch2's spec syntax gives these characters a meaning; a backslash
        // makes them literal.
        std::string spec;
        for (const char character : name)
        {
            if (character == ',' || character == '[' || character == ']' ||
                character == '*' || character == '\\' || character == '~' ||
                character == '"')
            {
                spec.push_back('\\');
            }
            spec.push_back(character);
        }
        return spec;
    }

    std::string
    FailureSignature(const Attempt &attempt)
    {
        std::string text = CategoryName(attempt.category);
        text += '|';
        bool described = false;
        for (const auto &assertion : attempt.assertions)
        {
            if (assertion.passed || assertion.allowed)
            {
                continue;
            }
            text += assertion.file + ":" + std::to_string(assertion.line) +
                    "|" + Normalise(assertion.message) + "|" +
                    assertion.expression;
            described = true;
            break;
        }
        if (!described)
        {
            text += Normalise(attempt.fatalMessage) + "|" +
                    attempt.lastLocation + "|" + OpenSections(attempt);
        }
        return "sig-" + ToHex(Fnv1a64(text)).substr(0, 12);
    }

    void
    Analyze(RunResult &run, const AnalysisInputs &inputs)
    {
        std::map<std::string, std::vector<std::size_t>> bySignature;
        for (std::size_t index = 0; index < run.tests.size(); ++index)
        {
            TestResult &test = run.tests[index];
            const Attempt *failure = FirstFailure(test);
            if (failure == nullptr)
            {
                continue;
            }
            test.signature = FailureSignature(*failure);
            bySignature[test.signature].push_back(index);
        }

        for (std::size_t index = 0; index < run.tests.size(); ++index)
        {
            TestResult &test = run.tests[index];
            const Attempt *failure = FirstFailure(test);
            if (failure == nullptr)
            {
                continue;
            }
            std::vector<std::string> findings;

            // How it relates to earlier runs.
            const TestHistory *past =
                inputs.history == nullptr ? nullptr
                                          : inputs.history->Find(test.id);
            const TestRecord *last = past == nullptr ? nullptr : past->Last();
            if (last == nullptr)
            {
                test.history = FailureHistory::kUnknown;
            }
            else if (test.flaky || past->FlakyEvents(20) > 0)
            {
                test.history = FailureHistory::kIntermittent;
                const auto events = past->FlakyEvents(20) + (test.flaky ? 1 : 0);
                findings.push_back("intermittent: " + std::to_string(events) +
                                   " flaky result(s) in the last 20 runs");
            }
            else if (IsFailure(last->outcome))
            {
                test.history = FailureHistory::kPersistent;
                std::size_t streak = 0;
                for (auto record = past->records.rbegin();
                     record != past->records.rend() &&
                     IsFailure(record->outcome);
                     ++record)
                {
                    ++streak;
                }
                std::string finding = "persistent: failed in the last " +
                                      std::to_string(streak) + " run(s)";
                if (!last->signature.empty() && last->signature != test.signature)
                {
                    finding += ", but differently last time";
                }
                findings.push_back(finding);
            }
            else
            {
                test.history = FailureHistory::kNew;
                std::string finding = "new failure after " +
                                      std::to_string(past->PassStreak()) +
                                      " passing run(s)";
                if (inputs.history != nullptr)
                {
                    for (const auto &runRecord : inputs.history->Runs())
                    {
                        if (runRecord.id == last->run && !runRecord.commit.empty())
                        {
                            finding += "; it last passed at commit " +
                                       runRecord.commit.substr(0, 12);
                        }
                    }
                }
                findings.push_back(finding);
            }

            // Other tests that fail the same way.
            const auto &group = bySignature[test.signature];
            if (group.size() > 1)
            {
                std::string others;
                std::size_t named = 0;
                for (const std::size_t other : group)
                {
                    if (other == index)
                    {
                        continue;
                    }
                    if (named < 3)
                    {
                        others += (named > 0 ? ", '" : "'") +
                                  run.tests[other].name + "'";
                    }
                    ++named;
                }
                if (named > 3)
                {
                    others += " and " + std::to_string(named - 3) + " more";
                }
                findings.push_back("fails the same way as " + others +
                                   ": probably one cause");
            }

            // Where it stopped.
            if (failure->outcome == Outcome::kCrashed)
            {
                std::string finding = "crashed";
                if (!failure->exitDescription.empty())
                {
                    finding += " (" + failure->exitDescription + ")";
                }
                if (!failure->lastLocation.empty())
                {
                    finding += " after the assertion at " + failure->lastLocation;
                }
                const std::string sections = OpenSections(*failure);
                if (!sections.empty())
                {
                    finding += ", in section " + sections;
                }
                findings.push_back(finding);
            }
            else if (failure->outcome == Outcome::kTimedOut)
            {
                std::string finding =
                    "stopped after " + Seconds(failure->durationMicroseconds);
                if (test.predictedDurationMicroseconds > 0 && last != nullptr)
                {
                    finding += "; it usually takes " +
                               Seconds(test.predictedDurationMicroseconds);
                }
                const std::string sections = OpenSections(*failure);
                if (!sections.empty())
                {
                    finding += "; it was in section " + sections;
                }
                if (!failure->lastLocation.empty())
                {
                    finding += "; last assertion started at " +
                               failure->lastLocation;
                }
                findings.push_back(finding);
            }

            // The change it follows.
            if (inputs.changes != nullptr)
            {
                std::vector<std::string> changedFiles;
                const auto changed = [&](const std::string &file) {
                    return std::any_of(inputs.changes->files.begin(),
                                       inputs.changes->files.end(),
                                       [&](const auto &path) {
                                           return SameSourceFile(path, file);
                                       });
                };
                if (changed(test.file))
                {
                    changedFiles.push_back(test.file);
                }
                for (const auto &assertion : failure->assertions)
                {
                    if (!assertion.passed && assertion.file != test.file &&
                        changed(assertion.file) &&
                        std::find(changedFiles.begin(),
                                  changedFiles.end(),
                                  assertion.file) == changedFiles.end())
                    {
                        changedFiles.push_back(assertion.file);
                    }
                }
                for (const auto &file : changedFiles)
                {
                    findings.push_back(
                        "changed since " +
                        (inputs.changesSince.empty() ? std::string{"HEAD"}
                                                     : inputs.changesSince) +
                        ": " + file);
                }
            }

            if (!failure->temporaryDirectory.empty())
            {
                findings.push_back("its temporary files are kept in " +
                                   failure->temporaryDirectory);
            }

            // Findings of the scheduler (order dependence, flakiness,
            // quarantine) come first.
            test.analysis.insert(
                test.analysis.end(), findings.begin(), findings.end());

            std::string replay = QuoteArgument(inputs.program) + " " +
                                 QuoteArgument(QuoteTestSpec(test.name)) +
                                 " --seed=" + std::to_string(failure->seed);
            if (!failure->precededBy.empty() && !test.flaky)
            {
                // The replay mode runs the same tests first, in the same
                // process, as the failure did.
                replay = QuoteArgument(inputs.program) + " --replay";
            }
            test.replayCommand = replay;
        }
    }
} // namespace Progmasoft::Catch3::Runner
