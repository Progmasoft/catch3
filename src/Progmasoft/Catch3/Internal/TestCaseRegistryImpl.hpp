
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Interfaces/ITestCaseRegistry.hpp>
#include <Progmasoft/Catch3/Interfaces/IConfig.hpp>
#include <Progmasoft/Catch3/Internal/UniquePtr.hpp>

#include <vector>

namespace Catch {

    class IConfig;
    class ITestInvoker;
    class TestCaseHandle;
    class TestSpec;

    std::vector<TestCaseHandle> sortTests( IConfig const& config, std::vector<TestCaseHandle> const& unsortedTestCases );

    bool isThrowSafe( TestCaseHandle const& testCase, IConfig const& config );

    std::vector<TestCaseHandle> filterTests( std::vector<TestCaseHandle> const& testCases, TestSpec const& testSpec, IConfig const& config );
    std::vector<TestCaseHandle> const& getAllTestCasesSorted( IConfig const& config );

    class TestRegistry final : public ITestCaseRegistry {
    public:
        void registerTest( Detail::unique_ptr<TestCaseInfo> testInfo, Detail::unique_ptr<ITestInvoker> testInvoker );
        void enableFilenameTags() override;

        std::vector<TestCaseHandle> const& getAllTests() const override;
        std::vector<TestCaseHandle> const& getAllTestsSorted( IConfig const& config ) const override;

        TestRegistry();
        ~TestRegistry() override; // = default

    private:
        // Owns the test infos for handles
        std::vector<Detail::unique_ptr<TestCaseInfo>> m_test_infos;
        // Owns the test invokers for handles
        std::vector<Detail::unique_ptr<ITestInvoker>> m_invokers;

        std::vector<TestCaseHandle> m_handles;
        mutable TestRunOrder m_currentSortOrder = TestRunOrder::Declared;
        mutable std::vector<TestCaseHandle> m_sortedFunctions;
    };

    ///////////////////////////////////////////////////////////////////////////


} // end namespace Catch


