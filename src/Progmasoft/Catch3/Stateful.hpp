// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/ForAll.hpp>
#include <Progmasoft/Catch3/Strategy/Containers.hpp>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{

    /// One step of a stateful test: a description for the report and the
    /// code that applies the step to the model and to the system under
    /// test. It checks the system against the model with
    /// CATCH3_PROPERTY_ASSERT.
    template<typename Model, typename System>
    struct Action final
    {
        std::string Description;
        std::function<void(Model &, System &)> Apply;
    };

    /// A kind of step. Make builds the step from the current model, so its
    /// arguments can depend on the state, for example by choosing a key
    /// that the model holds. Precondition says whether the step may run in
    /// a state; an unset precondition always holds.
    template<typename Model, typename System>
    struct Command final
    {
        std::string Name;
        std::function<Strategy<Action<Model, System>>(const Model &)> Make;
        std::function<bool(const Model &)> Precondition = {};
    };

    /// A command without arguments.
    template<typename Model, typename System, typename Apply>
    [[nodiscard]] Command<Model, System>
    MakeCommand(std::string name, Apply apply)
    {
        return Command<Model, System>{
            name,
            [name, apply](const Model &) {
                return Gen::Just(Action<Model, System>{ name, apply });
            },
        };
    }

    namespace Detail
    {

        template<typename Model, typename System>
        [[nodiscard]] TrialOutcome
        RunCommands(ChoiceSource &source,
                    bool describe,
                    const std::function<Model()> &initialModel,
                    const std::function<System(const Model &)> &makeSystem,
                    const std::vector<Command<Model, System>> &commands,
                    const SizeRange &steps)
        {
            TrialOutcome outcome;
            Model model = initialModel();
            std::optional<System> system;
            std::vector<std::string> log;
            {
                TrialRecordScope scope(outcome.Record);
                system.emplace(makeSystem(model));
            }
            const auto addStep = [&]() -> bool {
                // The choice names a command; when its precondition does not
                // hold, the next commands are tried in order, so that the
                // same choice always leads to the same command.
                const std::size_t first = static_cast<std::size_t>(
                    source.Draw(commands.size() - 1U));
                const Command<Model, System> *chosen = nullptr;
                for (std::size_t offset = 0; offset < commands.size(); ++offset)
                {
                    const auto &candidate
                        = commands[(first + offset) % commands.size()];
                    if (!candidate.Precondition || candidate.Precondition(model))
                    {
                        chosen = &candidate;
                        break;
                    }
                }
                if (chosen == nullptr)
                {
                    return false;
                }
                auto action = DrawGuarded(chosen->Make(model), source, outcome);
                if (!action.has_value())
                {
                    return false;
                }
                log.push_back(action->Description);
                RunBody(
                    [&]() { action->Apply(model, *system); },
                    std::tuple<>(),
                    outcome);
                return outcome.Verdict == TrialVerdict::kPassed;
            };
            (void)Detail::DrawElements(source, steps, addStep);
            if (outcome.Verdict == TrialVerdict::kInvalid && !log.empty())
            {
                // A step that could not be built ends the sequence; the
                // steps before it still ran and passed.
                outcome.Verdict = TrialVerdict::kPassed;
            }
            if (describe)
            {
                std::string text = "{";
                for (std::size_t index = 0; index < log.size(); ++index)
                {
                    text += index == 0 ? " " : ", ";
                    text += log[index];
                }
                outcome.Description = text + (log.empty() ? "}" : " }");
            }
            return outcome;
        }

    } // namespace Detail

    /// Model-based testing: runs random sequences of commands against a
    /// model and the system under test, side by side. A failing sequence
    /// shrinks to a short one with simple arguments, since dropping a step
    /// is deleting its choices.
    template<typename Model, typename System>
    [[nodiscard]] PropertyResult
    CheckStateful(const PropertyOptions &options,
                  std::function<Model()> initialModel,
                  std::function<System(const Model &)> makeSystem,
                  std::vector<Command<Model, System>> commands,
                  SizeRange steps = {})
    {
        if (commands.empty())
        {
            Detail::RejectInvalidArgument(
                "CheckStateful requires at least one command");
        }
        Detail::CheckRange(steps);
        const Detail::TrialRunner runner
            = [&](ChoiceSource &source, bool describe) {
                  return Detail::RunCommands(source,
                                             describe,
                                             initialModel,
                                             makeSystem,
                                             commands,
                                             steps);
              };
        return Detail::RunPropertyEngine(options, runner);
    }

} // namespace Progmasoft::Catch3
