
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/AssertionInfo.hpp>
#include <Progmasoft/Catch3/Internal/ResultType.hpp>
#include <Progmasoft/Catch3/Internal/SourceLineInfo.hpp>
#include <Progmasoft/Catch3/Internal/StringRef.hpp>
#include <Progmasoft/Catch3/Internal/LazyExpr.hpp>

#include <string>

namespace Catch {

    struct AssertionResultData
    {
        AssertionResultData() = delete;

        AssertionResultData( ResultWas::OfType _resultType, LazyExpression const& _lazyExpression );

        std::string message;
        mutable std::string reconstructedExpression;
        LazyExpression lazyExpression;
        ResultWas::OfType resultType;

        std::string reconstructExpression() const;
    };

    class AssertionResult {
    public:
        AssertionResult() = delete;
        AssertionResult( AssertionInfo const& info, AssertionResultData&& data );

        bool isOk() const;
        bool succeeded() const;
        ResultWas::OfType getResultType() const;
        bool hasExpression() const;
        bool hasMessage() const;
        std::string getExpression() const;
        std::string getExpressionInMacro() const;
        bool hasExpandedExpression() const;
        std::string getExpandedExpression() const;
        StringRef getMessage() const;
        SourceLineInfo getSourceInfo() const;
        StringRef getTestMacroName() const;

    //protected:
        AssertionInfo m_info;
        AssertionResultData m_resultData;
    };

} // end namespace Catch

