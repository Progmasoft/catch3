// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include "Catalog.hpp"
#include "Expected.hpp"
#include "Git.hpp"
#include "History.hpp"
#include "Options.hpp"

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Progmasoft::Catch3::Runner
{
    /// One test the run will start, with everything decided in advance.
    struct PlannedTest final
    {
        const TestEntry *entry = nullptr;
        std::string selectionReason;
        double failureProbability = 0.0;
        std::int64_t predictedMicroseconds = 0;
        /// Zero: no limit.
        std::chrono::milliseconds timeout{0};
        bool timeoutLearned = false;
        unsigned retryLimit = 0;
        bool quarantined = false;
        std::string quarantineReason;
        /// The history shows the test passing and failing with the same
        /// inputs.
        bool flakyHistory = false;
        /// Positions in `Plan::tests` of the tests this one waits for.
        std::vector<std::size_t> dependencies;
        /// What the result depends on; equal for an unchanged test.
        std::string fingerprint;
        /// For --replay: the tests (catalog indices) to run before it in the
        /// same fresh process, and the seed it failed with.
        std::optional<std::vector<std::size_t>> replaySequence;
        std::optional<std::uint32_t> replaySeed;
    };

    /// What a run will do, decided before the first test starts.
    struct Plan final
    {
        /// In the order the scheduler should prefer.
        std::vector<PlannedTest> tests;
        /// Tests whose earlier result stands: up to date or out of the
        /// selection but needed by nothing.
        std::vector<TestResult> kept;
        std::vector<std::string> unmatchedSpecs;
        std::vector<std::string> notes;
        /// Tests that matched the specs, before selection and sharding.
        std::size_t candidates = 0;
        std::map<std::string, std::string> fingerprints;
    };

    /// What the planner reads besides the options.
    struct PlanInputs final
    {
        const Catalog *catalog = nullptr;
        /// The tests the specs matched, in registration order.
        std::vector<std::size_t> matched;
        std::vector<std::string> unmatchedSpecs;
        const History *history = nullptr;
        /// Present when Git answered.
        const Changes *changes = nullptr;
        /// A digest of the test binary.
        std::string binaryFingerprint;
    };

    [[nodiscard]] Expected<Plan>
    MakePlan(const Options &options, const PlanInputs &inputs);

    /// The shard of a group of tests that must stay together, by a hash of
    /// its first key. Every machine computes the same answer.
    [[nodiscard]] unsigned
    StableShard(const std::string &key, unsigned count) noexcept;
} // namespace Progmasoft::Catch3::Runner
