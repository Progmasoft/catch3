// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <Progmasoft/Catch3/Results.hpp>

#include <array>
#include <string_view>
#include <utility>

namespace Progmasoft::Catch3
{
    namespace
    {
        constexpr std::array<std::pair<Outcome, std::string_view>, 9>
            kOutcomeNames = {{
                {Outcome::kPassed, "passed"},
                {Outcome::kFailed, "failed"},
                {Outcome::kFailedAsExpected, "failed-as-expected"},
                {Outcome::kSkipped, "skipped"},
                {Outcome::kCrashed, "crashed"},
                {Outcome::kTimedOut, "timed-out"},
                {Outcome::kBlocked, "blocked"},
                {Outcome::kNotRun, "not-run"},
                {Outcome::kUpToDate, "up-to-date"},
            }};

        constexpr std::array<std::pair<FailureCategory, std::string_view>, 9>
            kCategoryNames = {{
                {FailureCategory::kNone, "none"},
                {FailureCategory::kAssertion, "assertion"},
                {FailureCategory::kUnexpectedException, "unexpected-exception"},
                {FailureCategory::kMissingException, "missing-exception"},
                {FailureCategory::kExplicitFailure, "explicit-failure"},
                {FailureCategory::kCrash, "crash"},
                {FailureCategory::kTimeout, "timeout"},
                {FailureCategory::kInfrastructure, "infrastructure"},
                {FailureCategory::kDependency, "dependency"},
            }};
    } // namespace

    std::int64_t
    TestResult::DurationMicroseconds() const noexcept
    {
        std::int64_t total = 0;
        for (const auto &attempt : attempts)
        {
            total += attempt.durationMicroseconds;
        }
        return total;
    }

    bool
    TestResult::FailsRun() const noexcept
    {
        if (quarantined)
        {
            return false;
        }
        switch (outcome)
        {
            case Outcome::kFailed:
            case Outcome::kCrashed:
            case Outcome::kTimedOut:
            case Outcome::kBlocked:
                return true;
            default:
                return false;
        }
    }

    RunSummary
    Summarize(const RunResult &run)
    {
        RunSummary summary;
        summary.tests = run.tests.size();
        summary.wallMicroseconds = run.wallMicroseconds;
        for (const auto &test : run.tests)
        {
            switch (test.outcome)
            {
                case Outcome::kPassed:
                    ++summary.passed;
                    break;
                case Outcome::kFailed:
                    ++summary.failed;
                    break;
                case Outcome::kFailedAsExpected:
                    ++summary.failedAsExpected;
                    break;
                case Outcome::kSkipped:
                    ++summary.skipped;
                    break;
                case Outcome::kCrashed:
                    ++summary.crashed;
                    break;
                case Outcome::kTimedOut:
                    ++summary.timedOut;
                    break;
                case Outcome::kBlocked:
                    ++summary.blocked;
                    break;
                case Outcome::kNotRun:
                    ++summary.notRun;
                    break;
                case Outcome::kUpToDate:
                    ++summary.upToDate;
                    break;
            }
            summary.flaky += test.flaky ? 1 : 0;
            summary.quarantined += test.quarantined ? 1 : 0;
            summary.attempts += test.attempts.size();
            for (const auto &attempt : test.attempts)
            {
                summary.assertionsPassed += attempt.assertionsPassed;
                summary.assertionsFailed += attempt.assertionsFailed;
                summary.testMicroseconds += attempt.durationMicroseconds;
            }
        }
        return summary;
    }

    const char *
    OutcomeName(Outcome outcome) noexcept
    {
        for (const auto &[value, name] : kOutcomeNames)
        {
            if (value == outcome)
            {
                return name.data();
            }
        }
        return "unknown";
    }

    std::optional<Outcome>
    OutcomeFromName(const std::string &name) noexcept
    {
        for (const auto &[value, text] : kOutcomeNames)
        {
            if (text == name)
            {
                return value;
            }
        }
        return std::nullopt;
    }

    const char *
    CategoryName(FailureCategory category) noexcept
    {
        for (const auto &[value, name] : kCategoryNames)
        {
            if (value == category)
            {
                return name.data();
            }
        }
        return "unknown";
    }

    std::optional<FailureCategory>
    CategoryFromName(const std::string &name) noexcept
    {
        for (const auto &[value, text] : kCategoryNames)
        {
            if (text == name)
            {
                return value;
            }
        }
        return std::nullopt;
    }

    const char *
    HistoryName(FailureHistory history) noexcept
    {
        switch (history)
        {
            case FailureHistory::kUnknown:
                return "unknown";
            case FailureHistory::kNew:
                return "new";
            case FailureHistory::kPersistent:
                return "persistent";
            case FailureHistory::kIntermittent:
                return "intermittent";
        }
        return "unknown";
    }

    namespace
    {
        void
        AppendIndented(std::string &text, const std::string &block)
        {
            std::size_t start = 0;
            while (start <= block.size())
            {
                const auto end = block.find('\n', start);
                text += "  ";
                text += block.substr(start,
                                     end == std::string::npos ? std::string::npos
                                                              : end - start);
                text += '\n';
                if (end == std::string::npos)
                {
                    break;
                }
                start = end + 1;
            }
        }

        std::string
        JoinSections(const std::vector<std::string> &sections)
        {
            std::string text;
            for (const auto &section : sections)
            {
                text += text.empty() ? "" : " / ";
                text += section;
            }
            return text;
        }

        std::string
        Seconds(std::int64_t microseconds)
        {
            const auto tenths = (microseconds + 50000) / 100000;
            return std::to_string(tenths / 10) + "." +
                   std::to_string(tenths % 10) + " s";
        }
    } // namespace

    std::string
    DescribeFailure(const Attempt &attempt)
    {
        std::string text;
        if (attempt.outcome == Outcome::kTimedOut)
        {
            text += "timed out after " + Seconds(attempt.durationMicroseconds);
            text += "\n";
        }
        else if (attempt.outcome == Outcome::kCrashed)
        {
            text += "crashed";
            if (!attempt.exitDescription.empty())
            {
                text += " with " + attempt.exitDescription;
            }
            text += "\n";
            if (!attempt.fatalMessage.empty())
            {
                AppendIndented(text, attempt.fatalMessage);
            }
        }
        if ((attempt.outcome == Outcome::kTimedOut ||
             attempt.outcome == Outcome::kCrashed) &&
            !attempt.lastLocation.empty())
        {
            text += "last assertion started at " + attempt.lastLocation + "\n";
        }
        if ((attempt.outcome == Outcome::kTimedOut ||
             attempt.outcome == Outcome::kCrashed))
        {
            // The innermost section that had not ended is where it stopped.
            std::vector<std::string> open;
            for (const auto &section : attempt.sections)
            {
                if (section.durationMicroseconds < 0)
                {
                    open.resize(section.depth);
                    open.push_back(section.name);
                }
            }
            if (!open.empty())
            {
                text += "in section " + JoinSections(open) + "\n";
            }
        }
        for (const auto &assertion : attempt.assertions)
        {
            if (assertion.passed)
            {
                continue;
            }
            text += assertion.file + ":" + std::to_string(assertion.line) +
                    ": " + (assertion.allowed ? "FAILED (allowed)" : "FAILED") +
                    ":\n";
            if (!assertion.expression.empty())
            {
                AppendIndented(text,
                               assertion.macro.empty()
                                   ? assertion.expression
                                   : assertion.macro + "( " +
                                         assertion.expression + " )");
            }
            if (!assertion.expansion.empty() &&
                assertion.expansion != assertion.expression)
            {
                text += "with expansion:\n";
                AppendIndented(text, assertion.expansion);
            }
            if (!assertion.message.empty())
            {
                text += assertion.category ==
                                FailureCategory::kUnexpectedException
                            ? "due to unexpected exception with message:\n"
                            : "with message:\n";
                AppendIndented(text, assertion.message);
            }
            for (const auto &context : assertion.context)
            {
                text += "with context:\n";
                AppendIndented(text, context);
            }
            if (!assertion.sections.empty())
            {
                text += "in section " + JoinSections(assertion.sections) + "\n";
            }
        }
        return text;
    }

    std::string
    SummarizeFailure(const Attempt &attempt)
    {
        const std::string text = DescribeFailure(attempt);
        return text.substr(0, text.find('\n'));
    }

    bool
    IsFailure(Outcome outcome) noexcept
    {
        return outcome == Outcome::kFailed || outcome == Outcome::kCrashed ||
               outcome == Outcome::kTimedOut;
    }
} // namespace Progmasoft::Catch3
