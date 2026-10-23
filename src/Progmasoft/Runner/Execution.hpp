// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "Catalog.hpp"
#include "Json.hpp"
#include "Options.hpp"

namespace Catch
{
    struct ConfigData;
} // namespace Catch

namespace Progmasoft::Catch3::Runner
{
    /// The Catch2 configuration a process that runs tests uses for
    /// `options`.
    [[nodiscard]] Catch::ConfigData
    ToConfigData(const Options &options, const std::string &programName);

    /// Receives the events of the tests a `TestExecutor` runs, as the JSON
    /// messages a worker sends; see `Protocol.hpp`.
    using EventSink = std::function<void(const JsonValue &)>;

    /// Runs tests of this binary in this process with Catch2's engine: the
    /// sections, generators, assertions and listeners are Catch2's.
    ///
    /// One executor lives for the whole process, because Catch2 allows one
    /// run context per process; listeners see one test run, from the first
    /// test to the destruction of the executor.
    class TestExecutor final
    {
    public:
        TestExecutor(const Options &options,
                     const Catalog &catalog,
                     const std::string &programName,
                     EventSink sink);
        TestExecutor(const TestExecutor &) = delete;
        TestExecutor &
        operator=(const TestExecutor &) = delete;
        ~TestExecutor();

        /// Runs one test. `temporaryDirectory` becomes TMPDIR, TMP and TEMP
        /// while it runs; empty leaves them alone.
        void
        Run(std::size_t index,
            std::size_t attempt,
            const std::string &temporaryDirectory);

        /// The executor's Catch2 configuration and run context.
        struct State;

    private:
        std::unique_ptr<State> state_;
    };

    /// Serves a runner over the standard streams until it says to stop.
    [[nodiscard]] int
    RunWorker(const Options &options, const std::string &programName);
} // namespace Progmasoft::Catch3::Runner
