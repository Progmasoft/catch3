
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <Progmasoft/Catch3/Interfaces/IEventListener.hpp>
#include <Progmasoft/Catch3/Interfaces/IConfig.hpp>
#include <Progmasoft/Catch3/Message.hpp>
#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>

#include <cassert>

namespace Catch {

    AssertionStats::AssertionStats( AssertionResult const& _assertionResult,
                                    std::vector<MessageInfo> const& _infoMessages,
                                    Totals const& _totals )
    :   assertionResult( _assertionResult ),
        infoMessages( _infoMessages ),
        totals( _totals )
    {
        if( assertionResult.hasMessage() ) {
            // Copy message into messages list.
            // !TBD This should have been done earlier, somewhere
            MessageBuilder builder( assertionResult.getTestMacroName(), assertionResult.getSourceInfo(), assertionResult.getResultType() );
            builder.m_info.message = static_cast<std::string>(assertionResult.getMessage());

            infoMessages.push_back( CATCH_MOVE(builder.m_info) );
        }
    }

    SectionStats::SectionStats(  SectionInfo&& _sectionInfo,
                                 Counts const& _assertions,
                                 double _durationInSeconds,
                                 bool _missingAssertions )
    :   sectionInfo( CATCH_MOVE(_sectionInfo) ),
        assertions( _assertions ),
        durationInSeconds( _durationInSeconds ),
        missingAssertions( _missingAssertions )
    {}


    TestCaseStats::TestCaseStats(  TestCaseInfo const& _testInfo,
                                   Totals const& _totals,
                                   std::string&& _stdOut,
                                   std::string&& _stdErr,
                                   bool _aborting )
    : testInfo( &_testInfo ),
        totals( _totals ),
        stdOut( CATCH_MOVE(_stdOut) ),
        stdErr( CATCH_MOVE(_stdErr) ),
        aborting( _aborting )
    {}


    TestRunStats::TestRunStats(   TestRunInfo const& _runInfo,
                    Totals const& _totals,
                    bool _aborting )
    :   runInfo( _runInfo ),
        totals( _totals ),
        aborting( _aborting )
    {}

    IEventListener::~IEventListener() = default;

} // end namespace Catch
