
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#ifndef CATCH_CONFIG_HPP_INCLUDED
#define CATCH_CONFIG_HPP_INCLUDED

#include <catch2/catch_test_spec.hpp>
#include <catch2/interfaces/catch_interfaces_config.hpp>
#include <catch2/internal/catch_unique_ptr.hpp>
#include <catch2/internal/catch_optional.hpp>
#include <catch2/internal/catch_path_filter.hpp>
#include <catch2/internal/catch_stringref.hpp>
#include <catch2/internal/catch_random_seed_generation.hpp>

#include <chrono>
#include <string>
#include <vector>

namespace Catch {

    /**
     * How the tests of one process behave.
     *
     * The Catch3 runner fills this in from its own command line, in the
     * process that runs the tests: a worker, or the runner itself with
     * `--in-process`. What to run, in which order and where to report is
     * the runner's, not part of this configuration.
     */
    struct ConfigData {
        bool showSuccessfulTests = false;
        bool shouldDebugBreak = false;
        bool noThrow = false;
        bool showInvisibles = false;
        bool filenamesAsTags = false;
        bool allowZeroTests = false;

        int abortAfter = -1;
        uint32_t rngSeed = generateRandomSeed(GenerateFrom::Default);
        bool rngSeedWasFixed = false;

        unsigned int shardCount = 1;
        unsigned int shardIndex = 0;

        bool skipBenchmarks = false;
        bool benchmarkNoAnalysis = false;
        unsigned int benchmarkSamples = 100;
        double benchmarkConfidenceInterval = 0.95;
        unsigned int benchmarkResamples = 100'000;
        std::chrono::milliseconds::rep benchmarkWarmupTime = 100;

        Verbosity verbosity = Verbosity::Normal;
        WarnAbout::What warnings = WarnAbout::Nothing;
        ShowDurations showDurations = ShowDurations::DefaultForReporter;
        double minDuration = -1;
        TestRunOrder runOrder = TestRunOrder::Declared;
        ColourMode defaultColourMode = ColourMode::PlatformDefault;

        std::string name;
        std::string processName;

        std::vector<std::string> testsOrTags;
        std::vector<PathFilter> pathFilters;
        bool useNewPathFilteringBehaviour = false;
    };


    class Config : public IConfig {
    public:

        Config() = default;
        Config( ConfigData const& data );
        ~Config() override; // = default in the cpp file

        std::vector<std::string> const& getTestsOrTags() const override;
        std::vector<PathFilter> const& getPathFilters() const override;
        bool useNewFilterBehaviour() const override;

        TestSpec const& testSpec() const override;
        bool hasTestFilters() const override;

        // IConfig interface
        bool allowThrows() const override;
        StringRef name() const override;
        bool includeSuccessfulResults() const override;
        bool warnAboutMissingAssertions() const override;
        bool warnAboutUnmatchedTestSpecs() const override;
        bool warnAboutInfiniteGenerators() const override;
        bool zeroTestsCountAsSuccess() const override;
        ShowDurations showDurations() const override;
        double minDuration() const override;
        TestRunOrder runOrder() const override;
        uint32_t rngSeed() const override;
        bool rngSeedWasFixed() const;
        unsigned int shardCount() const override;
        unsigned int shardIndex() const override;
        ColourMode defaultColourMode() const override;
        bool shouldDebugBreak() const override;
        int abortAfter() const override;
        bool showInvisibles() const override;
        Verbosity verbosity() const override;
        bool skipBenchmarks() const override;
        bool benchmarkNoAnalysis() const override;
        unsigned int benchmarkSamples() const override;
        double benchmarkConfidenceInterval() const override;
        unsigned int benchmarkResamples() const override;
        std::chrono::milliseconds benchmarkWarmupTime() const override;

    private:
        ConfigData m_data;
        TestSpec m_testSpec;
        bool m_hasTestFilters = false;
    };
} // end namespace Catch

#endif // CATCH_CONFIG_HPP_INCLUDED
