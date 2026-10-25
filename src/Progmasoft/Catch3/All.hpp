
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
/** \file
 * This is a convenience header for Catch2. It includes **all** of Catch2 headers.
 *
 * Generally the Catch2 users should use specific includes they need,
 * but this header can be used instead for ease-of-experimentation, or
 * just plain convenience, at the cost of (significantly) increased
 * compilation times.
 *
 * When a new header is added to either the top level folder, or to the
 * corresponding internal subfolder, it should be added here. Headers
 * added to the various subparts (e.g. matchers, generators, etc...),
 * should go their respective catch-all headers.
 */

#pragma once

#include <Progmasoft/Catch3/Approx.hpp>
#include <Progmasoft/Catch3/AssertionInfo.hpp>
#include <Progmasoft/Catch3/AssertionResult.hpp>
#include <Progmasoft/Catch3/Benchmark/All.hpp>
#include <Progmasoft/Catch3/CaseSensitive.hpp>
#include <Progmasoft/Catch3/Config.hpp>
#include <Progmasoft/Catch3/EngineVersion.hpp>
#include <Progmasoft/Catch3/EngineVersionMacros.hpp>
#include <Progmasoft/Catch3/Generators/All.hpp>
#include <Progmasoft/Catch3/GetRandomSeed.hpp>
#include <Progmasoft/Catch3/Interfaces/All.hpp>
#include <Progmasoft/Catch3/Internal/AssertionHandler.hpp>
#include <Progmasoft/Catch3/Internal/CaseInsensitiveComparisons.hpp>
#include <Progmasoft/Catch3/Internal/CompareTraits.hpp>
#include <Progmasoft/Catch3/Internal/CompilerCapabilities.hpp>
#include <Progmasoft/Catch3/Internal/ConfigAndroidLogWrite.hpp>
#include <Progmasoft/Catch3/Internal/ConfigCounter.hpp>
#include <Progmasoft/Catch3/Internal/ConfigPrefixMessages.hpp>
#include <Progmasoft/Catch3/Internal/ConfigStaticAnalysisSupport.hpp>
#include <Progmasoft/Catch3/Internal/ConfigUncaughtExceptions.hpp>
#include <Progmasoft/Catch3/Internal/ConfigWchar.hpp>
#include <Progmasoft/Catch3/Internal/ContainerNonMembers.hpp>
#include <Progmasoft/Catch3/Internal/Context.hpp>
#include <Progmasoft/Catch3/Internal/Debugger.hpp>
#include <Progmasoft/Catch3/Internal/Decomposer.hpp>
#include <Progmasoft/Catch3/Internal/DeprecationMacro.hpp>
#include <Progmasoft/Catch3/Internal/Enforce.hpp>
#include <Progmasoft/Catch3/Internal/EnumInfo.hpp>
#include <Progmasoft/Catch3/Internal/ErrnoGuard.hpp>
#include <Progmasoft/Catch3/Internal/ExceptionTranslatorRegistry.hpp>
#include <Progmasoft/Catch3/Internal/FatalConditionHandler.hpp>
#include <Progmasoft/Catch3/Internal/FloatingPointHelpers.hpp>
#include <Progmasoft/Catch3/Internal/GetEnv.hpp>
#include <Progmasoft/Catch3/Internal/IsPermutation.hpp>
#include <Progmasoft/Catch3/Internal/LazyExpr.hpp>
#include <Progmasoft/Catch3/Internal/LeakDetector.hpp>
#include <Progmasoft/Catch3/Internal/LifetimeBound.hpp>
#include <Progmasoft/Catch3/Internal/LogicalTraits.hpp>
#include <Progmasoft/Catch3/Internal/MessageInfo.hpp>
#include <Progmasoft/Catch3/Internal/Meta.hpp>
#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>
#include <Progmasoft/Catch3/Internal/NonCopyable.hpp>
#include <Progmasoft/Catch3/Internal/Optional.hpp>
#include <Progmasoft/Catch3/Internal/OutputRedirect.hpp>
#include <Progmasoft/Catch3/Internal/ParseNumbers.hpp>
#include <Progmasoft/Catch3/Internal/PathFilter.hpp>
#include <Progmasoft/Catch3/Internal/Platform.hpp>
#include <Progmasoft/Catch3/Internal/Polyfills.hpp>
#include <Progmasoft/Catch3/Internal/Preprocessor.hpp>
#include <Progmasoft/Catch3/Internal/PreprocessorInternalStringify.hpp>
#include <Progmasoft/Catch3/Internal/PreprocessorRemoveParens.hpp>
#include <Progmasoft/Catch3/Internal/RandomFloatingPointHelpers.hpp>
#include <Progmasoft/Catch3/Internal/RandomIntegerHelpers.hpp>
#include <Progmasoft/Catch3/Internal/RandomNumberGenerator.hpp>
#include <Progmasoft/Catch3/Internal/RandomSeedGeneration.hpp>
#include <Progmasoft/Catch3/Internal/ReporterRegistry.hpp>
#include <Progmasoft/Catch3/Internal/ResultType.hpp>
#include <Progmasoft/Catch3/Internal/ReusableStringStream.hpp>
#include <Progmasoft/Catch3/Internal/RunContext.hpp>
#include <Progmasoft/Catch3/Internal/Section.hpp>
#include <Progmasoft/Catch3/Internal/Singletons.hpp>
#include <Progmasoft/Catch3/Internal/SourceLineInfo.hpp>
#include <Progmasoft/Catch3/Internal/StartupExceptionRegistry.hpp>
#include <Progmasoft/Catch3/Internal/StdStreams.hpp>
#include <Progmasoft/Catch3/Internal/StreamEndStop.hpp>
#include <Progmasoft/Catch3/Internal/StringManip.hpp>
#include <Progmasoft/Catch3/Internal/StringRef.hpp>
#include <Progmasoft/Catch3/Internal/TagAliasRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TemplateTestRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TestCaseInfoHasher.hpp>
#include <Progmasoft/Catch3/Internal/TestCaseRegistryImpl.hpp>
#include <Progmasoft/Catch3/Internal/TestCaseTracker.hpp>
#include <Progmasoft/Catch3/Internal/TestFailureException.hpp>
#include <Progmasoft/Catch3/Internal/TestMacroImpl.hpp>
#include <Progmasoft/Catch3/Internal/TestRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TestSpecParser.hpp>
#include <Progmasoft/Catch3/Internal/ThreadLocal.hpp>
#include <Progmasoft/Catch3/Internal/ThreadSupport.hpp>
#include <Progmasoft/Catch3/Internal/ToString.hpp>
#include <Progmasoft/Catch3/Internal/UncaughtExceptions.hpp>
#include <Progmasoft/Catch3/Internal/UniformFloatingPointDistribution.hpp>
#include <Progmasoft/Catch3/Internal/UniformIntegerDistribution.hpp>
#include <Progmasoft/Catch3/Internal/UniqueName.hpp>
#include <Progmasoft/Catch3/Internal/UniquePtr.hpp>
#include <Progmasoft/Catch3/Internal/Unreachable.hpp>
#include <Progmasoft/Catch3/Internal/VoidType.hpp>
#include <Progmasoft/Catch3/Internal/WildcardPattern.hpp>
#include <Progmasoft/Catch3/Matchers/All.hpp>
#include <Progmasoft/Catch3/Message.hpp>
#include <Progmasoft/Catch3/Reporters/All.hpp>
#include <Progmasoft/Catch3/SectionInfo.hpp>
#include <Progmasoft/Catch3/Session.hpp>
#include <Progmasoft/Catch3/TagAlias.hpp>
#include <Progmasoft/Catch3/TagAliasAutoregistrar.hpp>
#include <Progmasoft/Catch3/TemplateTestMacros.hpp>
#include <Progmasoft/Catch3/TestCaseInfo.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <Progmasoft/Catch3/TestRunInfo.hpp>
#include <Progmasoft/Catch3/TestSpec.hpp>
#include <Progmasoft/Catch3/Timer.hpp>
#include <Progmasoft/Catch3/ToString.hpp>
#include <Progmasoft/Catch3/Totals.hpp>
#include <Progmasoft/Catch3/TranslateException.hpp>

