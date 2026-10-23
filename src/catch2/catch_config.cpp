
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <catch2/catch_config.hpp>
#include <catch2/catch_user_config.hpp>
#include <catch2/internal/catch_string_manip.hpp>
#include <catch2/internal/catch_test_spec_parser.hpp>
#include <catch2/interfaces/catch_interfaces_tag_alias_registry.hpp>

namespace Catch {

    bool operator==( PathFilter const& lhs, PathFilter const& rhs ) {
        return lhs.type == rhs.type && lhs.filter == rhs.filter;
    }

    Config::Config( ConfigData const& data ):
        m_data( data ) {
        // We need to trim filter specs to avoid trouble with superfluous
        // whitespace (esp. important for bdd macros, as those are manually
        // aligned with whitespace).
        for (auto& elem : m_data.testsOrTags) {
            elem = trim(elem);
        }

        TestSpecParser parser( ITagAliasRegistry::get() );
        if ( !m_data.testsOrTags.empty() ) {
            m_hasTestFilters = true;
            for ( auto const& testOrTags : m_data.testsOrTags ) {
                parser.parse( testOrTags );
            }
        }
        m_testSpec = parser.testSpec();
    }

    Config::~Config() = default;

    std::vector<std::string> const& Config::getTestsOrTags() const { return m_data.testsOrTags; }
    std::vector<PathFilter> const& Config::getPathFilters() const { return m_data.pathFilters; }
    bool Config::useNewFilterBehaviour() const { return m_data.useNewPathFilteringBehaviour; }

    TestSpec const& Config::testSpec() const { return m_testSpec; }
    bool Config::hasTestFilters() const { return m_hasTestFilters; }

    // IConfig interface
    bool Config::allowThrows() const                   { return !m_data.noThrow; }
    StringRef Config::name() const { return m_data.name.empty() ? m_data.processName : m_data.name; }
    bool Config::includeSuccessfulResults() const      { return m_data.showSuccessfulTests; }
    bool Config::warnAboutMissingAssertions() const {
        return !!( m_data.warnings & WarnAbout::NoAssertions );
    }
    bool Config::warnAboutUnmatchedTestSpecs() const {
        return !!( m_data.warnings & WarnAbout::UnmatchedTestSpec );
    }
    bool Config::warnAboutInfiniteGenerators() const {
        return !!( m_data.warnings & WarnAbout::InfiniteGenerator );
    }
    bool Config::zeroTestsCountAsSuccess() const       { return m_data.allowZeroTests; }
    ShowDurations Config::showDurations() const        { return m_data.showDurations; }
    double Config::minDuration() const                 { return m_data.minDuration; }
    TestRunOrder Config::runOrder() const              { return m_data.runOrder; }
    uint32_t Config::rngSeed() const                   { return m_data.rngSeed; }
    bool Config::rngSeedWasFixed() const               { return m_data.rngSeedWasFixed; }
    unsigned int Config::shardCount() const            { return m_data.shardCount; }
    unsigned int Config::shardIndex() const            { return m_data.shardIndex; }
    ColourMode Config::defaultColourMode() const       { return m_data.defaultColourMode; }
    bool Config::shouldDebugBreak() const              { return m_data.shouldDebugBreak; }
    int Config::abortAfter() const                     { return m_data.abortAfter; }
    bool Config::showInvisibles() const                { return m_data.showInvisibles; }
    Verbosity Config::verbosity() const                { return m_data.verbosity; }

    bool Config::skipBenchmarks() const                           { return m_data.skipBenchmarks; }
    bool Config::benchmarkNoAnalysis() const                      { return m_data.benchmarkNoAnalysis; }
    unsigned int Config::benchmarkSamples() const                 { return m_data.benchmarkSamples; }
    double Config::benchmarkConfidenceInterval() const            { return m_data.benchmarkConfidenceInterval; }
    unsigned int Config::benchmarkResamples() const               { return m_data.benchmarkResamples; }
    std::chrono::milliseconds Config::benchmarkWarmupTime() const { return std::chrono::milliseconds(m_data.benchmarkWarmupTime); }

} // end namespace Catch
