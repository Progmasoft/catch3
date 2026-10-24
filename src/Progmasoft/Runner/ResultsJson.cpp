// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include "ResultsJson.hpp"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        JsonValue
        Strings(const std::vector<std::string> &values)
        {
            JsonValue array = JsonValue::Array();
            for (const auto &value : values)
            {
                array.Append(value);
            }
            return array;
        }

        std::vector<std::string>
        StringsFrom(const JsonValue &value)
        {
            std::vector<std::string> strings;
            for (const auto &item : value.Items())
            {
                strings.push_back(item.AsString());
            }
            return strings;
        }

        std::size_t
        Size(const JsonValue &value)
        {
            return static_cast<std::size_t>(value.AsUnsigned());
        }

        /// Adds a member only when it says something, so reports and
        /// protocol messages stay small.
        void
        SetText(JsonValue &object, std::string_view key, const std::string &text)
        {
            if (!text.empty())
            {
                object.Set(key, text);
            }
        }
    } // namespace

    JsonValue
    ToJson(const AssertionRecord &assertion)
    {
        JsonValue value = JsonValue::Object();
        value.Set("file", assertion.file);
        value.Set("line", assertion.line);
        SetText(value, "macro", assertion.macro);
        SetText(value, "expression", assertion.expression);
        SetText(value, "expansion", assertion.expansion);
        SetText(value, "message", assertion.message);
        if (!assertion.context.empty())
        {
            value.Set("context", Strings(assertion.context));
        }
        if (!assertion.sections.empty())
        {
            value.Set("sections", Strings(assertion.sections));
        }
        value.Set("category", CategoryName(assertion.category));
        value.Set("passed", assertion.passed);
        if (assertion.allowed)
        {
            value.Set("allowed", true);
        }
        return value;
    }

    AssertionRecord
    AssertionFromJson(const JsonValue &value)
    {
        AssertionRecord assertion;
        assertion.file = value["file"].AsString();
        assertion.line = Size(value["line"]);
        assertion.macro = value["macro"].AsString();
        assertion.expression = value["expression"].AsString();
        assertion.expansion = value["expansion"].AsString();
        assertion.message = value["message"].AsString();
        assertion.context = StringsFrom(value["context"]);
        assertion.sections = StringsFrom(value["sections"]);
        assertion.category = CategoryFromName(value["category"].AsString())
                                 .value_or(FailureCategory::kAssertion);
        assertion.passed = value["passed"].AsBoolean();
        assertion.allowed = value["allowed"].AsBoolean();
        return assertion;
    }

    JsonValue
    ToJson(const BenchmarkRecord &benchmark)
    {
        JsonValue value = JsonValue::Object();
        value.Set("name", benchmark.name);
        value.Set("samples", benchmark.samples);
        value.Set("iterations", benchmark.iterations);
        value.Set("meanNanoseconds", benchmark.meanNanoseconds);
        value.Set("lowMeanNanoseconds", benchmark.lowMeanNanoseconds);
        value.Set("highMeanNanoseconds", benchmark.highMeanNanoseconds);
        value.Set("standardDeviationNanoseconds",
                  benchmark.standardDeviationNanoseconds);
        value.Set("outlierVariance", benchmark.outlierVariance);
        return value;
    }

    BenchmarkRecord
    BenchmarkFromJson(const JsonValue &value)
    {
        BenchmarkRecord benchmark;
        benchmark.name = value["name"].AsString();
        benchmark.samples = Size(value["samples"]);
        benchmark.iterations = Size(value["iterations"]);
        benchmark.meanNanoseconds = value["meanNanoseconds"].AsNumber();
        benchmark.lowMeanNanoseconds = value["lowMeanNanoseconds"].AsNumber();
        benchmark.highMeanNanoseconds = value["highMeanNanoseconds"].AsNumber();
        benchmark.standardDeviationNanoseconds =
            value["standardDeviationNanoseconds"].AsNumber();
        benchmark.outlierVariance = value["outlierVariance"].AsNumber();
        return benchmark;
    }

    JsonValue
    ToJson(const SectionRecord &section)
    {
        JsonValue value = JsonValue::Object();
        value.Set("name", section.name);
        value.Set("file", section.file);
        value.Set("line", section.line);
        value.Set("depth", section.depth);
        value.Set("startMicroseconds", section.startMicroseconds);
        value.Set("durationMicroseconds", section.durationMicroseconds);
        return value;
    }

    SectionRecord
    SectionFromJson(const JsonValue &value)
    {
        SectionRecord section;
        section.name = value["name"].AsString();
        section.file = value["file"].AsString();
        section.line = Size(value["line"]);
        section.depth = Size(value["depth"]);
        section.startMicroseconds = value["startMicroseconds"].AsInteger();
        section.durationMicroseconds =
            value["durationMicroseconds"].AsInteger();
        return section;
    }

    JsonValue
    ToJson(const Attempt &attempt)
    {
        JsonValue value = JsonValue::Object();
        value.Set("number", attempt.number);
        value.Set("outcome", OutcomeName(attempt.outcome));
        if (attempt.category != FailureCategory::kNone)
        {
            value.Set("category", CategoryName(attempt.category));
        }
        value.Set("worker", attempt.worker);
        value.Set("freshProcess", attempt.freshProcess);
        value.Set("seed", attempt.seed);
        value.Set("startMicroseconds", attempt.startMicroseconds);
        value.Set("durationMicroseconds", attempt.durationMicroseconds);
        value.Set("assertionsPassed", attempt.assertionsPassed);
        value.Set("assertionsFailed", attempt.assertionsFailed);
        if (!attempt.precededBy.empty())
        {
            value.Set("precededBy", Strings(attempt.precededBy));
        }
        JsonValue assertions = JsonValue::Array();
        for (const auto &assertion : attempt.assertions)
        {
            assertions.Append(ToJson(assertion));
        }
        value.Set("assertions", std::move(assertions));
        if (!attempt.benchmarks.empty())
        {
            JsonValue benchmarks = JsonValue::Array();
            for (const auto &benchmark : attempt.benchmarks)
            {
                benchmarks.Append(ToJson(benchmark));
            }
            value.Set("benchmarks", std::move(benchmarks));
        }
        if (!attempt.sections.empty())
        {
            JsonValue sections = JsonValue::Array();
            for (const auto &section : attempt.sections)
            {
                sections.Append(ToJson(section));
            }
            value.Set("sections", std::move(sections));
        }
        SetText(value, "standardOutput", attempt.standardOutput);
        SetText(value, "standardError", attempt.standardError);
        SetText(value, "processOutput", attempt.processOutput);
        SetText(value, "fatalMessage", attempt.fatalMessage);
        SetText(value, "exitDescription", attempt.exitDescription);
        SetText(value, "lastLocation", attempt.lastLocation);
        SetText(value, "temporaryDirectory", attempt.temporaryDirectory);
        return value;
    }

    Attempt
    AttemptFromJson(const JsonValue &value)
    {
        Attempt attempt;
        attempt.number = Size(value["number"]);
        attempt.outcome =
            OutcomeFromName(value["outcome"].AsString()).value_or(Outcome::kNotRun);
        attempt.category = CategoryFromName(value["category"].AsString())
                               .value_or(FailureCategory::kNone);
        attempt.worker = Size(value["worker"]);
        attempt.freshProcess = value["freshProcess"].AsBoolean();
        attempt.seed = static_cast<std::uint32_t>(value["seed"].AsUnsigned());
        attempt.startMicroseconds = value["startMicroseconds"].AsInteger();
        attempt.durationMicroseconds = value["durationMicroseconds"].AsInteger();
        attempt.assertionsPassed = Size(value["assertionsPassed"]);
        attempt.assertionsFailed = Size(value["assertionsFailed"]);
        attempt.precededBy = StringsFrom(value["precededBy"]);
        for (const auto &item : value["assertions"].Items())
        {
            attempt.assertions.push_back(AssertionFromJson(item));
        }
        for (const auto &item : value["benchmarks"].Items())
        {
            attempt.benchmarks.push_back(BenchmarkFromJson(item));
        }
        for (const auto &item : value["sections"].Items())
        {
            attempt.sections.push_back(SectionFromJson(item));
        }
        attempt.standardOutput = value["standardOutput"].AsString();
        attempt.standardError = value["standardError"].AsString();
        attempt.processOutput = value["processOutput"].AsString();
        attempt.fatalMessage = value["fatalMessage"].AsString();
        attempt.exitDescription = value["exitDescription"].AsString();
        attempt.lastLocation = value["lastLocation"].AsString();
        attempt.temporaryDirectory = value["temporaryDirectory"].AsString();
        return attempt;
    }

    JsonValue
    ToJson(const TestResult &test)
    {
        JsonValue value = JsonValue::Object();
        value.Set("id", test.id);
        value.Set("name", test.name);
        SetText(value, "className", test.className);
        value.Set("file", test.file);
        value.Set("line", test.line);
        value.Set("tags", Strings(test.tags));
        value.Set("outcome", OutcomeName(test.outcome));
        if (test.category != FailureCategory::kNone)
        {
            value.Set("category", CategoryName(test.category));
        }
        value.Set("history", HistoryName(test.history));
        value.Set("flaky", test.flaky);
        value.Set("quarantined", test.quarantined);
        value.Set("durationMicroseconds", test.DurationMicroseconds());
        SetText(value, "selectionReason", test.selectionReason);
        SetText(value, "signature", test.signature);
        if (!test.analysis.empty())
        {
            value.Set("analysis", Strings(test.analysis));
        }
        SetText(value, "replayCommand", test.replayCommand);
        value.Set("predictedDurationMicroseconds",
                  test.predictedDurationMicroseconds);
        value.Set("predictedFailureProbability",
                  test.predictedFailureProbability);
        JsonValue attempts = JsonValue::Array();
        for (const auto &attempt : test.attempts)
        {
            attempts.Append(ToJson(attempt));
        }
        value.Set("attempts", std::move(attempts));
        return value;
    }

    TestResult
    TestResultFromJson(const JsonValue &value)
    {
        TestResult test;
        test.id = value["id"].AsString();
        test.name = value["name"].AsString();
        test.className = value["className"].AsString();
        test.file = value["file"].AsString();
        test.line = Size(value["line"]);
        test.tags = StringsFrom(value["tags"]);
        test.outcome =
            OutcomeFromName(value["outcome"].AsString()).value_or(Outcome::kNotRun);
        test.category = CategoryFromName(value["category"].AsString())
                            .value_or(FailureCategory::kNone);
        const std::string &history = value["history"].AsString();
        test.history = history == "new"            ? FailureHistory::kNew
                       : history == "persistent"   ? FailureHistory::kPersistent
                       : history == "intermittent" ? FailureHistory::kIntermittent
                                                   : FailureHistory::kUnknown;
        test.flaky = value["flaky"].AsBoolean();
        test.quarantined = value["quarantined"].AsBoolean();
        test.selectionReason = value["selectionReason"].AsString();
        test.signature = value["signature"].AsString();
        test.analysis = StringsFrom(value["analysis"]);
        test.replayCommand = value["replayCommand"].AsString();
        test.predictedDurationMicroseconds =
            value["predictedDurationMicroseconds"].AsInteger();
        test.predictedFailureProbability =
            value["predictedFailureProbability"].AsNumber();
        for (const auto &item : value["attempts"].Items())
        {
            test.attempts.push_back(AttemptFromJson(item));
        }
        return test;
    }

    JsonValue
    ToJson(const RunResult &run)
    {
        const RunSummary summary = Summarize(run);
        JsonValue value = JsonValue::Object();
        value.Set("format", "catch3-run");
        value.Set("version", kResultsFormatVersion);
        value.Set("binary", run.binary);
        value.Set("seed", run.seed);
        value.Set("workers", run.workers);
        value.Set("shardIndex", run.shardIndex);
        value.Set("shardCount", run.shardCount);
        value.Set("candidates", run.candidates);
        value.Set("startUnixMilliseconds", run.startUnixMilliseconds);
        value.Set("wallMicroseconds", run.wallMicroseconds);
        JsonValue totals = JsonValue::Object();
        totals.Set("tests", summary.tests);
        totals.Set("passed", summary.passed);
        totals.Set("failed", summary.failed);
        totals.Set("failedAsExpected", summary.failedAsExpected);
        totals.Set("skipped", summary.skipped);
        totals.Set("crashed", summary.crashed);
        totals.Set("timedOut", summary.timedOut);
        totals.Set("blocked", summary.blocked);
        totals.Set("notRun", summary.notRun);
        totals.Set("upToDate", summary.upToDate);
        totals.Set("flaky", summary.flaky);
        totals.Set("quarantined", summary.quarantined);
        totals.Set("attempts", summary.attempts);
        totals.Set("assertionsPassed", summary.assertionsPassed);
        totals.Set("assertionsFailed", summary.assertionsFailed);
        totals.Set("testMicroseconds", summary.testMicroseconds);
        value.Set("summary", std::move(totals));
        value.Set("unmatchedSpecs", Strings(run.unmatchedSpecs));
        value.Set("notes", Strings(run.notes));
        JsonValue tests = JsonValue::Array();
        for (const auto &test : run.tests)
        {
            tests.Append(ToJson(test));
        }
        value.Set("tests", std::move(tests));
        return value;
    }

    Expected<RunResult>
    RunResultFromJson(const JsonValue &value)
    {
        if (value["format"].AsString() != "catch3-run")
        {
            return Error{"not a Catch3 run report"};
        }
        const auto version = value["version"].AsInteger();
        if (version < 1 || version > kResultsFormatVersion)
        {
            return Error{"report version " + std::to_string(version) +
                         " is newer than this Catch3 reads (" +
                         std::to_string(kResultsFormatVersion) + ")"};
        }
        RunResult run;
        run.binary = value["binary"].AsString();
        run.seed = static_cast<std::uint32_t>(value["seed"].AsUnsigned());
        run.workers = Size(value["workers"]);
        run.shardIndex = static_cast<unsigned>(value["shardIndex"].AsUnsigned());
        run.shardCount =
            static_cast<unsigned>(value["shardCount"].AsUnsigned(1));
        run.candidates = Size(value["candidates"]);
        run.startUnixMilliseconds = value["startUnixMilliseconds"].AsInteger();
        run.wallMicroseconds = value["wallMicroseconds"].AsInteger();
        run.unmatchedSpecs = StringsFrom(value["unmatchedSpecs"]);
        run.notes = StringsFrom(value["notes"]);
        for (const auto &item : value["tests"].Items())
        {
            run.tests.push_back(TestResultFromJson(item));
        }
        return run;
    }
} // namespace Progmasoft::Catch3::Runner
