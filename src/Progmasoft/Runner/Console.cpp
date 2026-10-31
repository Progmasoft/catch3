// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include <algorithm>
#include <cstdio>
#include <set>

#include "Console.hpp"
#include "Json.hpp"
#include "Process.hpp"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        constexpr const char *kRed = "\x1b[31m";
        constexpr const char *kGreen = "\x1b[32m";
        constexpr const char *kYellow = "\x1b[33m";
        constexpr const char *kCyan = "\x1b[36m";
        constexpr const char *kGrey = "\x1b[90m";
        constexpr const char *kBold = "\x1b[1m";

        std::string
        Duration(std::int64_t microseconds)
        {
            if (microseconds < 1000)
            {
                return std::to_string(microseconds) + " us";
            }
            if (microseconds < 1000000)
            {
                return std::to_string(microseconds / 1000) + " ms";
            }
            const auto tenths = (microseconds + 50000) / 100000;
            return std::to_string(tenths / 10) + "."
                   + std::to_string(tenths % 10) + " s";
        }

        const char *
        Label(const TestResult &test)
        {
            if (test.flaky)
            {
                return "FLAKY";
            }
            switch (test.outcome)
            {
                case Outcome::kPassed:
                    return "PASS";
                case Outcome::kFailed:
                    return "FAIL";
                case Outcome::kFailedAsExpected:
                    return "XFAIL";
                case Outcome::kSkipped:
                    return "SKIP";
                case Outcome::kCrashed:
                    return "CRASH";
                case Outcome::kTimedOut:
                    return "TIMEOUT";
                case Outcome::kBlocked:
                    return "BLOCKED";
                case Outcome::kNotRun:
                    return "NOT RUN";
                case Outcome::kUpToDate:
                    return "UP TO DATE";
            }
            return "?";
        }

        void
        Write(std::FILE *output, const std::string &text)
        {
            std::fwrite(text.data(), 1, text.size(), output);
        }

        std::string
        Indent(const std::string &text, const std::string &prefix)
        {
            std::string result;
            std::size_t start = 0;
            while (start < text.size())
            {
                auto end = text.find('\n', start);
                if (end == std::string::npos)
                {
                    end = text.size();
                }
                result += prefix + text.substr(start, end - start) + "\n";
                start = end + 1;
            }
            return result;
        }

        /// One assertion on one line, then its messages: what --success
        /// shows for every assertion of a test.
        std::string
        DescribeAssertion(const AssertionRecord &assertion)
        {
            std::string text = assertion.file + ":"
                               + std::to_string(assertion.line) + ": "
                               + (assertion.passed ? "passed" : "FAILED") + ":";
            if (!assertion.expression.empty())
            {
                text += " "
                        + (assertion.macro.empty()
                               ? assertion.expression
                               : assertion.macro + "( " + assertion.expression
                                     + " )");
                if (!assertion.expansion.empty()
                    && assertion.expansion != assertion.expression)
                {
                    text += " for: " + assertion.expansion;
                }
            }
            text += "\n";
            if (!assertion.message.empty())
            {
                text += "  with message: " + assertion.message + "\n";
            }
            for (const auto &context : assertion.context)
            {
                text += "  with context: " + context + "\n";
            }
            return text;
        }

        /// The last lines of a long output.
        std::string
        Tail(const std::string &text, std::size_t lines)
        {
            std::size_t position = text.size();
            std::size_t count = 0;
            while (position > 0 && count <= lines)
            {
                position = text.rfind('\n', position - 1);
                if (position == std::string::npos)
                {
                    return text;
                }
                ++count;
            }
            return count > lines ? text.substr(position + 1) : text;
        }
    } // namespace

    ConsoleReport::ConsoleReport(const Options &options, std::FILE *output)
        : options_{ options }
        , output_{ output }
    {
        terminal_ = output == stdout && IsStandardOutputTerminal();
        colour_ = options.color == ColorMode::kAlways
                  || (options.color == ColorMode::kAuto && terminal_
                      && EnableTerminalColours());
    }

    std::string
    ConsoleReport::Paint(std::string_view text, const char *colour) const
    {
        if (!colour_)
        {
            return std::string{ text };
        }
        return std::string{ colour } + std::string{ text } + "\x1b[0m";
    }

    void
    ConsoleReport::ClearProgress()
    {
        if (progressShown_)
        {
            Write(output_, "\r\x1b[2K");
            progressShown_ = false;
        }
    }

    void
    ConsoleReport::RunStarting(std::size_t tests,
                               std::size_t workers,
                               std::uint32_t seed,
                               const std::vector<std::string> &notes)
    {
        total_ = tests;
        if (options_.verbosity == Verbosity::kQuiet)
        {
            return;
        }
        std::string text = Paint("Catch3", kBold) + ": " + std::to_string(tests)
                           + " test(s)";
        if (options_.inProcess)
        {
            text += " in this process";
        }
        else
        {
            text += ", up to " + std::to_string(workers) + " worker(s)";
        }
        text += ", seed " + std::to_string(seed) + "\n";
        for (const auto &note : notes)
        {
            text += Paint("note: ", kCyan) + note + "\n";
        }
        Write(output_, text);
        std::fflush(output_);
    }

    void
    ConsoleReport::Retrying(const TestResult &test, const Attempt &attempt)
    {
        if (options_.verbosity != Verbosity::kVerbose)
        {
            return;
        }
        ClearProgress();
        Write(output_,
              Paint("RETRY ", kYellow) + test.name + " after "
                  + OutcomeName(attempt.outcome) + "\n");
    }

    void
    ConsoleReport::TestFinished(const TestResult &test)
    {
        ++finished_;
        const bool failed = test.FailsRun();
        failed_ += failed ? 1 : 0;
        const bool show = options_.verbosity == Verbosity::kVerbose
                          || options_.includeSuccessful || failed || test.flaky
                          || (test.quarantined && IsFailure(test.outcome));
        if (show)
        {
            ClearProgress();
            const char *colour = failed       ? kRed
                                 : test.flaky ? kYellow
                                 : test.outcome == Outcome::kSkipped
                                         || test.outcome == Outcome::kBlocked
                                     ? kGrey
                                     : kGreen;
            std::string line
                = Paint(Label(test), colour) + " " + test.name + " "
                  + Paint("(" + Duration(test.DurationMicroseconds()) + ")",
                          kGrey);
            if (test.quarantined && IsFailure(test.outcome))
            {
                line += Paint(" [quarantined]", kYellow);
            }
            Write(output_, line + "\n");
            if (options_.includeSuccessful && !test.attempts.empty())
            {
                for (const auto &assertion : test.attempts.back().assertions)
                {
                    Write(output_,
                          Indent(DescribeAssertion(assertion), "    "));
                }
            }
            if (options_.verbosity == Verbosity::kVerbose && !failed
                && !test.attempts.empty())
            {
                // What a passing test printed; a failure shows it later.
                const Attempt &attempt = test.attempts.back();
                const std::string output = attempt.standardOutput
                                           + attempt.standardError
                                           + attempt.processOutput;
                if (!output.empty())
                {
                    Write(output_, Indent(output, "    "));
                }
            }
        }
        if (terminal_ && options_.verbosity != Verbosity::kQuiet
            && finished_ < total_)
        {
            ClearProgress();
            std::string progress = "[" + std::to_string(finished_) + "/"
                                   + std::to_string(total_) + "]";
            if (failed_ > 0)
            {
                progress
                    += " " + Paint(std::to_string(failed_) + " failed", kRed);
            }
            Write(output_, progress);
            progressShown_ = true;
        }
        std::fflush(output_);
    }

    void
    ConsoleReport::PrintFailure(const TestResult &test)
    {
        std::string text
            = "\n" + Paint(std::string{ Label(test) } + ": " + test.name, kBold)
              + "\n" + Paint(test.file + ":" + std::to_string(test.line), kGrey)
              + "\n";
        const Attempt *failure = nullptr;
        for (const auto &attempt : test.attempts)
        {
            if (IsFailure(attempt.outcome))
            {
                failure = &attempt;
                break;
            }
        }
        if (failure != nullptr)
        {
            text += Indent(DescribeFailure(*failure), "  ");
            if (!failure->standardOutput.empty())
            {
                text += "  standard output:\n"
                        + Indent(Tail(failure->standardOutput, 20), "    ");
            }
            const std::string errors
                = failure->standardError + failure->processOutput;
            if (!errors.empty())
            {
                text
                    += "  standard error:\n" + Indent(Tail(errors, 20), "    ");
            }
        }
        if (test.outcome == Outcome::kBlocked && !test.analysis.empty())
        {
            text += "  " + test.analysis.front() + "\n";
        }
        else
        {
            for (const auto &finding : test.analysis)
            {
                text += Paint("  analysis: ", kCyan) + finding + "\n";
            }
        }
        if (!test.replayCommand.empty())
        {
            text += Paint("  replay: ", kCyan) + test.replayCommand + "\n";
        }
        Write(output_, text);
    }

    void
    ConsoleReport::RunEnded(const RunResult &run)
    {
        ClearProgress();
        for (const auto &test : run.tests)
        {
            if (test.FailsRun() || test.flaky
                || (test.quarantined && IsFailure(test.outcome)))
            {
                PrintFailure(test);
            }
        }
        for (const auto &spec : run.unmatchedSpecs)
        {
            Write(output_,
                  Paint("warning: ", kYellow) + "no test matches '" + spec
                      + "'\n");
        }
        for (const auto &note : run.notes)
        {
            Write(output_, Paint("note: ", kCyan) + note + "\n");
        }
        if (options_.slowest > 0)
        {
            std::vector<const TestResult *> ordered;
            for (const auto &test : run.tests)
            {
                if (!test.attempts.empty())
                {
                    ordered.push_back(&test);
                }
            }
            std::sort(ordered.begin(),
                      ordered.end(),
                      [](const TestResult *left, const TestResult *right) {
                          return left->DurationMicroseconds()
                                 > right->DurationMicroseconds();
                      });
            ordered.resize(
                std::min<std::size_t>(ordered.size(), options_.slowest));
            Write(output_, "\nslowest tests:\n");
            for (const auto *test : ordered)
            {
                std::string duration = Duration(test->DurationMicroseconds());
                duration.insert(0,
                                duration.size() < 9 ? 9 - duration.size() : 0,
                                ' ');
                Write(output_, "  " + duration + "  " + test->name + "\n");
            }
        }

        const RunSummary summary = Summarize(run);
        std::string line = "\n" + std::to_string(summary.tests) + " test(s): ";
        std::vector<std::string> parts;
        const auto add
            = [&](std::size_t count, const char *what, const char *colour) {
                  if (count > 0)
                  {
                      parts.push_back(
                          Paint(std::to_string(count) + " " + what, colour));
                  }
              };
        add(summary.passed, "passed", kGreen);
        add(summary.failed, "failed", kRed);
        add(summary.crashed, "crashed", kRed);
        add(summary.timedOut, "timed out", kRed);
        add(summary.failedAsExpected, "failed as expected", kGreen);
        add(summary.skipped, "skipped", kGrey);
        add(summary.blocked, "blocked", kRed);
        add(summary.notRun, "not run", kGrey);
        add(summary.upToDate, "up to date", kGrey);
        add(summary.flaky, "flaky", kYellow);
        add(summary.quarantined, "in quarantine", kYellow);
        if (parts.empty())
        {
            parts.emplace_back("none ran");
        }
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            line += (index > 0 ? ", " : "") + parts[index];
        }
        line += "\n"
                + std::to_string(summary.assertionsPassed
                                 + summary.assertionsFailed)
                + " assertion(s), " + std::to_string(summary.attempts)
                + " attempt(s); " + Duration(summary.wallMicroseconds);
        if (summary.wallMicroseconds > 0 && summary.testMicroseconds > 0
            && !options_.inProcess)
        {
            const auto speedup
                = static_cast<double>(summary.testMicroseconds)
                  / static_cast<double>(summary.wallMicroseconds);
            line += " (" + Duration(summary.testMicroseconds) + " of tests";
            // Below this, worker start-up outweighs the tests and the ratio
            // says nothing about parallelism.
            if (speedup >= 1.05)
            {
                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "%.1f", speedup);
                line += std::string{ ", " } + buffer + "x in parallel";
            }
            line += ")";
        }
        Write(output_, line + "\n");
        std::fflush(output_);
    }

    std::string
    TraceJson(const RunResult &run)
    {
        JsonValue events = JsonValue::Array();
        std::set<std::size_t> workers;
        for (const auto &test : run.tests)
        {
            for (const auto &attempt : test.attempts)
            {
                workers.insert(attempt.worker);
                JsonValue slice = JsonValue::Object();
                slice.Set("name", test.name);
                slice.Set("cat", OutcomeName(attempt.outcome));
                slice.Set("ph", "X");
                slice.Set("ts", attempt.startMicroseconds);
                slice.Set(
                    "dur",
                    std::max<std::int64_t>(1, attempt.durationMicroseconds));
                slice.Set("pid", 1);
                slice.Set("tid", attempt.worker);
                JsonValue arguments = JsonValue::Object();
                arguments.Set("attempt", attempt.number);
                arguments.Set("outcome", OutcomeName(attempt.outcome));
                arguments.Set("file",
                              test.file + ":" + std::to_string(test.line));
                if (!attempt.precededBy.empty())
                {
                    arguments.Set("precededBy", attempt.precededBy.size());
                }
                slice.Set("args", std::move(arguments));
                events.Append(std::move(slice));
                for (const auto &section : attempt.sections)
                {
                    JsonValue inner = JsonValue::Object();
                    inner.Set("name", section.name);
                    inner.Set("cat", "section");
                    inner.Set("ph", "X");
                    inner.Set("ts",
                              attempt.startMicroseconds
                                  + section.startMicroseconds);
                    inner.Set("dur",
                              section.durationMicroseconds < 0
                                  ? std::max<std::int64_t>(
                                        1,
                                        attempt.durationMicroseconds
                                            - section.startMicroseconds)
                                  : std::max<std::int64_t>(
                                        1,
                                        section.durationMicroseconds));
                    inner.Set("pid", 1);
                    inner.Set("tid", attempt.worker);
                    events.Append(std::move(inner));
                }
            }
        }
        for (const std::size_t worker : workers)
        {
            JsonValue name = JsonValue::Object();
            name.Set("name", "thread_name");
            name.Set("ph", "M");
            name.Set("pid", 1);
            name.Set("tid", worker);
            JsonValue arguments = JsonValue::Object();
            arguments.Set("name",
                          worker == 0 ? std::string{ "in process" }
                                      : "worker " + std::to_string(worker));
            name.Set("args", std::move(arguments));
            events.Append(std::move(name));
        }
        JsonValue trace = JsonValue::Object();
        trace.Set("traceEvents", std::move(events));
        trace.Set("displayTimeUnit", "ms");
        JsonValue metadata = JsonValue::Object();
        metadata.Set("binary", run.binary);
        metadata.Set("seed", run.seed);
        trace.Set("otherData", std::move(metadata));
        return trace.ToString();
    }
} // namespace Progmasoft::Catch3::Runner
