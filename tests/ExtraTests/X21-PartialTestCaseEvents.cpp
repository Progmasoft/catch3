
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

/**\file
 * Registers a listener that records the testCase* events and prints them
 * when the test run ends, outside any test, where the runner passes the
 * output on.
 *
 * The resulting executable can then be used by an external Python script
 * to verify that testCase{Starting,Ended} and testCasePartial{Starting,Ended}
 * events are properly nested.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <iostream>
#include <string>

using Catch::TestCaseInfo;
using Catch::TestCaseStats;

namespace {
class PartialListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;

    //! Called _once_ for each TEST_CASE, no matter how many times it is entered
    void testCaseStarting(TestCaseInfo const& testInfo) override {
        m_events += "TestCaseStarting: " + testInfo.name + '\n';
    }
    //! Called _every time_ a TEST_CASE is entered, including repeats (due to sections)
    void testCasePartialStarting(TestCaseInfo const& testInfo, uint64_t partNumber) override {
        m_events += "TestCaseStartingPartial: " + testInfo.name + '#' + std::to_string(partNumber) + '\n';
    }

    //! Called _every time_ a TEST_CASE is entered, including repeats (due to sections)
    void testCasePartialEnded(TestCaseStats const& testCaseStats, uint64_t partNumber) override {
        m_events += "TestCasePartialEnded: " + testCaseStats.testInfo->name + '#' + std::to_string(partNumber) + '\n';
    }

    //! Called _once_ for each TEST_CASE, no matter how many times it is entered
    void testCaseEnded(TestCaseStats const& testCaseStats) override {
        m_events += "TestCaseEnded: " + testCaseStats.testInfo->name + '\n';
    }

    void testRunEnded( Catch::TestRunStats const& ) override {
        std::cout << "BEGIN EVENTS\n" << m_events << "END EVENTS\n" << std::flush;
    }

private:
    std::string m_events;
};
} // namespace

CATCH_REGISTER_LISTENER(PartialListener)

TEST_CASE("section") {
    SECTION("A") {}
    SECTION("B") {}
    SECTION("C") {}
    SECTION("D") {}
}

TEST_CASE("generator") {
    auto _ = GENERATE(1, 2, 3, 4);
    (void)_;
}
