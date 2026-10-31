// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Property.hpp>
#include <Progmasoft/Catch3/Strategy/ChoiceSource.hpp>
#include <Progmasoft/Catch3/Strategy/Context.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Progmasoft::Catch3::Detail
{

    enum class TrialVerdict : std::uint8_t
    {
        kPassed,
        kFailed,
        /// The body called CATCH3_PROPERTY_ASSUME with a false condition.
        kDiscarded,
        /// The strategies described no value, for example a filter that
        /// found nothing.
        kInvalid,
        /// A strategy threw; the property cannot be judged.
        kErrored,
    };

    struct TrialOutcome final
    {
        TrialVerdict Verdict = TrialVerdict::kPassed;
        std::string Reason;
        TrialRecord Record;
        /// The input, rendered; only filled when the engine asks for it.
        std::string Description;
    };

    /// Draws the input of one trial from the source and runs the body on
    /// it. describe asks for TrialOutcome::Description.
    using TrialRunner = std::function<TrialOutcome(ChoiceSource &, bool describe)>;

    /// Runs a strategy-based property: the regular trials, the targeted
    /// search, the coverage requirements, the example database and the
    /// shrinking of a failure. Independent of the value types, so that it
    /// is compiled once.
    [[nodiscard]] PropertyResult
    RunPropertyEngine(const PropertyOptions &options, const TrialRunner &runner);

    /// A replay token: the size of the trial and its choices.
    [[nodiscard]] std::string
    EncodeChoices(std::size_t size, const std::vector<std::uint64_t> &choices);

    struct DecodedChoices final
    {
        std::size_t Size = 0;
        std::vector<std::uint64_t> Choices;
    };

    [[nodiscard]] std::optional<DecodedChoices>
    DecodeChoices(std::string_view token);

    /// Minimizes a failing choice sequence: deletes and zeroes blocks of
    /// choices, lowers single choices by binary search, sorts neighbours
    /// and moves value between neighbours. stillFails replays a candidate
    /// and returns the choices the replay consumed when it fails too.
    /// Returns the simplest failing sequence found within budget replays;
    /// calls counts the replays made.
    [[nodiscard]] std::vector<std::uint64_t>
    ShrinkChoices(
        std::vector<std::uint64_t> failing,
        std::size_t budget,
        const std::function<std::optional<std::vector<std::uint64_t>>(
            const std::vector<std::uint64_t> &)> &stillFails,
        std::size_t &calls);

    /// Shortlex order: shorter first, then lexicographically smaller.
    [[nodiscard]] bool
    IsSimpler(const std::vector<std::uint64_t> &candidate,
              const std::vector<std::uint64_t> &current) noexcept;

} // namespace Progmasoft::Catch3::Detail
