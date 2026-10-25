
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <Progmasoft/Catch3/Generators/Generators.hpp>
#include <Progmasoft/Catch3/Interfaces/IResultCapture.hpp>

namespace Catch {

    IGeneratorTracker::~IGeneratorTracker() = default;

namespace Generators {

    GeneratorUntypedBase::~GeneratorUntypedBase() = default;

    IGeneratorTracker* acquireGeneratorTracker(StringRef generatorName, SourceLineInfo const& lineInfo ) {
        return getResultCapture().acquireGeneratorTracker( generatorName, lineInfo );
    }

    IGeneratorTracker* createGeneratorTracker( StringRef generatorName,
                                 SourceLineInfo lineInfo,
                                 GeneratorBasePtr&& generator ) {
        return getResultCapture().createGeneratorTracker(
            generatorName, lineInfo, CATCH_MOVE( generator ) );
    }

} // namespace Generators
} // namespace Catch
