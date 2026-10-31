// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include "Planner.hpp"

#include "Files.hpp"
#include "Hash.hpp"
#include "Json.hpp"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <numeric>
#include <random>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        using std::chrono::milliseconds;

        /// The duration assumed for a test never seen before.
        constexpr std::int64_t kUnknownDurationMicroseconds = 500000;

        /// Durations by test key, from a history or from a JSON report, for
        /// balancing shards.
        Expected<std::map<std::string, std::int64_t>>
        LoadDurations(const std::string &path)
        {
            auto text = ReadFile(path);
            if (!text)
            {
                return text.GetError();
            }
            auto json = ParseJson(*text);
            if (!json)
            {
                return json.GetError().Context(path);
            }
            std::map<std::string, std::int64_t> durations;
            const std::string &format = (*json)["format"].AsString();
            if (format == "catch3-history")
            {
                auto history = History::FromJson(*json);
                if (!history)
                {
                    return history.GetError().Context(path);
                }
                for (const auto &[key, test] : history->Tests())
                {
                    if (const auto median = test.DurationPercentile(0.5))
                    {
                        durations[key] = *median;
                    }
                }
            }
            else if (format == "catch3-run")
            {
                for (const auto &test : (*json)["tests"].Items())
                {
                    const std::string key = test["id"].AsString().empty()
                                                ? test["name"].AsString()
                                                : test["id"].AsString();
                    durations[key] =
                        test["durationMicroseconds"].AsInteger();
                }
            }
            else
            {
                return Error{path + " is neither a Catch3 history nor a "
                                    "Catch3 JSON report"};
            }
            return durations;
        }

        std::string
        Percent(double probability)
        {
            char buffer[16];
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%.0f%%",
                          probability * 100.0);
            return buffer;
        }

        /// Finds the root of a set in a union-find forest.
        std::size_t
        Root(std::vector<std::size_t> &parents, std::size_t index)
        {
            while (parents[index] != index)
            {
                parents[index] = parents[parents[index]];
                index = parents[index];
            }
            return index;
        }
    } // namespace

    unsigned
    StableShard(const std::string &key, unsigned count) noexcept
    {
        if (count <= 1)
        {
            return 0;
        }
        return static_cast<unsigned>(Fnv1a64(key) % count);
    }

    Expected<Plan>
    MakePlan(const Options &options, const PlanInputs &inputs)
    {
        const Catalog &catalog = *inputs.catalog;
        const auto &tests = catalog.Tests();
        const History *history = inputs.history;
        Plan plan;
        plan.unmatchedSpecs = inputs.unmatchedSpecs;
        plan.candidates = inputs.matched.size();

        const auto historyOf = [&](const TestEntry &entry) -> const TestHistory * {
            return history == nullptr ? nullptr : history->Find(entry.key);
        };

        std::vector<bool> selected(tests.size(), false);
        std::vector<std::string> reasons(tests.size());
        std::vector<std::optional<std::vector<std::string>>> replayKeys(
            tests.size());
        std::vector<std::optional<std::uint32_t>> replaySeeds(tests.size());

        // 1. Selection.
        if (options.replayFailures)
        {
            if (history != nullptr)
            {
                for (const auto &[key, test] : history->Tests())
                {
                    const TestEntry *entry = catalog.Find(key);
                    if (entry == nullptr || !test.lastFailure)
                    {
                        continue;
                    }
                    selected[entry->index] = true;
                    reasons[entry->index] =
                        "failed in run " + std::to_string(test.lastFailure->run);
                    replayKeys[entry->index] = test.lastFailure->precededBy;
                    replaySeeds[entry->index] = test.lastFailure->seed;
                }
            }
            if (std::none_of(selected.begin(), selected.end(), [](bool b) {
                    return b;
                }))
            {
                plan.notes.emplace_back(
                    "the history holds no failure to replay");
            }
        }
        else
        {
            const bool needsChanges =
                options.selection == Selection::kChanged ||
                options.selection == Selection::kSmart;
            if (needsChanges && inputs.changes == nullptr)
            {
                plan.notes.emplace_back(
                    "Git could not list the changed files, so no test is "
                    "selected for a change");
            }
            for (const std::size_t index : inputs.matched)
            {
                const TestEntry &entry = tests[index];
                const TestHistory *past = historyOf(entry);
                const TestRecord *last = past == nullptr ? nullptr : past->Last();
                const bool failedLast =
                    last != nullptr && (IsFailure(last->outcome) || last->flaky);
                const bool isNew = last == nullptr;
                bool changed = false;
                if (inputs.changes != nullptr)
                {
                    for (const auto &file : inputs.changes->files)
                    {
                        if (SameSourceFile(file, entry.file))
                        {
                            changed = true;
                            break;
                        }
                    }
                }
                const double probability =
                    past == nullptr ? 0.3 : past->FailureProbability();
                switch (options.selection)
                {
                    case Selection::kAll:
                        selected[index] = true;
                        break;
                    case Selection::kFailed:
                        if (failedLast)
                        {
                            selected[index] = true;
                            reasons[index] = last->flaky ? "flaky last run"
                                                         : "failed last run";
                        }
                        break;
                    case Selection::kNew:
                        if (isNew)
                        {
                            selected[index] = true;
                            reasons[index] = "no earlier result";
                        }
                        break;
                    case Selection::kChanged:
                        if (changed)
                        {
                            selected[index] = true;
                            reasons[index] = "its source file changed";
                        }
                        break;
                    case Selection::kSmart:
                        if (changed)
                        {
                            reasons[index] = "its source file changed";
                        }
                        else if (failedLast)
                        {
                            reasons[index] = "failed last run";
                        }
                        else if (isNew)
                        {
                            reasons[index] = "no earlier result";
                        }
                        else if (probability >= 0.5)
                        {
                            reasons[index] = "likely to fail (" +
                                             Percent(probability) + ")";
                        }
                        else if (past != nullptr && past->quarantine)
                        {
                            reasons[index] = "in quarantine";
                        }
                        selected[index] = !reasons[index].empty();
                        break;
                }
            }
        }

        // 2. Dependencies come along, whatever selected the test.
        std::vector<std::vector<std::size_t>> dependencies(tests.size());
        std::vector<std::size_t> pending;
        for (std::size_t index = 0; index < tests.size(); ++index)
        {
            if (selected[index])
            {
                pending.push_back(index);
            }
        }
        while (!pending.empty())
        {
            const std::size_t index = pending.back();
            pending.pop_back();
            for (const auto &name : tests[index].dependsOn)
            {
                const TestEntry *dependency = catalog.Find(name);
                if (dependency == nullptr)
                {
                    return Error{"test '" + tests[index].name +
                                 "' depends on '" + name +
                                 "', which is not a test of this binary"};
                }
                dependencies[index].push_back(dependency->index);
                if (!selected[dependency->index])
                {
                    selected[dependency->index] = true;
                    reasons[dependency->index] =
                        "needed by '" + tests[index].name + "'";
                    pending.push_back(dependency->index);
                }
            }
        }

        // 3. A cycle can never run.
        {
            std::vector<int> state(tests.size(), 0);
            std::vector<std::size_t> path;
            std::string cycle;
            const std::function<bool(std::size_t)> visit =
                [&](std::size_t index) {
                    state[index] = 1;
                    path.push_back(index);
                    for (const std::size_t next : dependencies[index])
                    {
                        if (state[next] == 1)
                        {
                            const auto start =
                                std::find(path.begin(), path.end(), next);
                            for (auto at = start; at != path.end(); ++at)
                            {
                                cycle += "'" + tests[*at].name + "' -> ";
                            }
                            cycle += "'" + tests[next].name + "'";
                            return true;
                        }
                        if (state[next] == 0 && visit(next))
                        {
                            return true;
                        }
                    }
                    path.pop_back();
                    state[index] = 2;
                    return false;
                };
            for (std::size_t index = 0; index < tests.size(); ++index)
            {
                if (selected[index] && state[index] == 0 && visit(index))
                {
                    return Error{"the tests depend on each other in a "
                                 "cycle: " +
                                 cycle};
                }
            }
        }

        // 4. Sharding. Tests linked by dependencies stay in one shard.
        if (options.shard.count > 1)
        {
            std::vector<std::size_t> parents(tests.size());
            std::iota(parents.begin(), parents.end(), std::size_t{0});
            for (std::size_t index = 0; index < tests.size(); ++index)
            {
                for (const std::size_t dependency : dependencies[index])
                {
                    parents[Root(parents, index)] = Root(parents, dependency);
                }
            }
            // A group is named by its first key, in registration order.
            std::map<std::size_t, std::vector<std::size_t>> groups;
            for (std::size_t index = 0; index < tests.size(); ++index)
            {
                if (selected[index])
                {
                    groups[Root(parents, index)].push_back(index);
                }
            }
            std::vector<unsigned> shardOf(tests.size(), 0);
            if (!options.shardDurations.empty())
            {
                auto durations = LoadDurations(options.shardDurations);
                if (!durations)
                {
                    return durations.GetError();
                }
                struct Group
                {
                    std::int64_t duration = 0;
                    std::string key;
                    const std::vector<std::size_t> *members = nullptr;
                };
                std::vector<Group> ordered;
                for (const auto &[root, members] : groups)
                {
                    Group group;
                    group.members = &members;
                    group.key = tests[members.front()].key;
                    for (const std::size_t member : members)
                    {
                        const auto found = durations->find(tests[member].key);
                        group.duration += found == durations->end()
                                              ? kUnknownDurationMicroseconds
                                              : found->second;
                    }
                    ordered.push_back(group);
                }
                // Longest first into the least loaded shard balances the
                // shards; the order is total, so every machine agrees.
                std::sort(ordered.begin(),
                          ordered.end(),
                          [](const Group &left, const Group &right) {
                              return left.duration != right.duration
                                         ? left.duration > right.duration
                                         : left.key < right.key;
                          });
                std::vector<std::int64_t> load(options.shard.count, 0);
                for (const auto &group : ordered)
                {
                    const auto least = static_cast<unsigned>(
                        std::min_element(load.begin(), load.end()) -
                        load.begin());
                    load[least] += group.duration;
                    for (const std::size_t member : *group.members)
                    {
                        shardOf[member] = least;
                    }
                }
            }
            else
            {
                for (const auto &[root, members] : groups)
                {
                    const unsigned shard = StableShard(
                        tests[members.front()].key, options.shard.count);
                    for (const std::size_t member : members)
                    {
                        shardOf[member] = shard;
                    }
                }
            }
            for (std::size_t index = 0; index < tests.size(); ++index)
            {
                if (selected[index] && shardOf[index] != options.shard.index)
                {
                    selected[index] = false;
                }
            }
        }

        // 5. Fingerprints, and what an incremental run may keep.
        std::vector<bool> kept(tests.size(), false);
        for (std::size_t index = 0; index < tests.size(); ++index)
        {
            if (!selected[index])
            {
                continue;
            }
            const TestEntry &entry = tests[index];
            const std::string fingerprint =
                Sha256Hex(inputs.binaryFingerprint + "\n" + entry.key);
            plan.fingerprints[entry.key] = fingerprint;
            if (!options.incremental || options.replayFailures)
            {
                continue;
            }
            const TestHistory *past = historyOf(entry);
            const TestRecord *last = past == nullptr ? nullptr : past->Last();
            if (last != nullptr && last->outcome == Outcome::kPassed &&
                !last->flaky && last->fingerprint == fingerprint &&
                !(past->quarantine && options.quarantine != QuarantineMode::kOff))
            {
                kept[index] = true;
                TestResult result;
                result.id = entry.key;
                result.name = entry.name;
                result.className = entry.className;
                result.file = entry.file;
                result.line = entry.line;
                result.tags = entry.tags;
                result.outcome = Outcome::kUpToDate;
                result.selectionReason = reasons[index];
                plan.kept.push_back(std::move(result));
            }
        }

        // 6. The tests to run, with their limits and predictions.
        std::vector<std::size_t> order;
        for (std::size_t index = 0; index < tests.size(); ++index)
        {
            if (selected[index] && !kept[index])
            {
                order.push_back(index);
            }
        }
        std::map<std::size_t, PlannedTest> planned;
        for (const std::size_t index : order)
        {
            const TestEntry &entry = tests[index];
            const TestHistory *past = historyOf(entry);
            PlannedTest test;
            test.entry = &entry;
            test.selectionReason = reasons[index];
            test.fingerprint = plan.fingerprints[entry.key];
            test.failureProbability =
                past == nullptr ? 0.3 : past->FailureProbability();
            test.flakyHistory = past != nullptr && past->FlakyEvents(20) > 0;
            const auto median =
                past == nullptr ? std::nullopt : past->DurationPercentile(0.5);
            test.predictedMicroseconds =
                median ? *median : kUnknownDurationMicroseconds;

            const milliseconds cap =
                options.timeoutGiven ? options.timeout : milliseconds{300000};
            if (entry.timeout)
            {
                test.timeout = *entry.timeout;
            }
            else if (options.adaptiveTimeout && !entry.benchmark && past &&
                     median)
            {
                // Generous enough for a slow machine, tight enough that a
                // hang is noticed long before the global limit.
                const auto p95 = past->DurationPercentile(0.95).value_or(*median);
                const auto learned = std::max(p95 * 4, *median * 10) / 1000 + 2000;
                test.timeout = milliseconds{std::max<std::int64_t>(5000, learned)};
                if (cap.count() > 0 && test.timeout > cap)
                {
                    test.timeout = cap;
                }
                test.timeoutLearned = true;
            }
            else
            {
                test.timeout = cap;
            }

            if (entry.retries)
            {
                test.retryLimit = *entry.retries;
            }
            else
            {
                test.retryLimit = options.retryMode == RetryMode::kOff
                                      ? 0
                                      : options.retries;
            }
            if (options.quarantine != QuarantineMode::kOff)
            {
                if (entry.quarantined)
                {
                    test.quarantined = true;
                    test.quarantineReason = "tagged [!quarantine]";
                }
                else if (past != nullptr && past->quarantine)
                {
                    test.quarantined = true;
                    test.quarantineReason = past->quarantine->reason;
                }
            }
            if (replaySeeds[index])
            {
                test.replaySeed = replaySeeds[index];
            }
            planned.emplace(index, std::move(test));
        }

        // 7. The order the scheduler prefers.
        switch (options.order)
        {
            case Order::kDeclared:
                break;
            case Order::kName:
                std::stable_sort(order.begin(),
                                 order.end(),
                                 [&](std::size_t left, std::size_t right) {
                                     return tests[left].name < tests[right].name;
                                 });
                break;
            case Order::kRandom:
            {
                std::mt19937 generator{options.seed};
                std::shuffle(order.begin(), order.end(), generator);
                break;
            }
            case Order::kPriority:
            {
                // Tests likely to fail go first, the most failures per second
                // first, so a failing run fails early. The rest go longest
                // first, which keeps the workers busy until the end.
                const auto risky = [&](std::size_t index) {
                    return planned.at(index).failureProbability >= 0.5;
                };
                const auto rate = [&](std::size_t index) {
                    const auto &test = planned.at(index);
                    const double seconds = std::max(
                        0.05,
                        static_cast<double>(test.predictedMicroseconds) / 1e6);
                    return test.failureProbability / seconds;
                };
                std::stable_sort(
                    order.begin(),
                    order.end(),
                    [&](std::size_t left, std::size_t right) {
                        if (risky(left) != risky(right))
                        {
                            return risky(left);
                        }
                        if (risky(left))
                        {
                            return rate(left) > rate(right);
                        }
                        return planned.at(left).predictedMicroseconds >
                               planned.at(right).predictedMicroseconds;
                    });
                break;
            }
        }
        std::map<std::size_t, std::size_t> position;
        for (std::size_t at = 0; at < order.size(); ++at)
        {
            position[order[at]] = at;
        }
        std::map<std::string, std::size_t> indexByKey;
        for (const auto &entry : tests)
        {
            indexByKey[entry.key] = entry.index;
        }
        for (const std::size_t index : order)
        {
            PlannedTest test = std::move(planned.at(index));
            for (const std::size_t dependency : dependencies[index])
            {
                // A dependency kept as up to date has already passed.
                if (const auto found = position.find(dependency);
                    found != position.end())
                {
                    test.dependencies.push_back(found->second);
                }
            }
            if (replayKeys[index])
            {
                std::vector<std::size_t> sequence;
                for (const auto &key : *replayKeys[index])
                {
                    if (const auto found = indexByKey.find(key);
                        found != indexByKey.end())
                    {
                        sequence.push_back(found->second);
                    }
                }
                test.replaySequence = std::move(sequence);
            }
            plan.tests.push_back(std::move(test));
        }
        return plan;
    }
} // namespace Progmasoft::Catch3::Runner
