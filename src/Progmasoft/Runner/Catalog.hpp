// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/TestCaseInfo.hpp>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Expected.hpp"

namespace Catch
{
    class IConfig;
} // namespace Catch

namespace Progmasoft::Catch3::Runner
{
    /// One registered test and what its tags tell the runner.
    struct TestEntry final
    {
        /// The position in registration order, the same in every process
        /// started from one binary.
        std::size_t index = 0;
        /// A name that identifies the test across runs and builds: the test
        /// name, with its class when it has one, and its tags when another
        /// test has the same name.
        std::string key;
        std::string name;
        std::string className;
        std::string file;
        std::size_t line = 0;
        std::vector<std::string> tags;
        bool hidden = false;
        bool benchmark = false;
        bool mayFail = false;
        bool shouldFail = false;

        // From the tags the runner reserves.
        /// `[!serial]`: nothing else runs at the same time.
        bool serial = false;
        /// `[!isolated]`: runs in a process of its own.
        bool isolated = false;
        /// `[!quarantine]`: in quarantine whatever its history says.
        bool quarantined = false;
        /// `[!resource:NAME]`: tests sharing a resource never overlap.
        std::vector<std::string> resources;
        /// `[!depends:NAME]`: runs after these tests, and only if they pass.
        std::vector<std::string> dependsOn;
        /// `[!timeout:SECONDS]`.
        std::optional<std::chrono::milliseconds> timeout;
        /// `[!retries:N]`.
        std::optional<unsigned> retries;
    };

    /// The tests registered in this binary.
    class Catalog final
    {
    public:
        /// Reads the test registry. Problems with a reserved tag, such as a
        /// timeout that is not a number, are errors.
        [[nodiscard]] static Expected<Catalog>
        Load();

        /// A catalog of the given tests, without a registry behind it: the
        /// tests cannot run, but they can be planned. Each entry's index
        /// becomes its position, and an empty key becomes its name.
        [[nodiscard]] static Catalog
        FromEntries(std::vector<TestEntry> entries);

        [[nodiscard]] const std::vector<TestEntry> &
        Tests() const noexcept
        {
            return tests_;
        }

        [[nodiscard]] const std::vector<Catch::TestCaseHandle> &
        Handles() const noexcept
        {
            return handles_;
        }

        /// The test of a key or of a name; null when there is none.
        [[nodiscard]] const TestEntry *
        Find(const std::string &keyOrName) const;

        /// A digest of every key in order. A worker reports it, so the runner
        /// knows the worker sees the same tests.
        [[nodiscard]] const std::string &
        Identity() const noexcept
        {
            return identity_;
        }

        /// The tests the specs of `config` select, in registration order, as
        /// Catch2 selects them: hidden tests only when a spec names them.
        /// Specs that matched nothing are added to `unmatched`.
        [[nodiscard]] std::vector<std::size_t>
        Match(const Catch::IConfig &config,
              std::vector<std::string> &unmatched) const;

    private:
        std::vector<TestEntry> tests_;
        std::vector<Catch::TestCaseHandle> handles_;
        std::string identity_;
    };
} // namespace Progmasoft::Catch3::Runner
