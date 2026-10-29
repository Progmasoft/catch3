#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0
pkg = import('pkgconfig')

conf_data = configuration_data()

configure_file(
  input: 'UserConfig.hpp.in',
  output: 'UserConfig.hpp',
  format: 'cmake@',
  install_dir: get_option('includedir') / 'Progmasoft' / 'Catch3',
  configuration: conf_data,
  install: get_option('install')
)

fs = import('fs')

benchmark_headers = [
  'Benchmark/Benchmark.hpp',
  'Benchmark/All.hpp',
  'Benchmark/Chronometer.hpp',
  'Benchmark/Clock.hpp',
  'Benchmark/Constructor.hpp',
  'Benchmark/Environment.hpp',
  'Benchmark/Estimate.hpp',
  'Benchmark/ExecutionPlan.hpp',
  'Benchmark/Optimizer.hpp',
  'Benchmark/OutlierClassification.hpp',
  'Benchmark/SampleAnalysis.hpp',
  'Benchmark/Detail/Analyse.hpp',
  'Benchmark/Detail/BenchmarkFunction.hpp',
  'Benchmark/Detail/BenchmarkStats.hpp',
  'Benchmark/Detail/BenchmarkStatsFwd.hpp',
  'Benchmark/Detail/CompleteInvoke.hpp',
  'Benchmark/Detail/EstimateClock.hpp',
  'Benchmark/Detail/Measure.hpp',
  'Benchmark/Detail/Repeat.hpp',
  'Benchmark/Detail/RunForAtLeast.hpp',
  'Benchmark/Detail/Stats.hpp',
  'Benchmark/Detail/Timing.hpp',
]

benchmark_sources = files(
  'Benchmark/Benchmark.cpp',
  'Benchmark/Chronometer.cpp',
  'Benchmark/Detail/Analyse.cpp',
  'Benchmark/Detail/BenchmarkFunction.cpp',
  'Benchmark/Detail/RunForAtLeast.cpp',
  'Benchmark/Detail/Stats.cpp',
)

internal_headers = [
  'Generators/GeneratorException.hpp',
  'Generators/Generators.hpp',
  'Generators/Adapters.hpp',
  'Generators/All.hpp',
  'Generators/Random.hpp',
  'Generators/Range.hpp',
  'Generators/Throw.hpp',
  'Interfaces/All.hpp',
  'Interfaces/IResultCapture.hpp',
  'Interfaces/IConfig.hpp',
  'Interfaces/IExceptionTranslator.hpp',
  'Interfaces/IGeneratorTracker.hpp',
  'Interfaces/IRegistryHub.hpp',
  'Interfaces/IEventListener.hpp',
  'Interfaces/EventListenerFactory.hpp',
  'Interfaces/ITagAliasRegistry.hpp',
  'Interfaces/ITestInvoker.hpp',
  'Interfaces/ITestCaseRegistry.hpp',
  'Internal/AssertionHandler.hpp',
  'Internal/CaseInsensitiveComparisons.hpp',
  'Internal/CompareTraits.hpp',
  'Internal/CompilerCapabilities.hpp',
  'Internal/ConfigAndroidLogWrite.hpp',
  'Internal/ConfigCounter.hpp',
  'Internal/ConfigPrefixMessages.hpp',
  'Internal/ConfigStaticAnalysisSupport.hpp',
  'Internal/ConfigUncaughtExceptions.hpp',
  'Internal/ConfigWchar.hpp',
  'Internal/ContainerNonMembers.hpp',
  'Internal/Context.hpp',
  'Internal/Debugger.hpp',
  'Internal/Decomposer.hpp',
  'Internal/DeprecationMacro.hpp',
  'Internal/Enforce.hpp',
  'Internal/EnumInfo.hpp',
  'Internal/ErrnoGuard.hpp',
  'Internal/ExceptionTranslatorRegistry.hpp',
  'Internal/FatalConditionHandler.hpp',
  'Internal/FloatingPointHelpers.hpp',
  'Internal/GetEnv.hpp',
  'Internal/IsPermutation.hpp',
  'Internal/LazyExpr.hpp',
  'Internal/LeakDetector.hpp',
  'Internal/LifetimeBound.hpp',
  'Internal/LogicalTraits.hpp',
  'Internal/MessageInfo.hpp',
  'Internal/Meta.hpp',
  'Internal/MoveAndForward.hpp',
  'Internal/NonCopyable.hpp',
  'Internal/Optional.hpp',
  'Internal/OutputRedirect.hpp',
  'Internal/ParseNumbers.hpp',
  'Internal/PathFilter.hpp',
  'Internal/Platform.hpp',
  'Internal/Polyfills.hpp',
  'Internal/Preprocessor.hpp',
  'Internal/PreprocessorInternalStringify.hpp',
  'Internal/PreprocessorRemoveParens.hpp',
  'Internal/RandomFloatingPointHelpers.hpp',
  'Internal/RandomIntegerHelpers.hpp',
  'Internal/RandomNumberGenerator.hpp',
  'Internal/RandomSeedGeneration.hpp',
  'Internal/ReporterRegistry.hpp',
  'Internal/ResultType.hpp',
  'Internal/ReusableStringStream.hpp',
  'Internal/RunContext.hpp',
  'Internal/Section.hpp',
  'Internal/Singletons.hpp',
  'Internal/SourceLineInfo.hpp',
  'Internal/StartupExceptionRegistry.hpp',
  'Internal/StdStreams.hpp',
  'Internal/StreamEndStop.hpp',
  'Internal/StringManip.hpp',
  'Internal/StringRef.hpp',
  'Internal/TagAliasRegistry.hpp',
  'Internal/TemplateTestRegistry.hpp',
  'Internal/TestCaseInfoHasher.hpp',
  'Internal/TestCaseRegistryImpl.hpp',
  'Internal/TestCaseTracker.hpp',
  'Internal/TestFailureException.hpp',
  'Internal/TestMacroImpl.hpp',
  'Internal/TestRegistry.hpp',
  'Internal/TestSpecParser.hpp',
  'Internal/ThreadLocal.hpp',
  'Internal/ThreadSupport.hpp',
  'Internal/ToString.hpp',
  'Internal/UncaughtExceptions.hpp',
  'Internal/UniformFloatingPointDistribution.hpp',
  'Internal/UniformIntegerDistribution.hpp',
  'Internal/UniqueName.hpp',
  'Internal/UniquePtr.hpp',
  'Internal/Unreachable.hpp',
  'Internal/VoidType.hpp',
  'Internal/WildcardPattern.hpp',
  'Internal/WindowsHProxy.hpp',
  'Matchers/Matchers.hpp',
  'Matchers/All.hpp',
  'Matchers/ContainerProperties.hpp',
  'Matchers/Contains.hpp',
  'Matchers/Exception.hpp',
  'Matchers/FloatingPoint.hpp',
  'Matchers/Predicate.hpp',
  'Matchers/Quantifiers.hpp',
  'Matchers/RangeEquals.hpp',
  'Matchers/String.hpp',
  'Matchers/Templated.hpp',
  'Matchers/Vector.hpp',
  'Matchers/Internal/Impl.hpp',
  'All.hpp',
  'Approx.hpp',
  'AssertionInfo.hpp',
  'AssertionResult.hpp',
  'CaseSensitive.hpp',
  'Config.hpp',
  'GetRandomSeed.hpp',
  'Message.hpp',
  'SectionInfo.hpp',
  'Session.hpp',
  'TagAlias.hpp',
  'TagAliasAutoregistrar.hpp',
  'TemplateTestMacros.hpp',
  'TestCaseInfo.hpp',
  'TestMacros.hpp',
  'TestRunInfo.hpp',
  'TestSpec.hpp',
  'Timer.hpp',
  'ToString.hpp',
  'Totals.hpp',
  'TranslateException.hpp',
  'EngineVersion.hpp',
  'EngineVersionMacros.hpp',
]

internal_sources = files(
  'Generators/GeneratorException.cpp',
  'Generators/Generators.cpp',
  'Generators/Random.cpp',
  'Generators/Throw.cpp',
  'Interfaces/IResultCapture.cpp',
  'Interfaces/IConfig.cpp',
  'Interfaces/IExceptionTranslator.cpp',
  'Interfaces/IGeneratorTracker.cpp',
  'Interfaces/IRegistryHub.cpp',
  'Interfaces/IEventListener.cpp',
  'Interfaces/EventListenerFactory.cpp',
  'Interfaces/ITestCaseRegistry.cpp',
  'Internal/AssertionHandler.cpp',
  'Internal/CaseInsensitiveComparisons.cpp',
  'Internal/Context.cpp',
  'Internal/Debugger.cpp',
  'Internal/Decomposer.cpp',
  'Internal/Enforce.cpp',
  'Internal/EnumInfo.cpp',
  'Internal/ErrnoGuard.cpp',
  'Internal/ExceptionTranslatorRegistry.cpp',
  'Internal/FatalConditionHandler.cpp',
  'Internal/FloatingPointHelpers.cpp',
  'Internal/GetEnv.cpp',
  'Internal/LazyExpr.cpp',
  'Internal/LeakDetector.cpp',
  'Internal/MessageInfo.cpp',
  'Internal/OutputRedirect.cpp',
  'Internal/ParseNumbers.cpp',
  'Internal/Polyfills.cpp',
  'Internal/RandomNumberGenerator.cpp',
  'Internal/RandomSeedGeneration.cpp',
  'Internal/ReporterRegistry.cpp',
  'Internal/ReusableStringStream.cpp',
  'Internal/RunContext.cpp',
  'Internal/Section.cpp',
  'Internal/Singletons.cpp',
  'Internal/SourceLineInfo.cpp',
  'Internal/StartupExceptionRegistry.cpp',
  'Internal/StdStreams.cpp',
  'Internal/StringManip.cpp',
  'Internal/StringRef.cpp',
  'Internal/TagAliasRegistry.cpp',
  'Internal/TestCaseInfoHasher.cpp',
  'Internal/TestCaseRegistryImpl.cpp',
  'Internal/TestCaseTracker.cpp',
  'Internal/TestFailureException.cpp',
  'Internal/TestRegistry.cpp',
  'Internal/TestSpecParser.cpp',
  'Internal/UncaughtExceptions.cpp',
  'Internal/WildcardPattern.cpp',
  'Matchers/Matchers.cpp',
  'Matchers/ContainerProperties.cpp',
  'Matchers/Exception.cpp',
  'Matchers/FloatingPoint.cpp',
  'Matchers/Predicate.cpp',
  'Matchers/Quantifiers.cpp',
  'Matchers/String.cpp',
  'Matchers/Templated.cpp',
  'Matchers/Internal/Impl.cpp',
  'Approx.cpp',
  'AssertionResult.cpp',
  'Config.cpp',
  'GetRandomSeed.cpp',
  'Message.cpp',
  'RegistryHub.cpp',
  'Session.cpp',
  'TagAliasAutoregistrar.cpp',
  'TestCaseInfo.cpp',
  'TestSpec.cpp',
  'Timer.cpp',
  'ToString.cpp',
  'Totals.cpp',
  'TranslateException.cpp',
  'EngineVersion.cpp',
)

reporter_headers = [
  'Reporters/EventListener.hpp',
  'Reporters/Multi.hpp',
  'Reporters/Registrars.hpp',
  'Reporters/All.hpp',
]

reporter_sources = files(
  'Reporters/EventListener.cpp',
  'Reporters/Multi.cpp',
  'Reporters/Registrars.cpp',
)

# The Catch3 runner, which replaced Catch2's session, command line and
# reporters.
runner_sources = files(
  '../Results.cpp',
  '../XmlWriter.cpp',
  '../Runner/Analysis.cpp',
  '../Runner/Catalog.cpp',
  '../Runner/Console.cpp',
  '../Runner/Execution.cpp',
  '../Runner/Files.cpp',
  '../Runner/Git.cpp',
  '../Runner/Hash.cpp',
  '../Runner/History.cpp',
  '../Runner/Json.cpp',
  '../Runner/Options.cpp',
  '../Runner/Planner.cpp',
  '../Runner/Process.cpp',
  '../Runner/ProjectConfig.cpp',
  '../Runner/Protocol.cpp',
  '../Runner/ResultsJson.cpp',
  '../Runner/Runner.cpp',
  '../Runner/Scheduler.cpp',
  '../Runner/TemporaryDirectory.cpp',
)

# Property testing and snapshots.
property_sources = files(
  '../ChoiceShrinker.cpp',
  '../StrategyContext.cpp',
  '../PropertyEngine.cpp',
  '../Snapshot.cpp',
)

headers = benchmark_headers + internal_headers + reporter_headers
sources = benchmark_sources + internal_sources + reporter_sources + runner_sources + property_sources

# The headers must be installed with their full paths, which meson
# v0.63 supports using `preserve_path` for `install_headers`. We'd
# like to be compatible with Debian 11 (current stable) which has
# meson v0.56.2. Instead, let's use the technique from
# https://github.com/mesonbuild/meson/issues/14 with some tweaks to
# make it compatible with newer meson.
include_subdir = 'Progmasoft' / 'Catch3'
foreach file : headers
  file_path = file.split('/')

  folder = ''
  foreach path : file_path
    if path != file_path[-1]
      folder = folder / path
    endif
  endforeach

  if get_option('install')
    install_headers(file, subdir: join_paths(include_subdir, folder))
  endif
endforeach

# The JSON and XML libraries of the runner are compiled into its own
# translation units, so only the build needs them.
catch3_dependencies = [yyjson_dep, pugixml_dep]
# Check if this is an Android NDK build.
if ((host_machine.system() == 'android') or
  # Check if this is an Android Termux build.
  (host_machine.system() == 'linux' and fs.is_dir('/data/data/com.termux')))
  log_dep = meson.get_compiler('cpp').find_library('log')
  catch3_dependencies += log_dep
endif

# Code that includes the engine's headers needs C++14, and the other
# Progmasoft/ headers C++20; the library itself, with the runner, is built as C++20.
catch3 = static_library(
  'Catch3',
  sources,
  dependencies: catch3_dependencies,
  include_directories: '../..',
  override_options: ['cpp_std=c++20'],
  install: get_option('install'),
)

catch3_dep = declare_dependency(
  link_with: catch3,
  include_directories: '../..',
)
meson.override_dependency('catch3', catch3_dep)

if get_option('install')
  pkg.generate(
    catch3,
    filebase: 'catch3',
    description: 'A C++ test framework with a Catch2-compatible API; headers need C++14 and above',
    url: 'https://github.com/Progmasoft/catch3',
  )
endif

catch3_with_main = static_library(
  'Catch3Main',
  'Internal/Main.cpp',
  link_with: catch3,
  include_directories: '../..',
  install: get_option('install'),
)

catch3_with_main_dep = declare_dependency(
  link_with: [catch3, catch3_with_main],
  include_directories: '../..',
)
meson.override_dependency('catch3-with-main', catch3_with_main_dep)

if get_option('install')
  pkg.generate(
    catch3_with_main,
    filebase: 'catch3-with-main',
    description: 'Catch3 with its default main',
    requires: 'catch3 = ' + meson.project_version(),
  )
endif
