include(CatchMiscFunctions)

# CMake derives a Visual Studio project GUID from the file path but can be overridden via a property
# (see https://gitlab.kitware.com/cmake/cmake/-/commit/c85367f4).  Using a non-constant GUID
# can cause problems if other projects/repos want to reference the vcxproj file,
# so we force a constant GUID here.
set(Catch3_GUID_CMAKE "e1270d21-1024-5593-b85a-e7fa93204357" CACHE INTERNAL "Project GUID")
set(Catch3WithMain_GUID_CMAKE "43cd2803-2f99-5aad-8802-81da0f3e776d" CACHE INTERNAL "Project GUID")

set(BENCHMARK_HEADERS
  ${SOURCES_DIR}/Benchmark/Benchmark.hpp
  ${SOURCES_DIR}/Benchmark/All.hpp
  ${SOURCES_DIR}/Benchmark/Chronometer.hpp
  ${SOURCES_DIR}/Benchmark/Clock.hpp
  ${SOURCES_DIR}/Benchmark/Constructor.hpp
  ${SOURCES_DIR}/Benchmark/Environment.hpp
  ${SOURCES_DIR}/Benchmark/Estimate.hpp
  ${SOURCES_DIR}/Benchmark/ExecutionPlan.hpp
  ${SOURCES_DIR}/Benchmark/Optimizer.hpp
  ${SOURCES_DIR}/Benchmark/OutlierClassification.hpp
  ${SOURCES_DIR}/Benchmark/SampleAnalysis.hpp
  ${SOURCES_DIR}/Benchmark/Detail/Analyse.hpp
  ${SOURCES_DIR}/Benchmark/Detail/BenchmarkFunction.hpp
  ${SOURCES_DIR}/Benchmark/Detail/BenchmarkStats.hpp
  ${SOURCES_DIR}/Benchmark/Detail/BenchmarkStatsFwd.hpp
  ${SOURCES_DIR}/Benchmark/Detail/CompleteInvoke.hpp
  ${SOURCES_DIR}/Benchmark/Detail/EstimateClock.hpp
  ${SOURCES_DIR}/Benchmark/Detail/Measure.hpp
  ${SOURCES_DIR}/Benchmark/Detail/Repeat.hpp
  ${SOURCES_DIR}/Benchmark/Detail/RunForAtLeast.hpp
  ${SOURCES_DIR}/Benchmark/Detail/Stats.hpp
  ${SOURCES_DIR}/Benchmark/Detail/Timing.hpp
)
set(BENCHMARK_SOURCES
  ${SOURCES_DIR}/Benchmark/Benchmark.cpp
  ${SOURCES_DIR}/Benchmark/Chronometer.cpp
  ${SOURCES_DIR}/Benchmark/Detail/Analyse.cpp
  ${SOURCES_DIR}/Benchmark/Detail/BenchmarkFunction.cpp
  ${SOURCES_DIR}/Benchmark/Detail/RunForAtLeast.cpp
  ${SOURCES_DIR}/Benchmark/Detail/Stats.cpp
)
set(BENCHMARK_FILES ${BENCHMARK_HEADERS} ${BENCHMARK_SOURCES})

set(IMPL_HEADERS
 "${PROJECT_BINARY_DIR}/generated-includes/Progmasoft/Catch3/UserConfig.hpp"
  ${SOURCES_DIR}/UserConfig.hpp.in
  ${SOURCES_DIR}/All.hpp
  ${SOURCES_DIR}/Approx.hpp
  ${SOURCES_DIR}/AssertionInfo.hpp
  ${SOURCES_DIR}/AssertionResult.hpp
  ${SOURCES_DIR}/CaseSensitive.hpp
  ${SOURCES_DIR}/Config.hpp
  ${SOURCES_DIR}/GetRandomSeed.hpp
  ${SOURCES_DIR}/Message.hpp
  ${SOURCES_DIR}/SectionInfo.hpp
  ${SOURCES_DIR}/Session.hpp
  ${SOURCES_DIR}/TagAlias.hpp
  ${SOURCES_DIR}/TagAliasAutoregistrar.hpp
  ${SOURCES_DIR}/TemplateTestMacros.hpp
  ${SOURCES_DIR}/TestCaseInfo.hpp
  ${SOURCES_DIR}/TestMacros.hpp
  ${SOURCES_DIR}/TestRunInfo.hpp
  ${SOURCES_DIR}/TestSpec.hpp
  ${SOURCES_DIR}/Timer.hpp
  ${SOURCES_DIR}/ToString.hpp
  ${SOURCES_DIR}/Totals.hpp
  ${SOURCES_DIR}/TranslateException.hpp
  ${SOURCES_DIR}/EngineVersion.hpp
  ${SOURCES_DIR}/EngineVersionMacros.hpp
  ${SOURCES_DIR}/Internal/AssertionHandler.hpp
  ${SOURCES_DIR}/Internal/CaseInsensitiveComparisons.hpp
  ${SOURCES_DIR}/Internal/CompareTraits.hpp
  ${SOURCES_DIR}/Internal/CompilerCapabilities.hpp
  ${SOURCES_DIR}/Internal/ConfigAndroidLogWrite.hpp
  ${SOURCES_DIR}/Internal/ConfigCounter.hpp
  ${SOURCES_DIR}/Internal/ConfigPrefixMessages.hpp
  ${SOURCES_DIR}/Internal/ConfigStaticAnalysisSupport.hpp
  ${SOURCES_DIR}/Internal/ConfigUncaughtExceptions.hpp
  ${SOURCES_DIR}/Internal/ConfigWchar.hpp
  ${SOURCES_DIR}/Internal/ContainerNonMembers.hpp
  ${SOURCES_DIR}/Internal/Context.hpp
  ${SOURCES_DIR}/Internal/Debugger.hpp
  ${SOURCES_DIR}/Internal/Decomposer.hpp
  ${SOURCES_DIR}/Internal/DeprecationMacro.hpp
  ${SOURCES_DIR}/Internal/Enforce.hpp
  ${SOURCES_DIR}/Internal/EnumInfo.hpp
  ${SOURCES_DIR}/Internal/ErrnoGuard.hpp
  ${SOURCES_DIR}/Internal/ExceptionTranslatorRegistry.hpp
  ${SOURCES_DIR}/Internal/FatalConditionHandler.hpp
  ${SOURCES_DIR}/Internal/FloatingPointHelpers.hpp
  ${SOURCES_DIR}/Internal/GetEnv.hpp
  ${SOURCES_DIR}/Internal/IsPermutation.hpp
  ${SOURCES_DIR}/Internal/LazyExpr.hpp
  ${SOURCES_DIR}/Internal/LeakDetector.hpp
  ${SOURCES_DIR}/Internal/LifetimeBound.hpp
  ${SOURCES_DIR}/Internal/LogicalTraits.hpp
  ${SOURCES_DIR}/Internal/MessageInfo.hpp
  ${SOURCES_DIR}/Internal/Meta.hpp
  ${SOURCES_DIR}/Internal/MoveAndForward.hpp
  ${SOURCES_DIR}/Internal/NonCopyable.hpp
  ${SOURCES_DIR}/Internal/Optional.hpp
  ${SOURCES_DIR}/Internal/OutputRedirect.hpp
  ${SOURCES_DIR}/Internal/ParseNumbers.hpp
  ${SOURCES_DIR}/Internal/PathFilter.hpp
  ${SOURCES_DIR}/Internal/Platform.hpp
  ${SOURCES_DIR}/Internal/Polyfills.hpp
  ${SOURCES_DIR}/Internal/Preprocessor.hpp
  ${SOURCES_DIR}/Internal/PreprocessorRemoveParens.hpp
  ${SOURCES_DIR}/Internal/RandomFloatingPointHelpers.hpp
  ${SOURCES_DIR}/Internal/RandomIntegerHelpers.hpp
  ${SOURCES_DIR}/Internal/RandomNumberGenerator.hpp
  ${SOURCES_DIR}/Internal/RandomSeedGeneration.hpp
  ${SOURCES_DIR}/Internal/ReporterRegistry.hpp
  ${SOURCES_DIR}/Internal/ResultType.hpp
  ${SOURCES_DIR}/Internal/ReusableStringStream.hpp
  ${SOURCES_DIR}/Internal/RunContext.hpp
  ${SOURCES_DIR}/Internal/Section.hpp
  ${SOURCES_DIR}/Internal/Singletons.hpp
  ${SOURCES_DIR}/Internal/SourceLineInfo.hpp
  ${SOURCES_DIR}/Internal/StartupExceptionRegistry.hpp
  ${SOURCES_DIR}/Internal/StdStreams.hpp
  ${SOURCES_DIR}/Internal/StreamEndStop.hpp
  ${SOURCES_DIR}/Internal/StringManip.hpp
  ${SOURCES_DIR}/Internal/StringRef.hpp
  ${SOURCES_DIR}/Internal/TagAliasRegistry.hpp
  ${SOURCES_DIR}/Internal/TemplateTestRegistry.hpp
  ${SOURCES_DIR}/Internal/TestCaseInfoHasher.hpp
  ${SOURCES_DIR}/Internal/TestCaseRegistryImpl.hpp
  ${SOURCES_DIR}/Internal/TestCaseTracker.hpp
  ${SOURCES_DIR}/Internal/TestFailureException.hpp
  ${SOURCES_DIR}/Internal/TestMacroImpl.hpp
  ${SOURCES_DIR}/Internal/TestRegistry.hpp
  ${SOURCES_DIR}/Internal/TestSpecParser.hpp
  ${SOURCES_DIR}/Internal/ThreadLocal.hpp
  ${SOURCES_DIR}/Internal/ThreadSupport.hpp
  ${SOURCES_DIR}/Internal/ToString.hpp
  ${SOURCES_DIR}/Internal/UncaughtExceptions.hpp
  ${SOURCES_DIR}/Internal/UniformFloatingPointDistribution.hpp
  ${SOURCES_DIR}/Internal/UniformIntegerDistribution.hpp
  ${SOURCES_DIR}/Internal/UniqueName.hpp
  ${SOURCES_DIR}/Internal/UniquePtr.hpp
  ${SOURCES_DIR}/Internal/Unreachable.hpp
  ${SOURCES_DIR}/Internal/VoidType.hpp
  ${SOURCES_DIR}/Internal/WildcardPattern.hpp
  ${SOURCES_DIR}/Internal/WindowsHProxy.hpp
)
set(IMPL_SOURCES
  ${SOURCES_DIR}/Approx.cpp
  ${SOURCES_DIR}/AssertionResult.cpp
  ${SOURCES_DIR}/Config.cpp
  ${SOURCES_DIR}/GetRandomSeed.cpp
  ${SOURCES_DIR}/Message.cpp
  ${SOURCES_DIR}/RegistryHub.cpp
  ${SOURCES_DIR}/Session.cpp
  ${SOURCES_DIR}/TagAliasAutoregistrar.cpp
  ${SOURCES_DIR}/TestCaseInfo.cpp
  ${SOURCES_DIR}/TestSpec.cpp
  ${SOURCES_DIR}/Timer.cpp
  ${SOURCES_DIR}/ToString.cpp
  ${SOURCES_DIR}/Totals.cpp
  ${SOURCES_DIR}/TranslateException.cpp
  ${SOURCES_DIR}/EngineVersion.cpp
  ${SOURCES_DIR}/Internal/AssertionHandler.cpp
  ${SOURCES_DIR}/Internal/CaseInsensitiveComparisons.cpp
  ${SOURCES_DIR}/Internal/Context.cpp
  ${SOURCES_DIR}/Internal/Debugger.cpp
  ${SOURCES_DIR}/Internal/Decomposer.cpp
  ${SOURCES_DIR}/Internal/Enforce.cpp
  ${SOURCES_DIR}/Internal/EnumInfo.cpp
  ${SOURCES_DIR}/Internal/ErrnoGuard.cpp
  ${SOURCES_DIR}/Internal/ExceptionTranslatorRegistry.cpp
  ${SOURCES_DIR}/Internal/FatalConditionHandler.cpp
  ${SOURCES_DIR}/Internal/FloatingPointHelpers.cpp
  ${SOURCES_DIR}/Internal/GetEnv.cpp
  ${SOURCES_DIR}/Internal/LazyExpr.cpp
  ${SOURCES_DIR}/Internal/LeakDetector.cpp
  ${SOURCES_DIR}/Internal/MessageInfo.cpp
  ${SOURCES_DIR}/Internal/OutputRedirect.cpp
  ${SOURCES_DIR}/Internal/ParseNumbers.cpp
  ${SOURCES_DIR}/Internal/Polyfills.cpp
  ${SOURCES_DIR}/Internal/RandomNumberGenerator.cpp
  ${SOURCES_DIR}/Internal/RandomSeedGeneration.cpp
  ${SOURCES_DIR}/Internal/ReporterRegistry.cpp
  ${SOURCES_DIR}/Internal/ReusableStringStream.cpp
  ${SOURCES_DIR}/Internal/RunContext.cpp
  ${SOURCES_DIR}/Internal/Section.cpp
  ${SOURCES_DIR}/Internal/Singletons.cpp
  ${SOURCES_DIR}/Internal/SourceLineInfo.cpp
  ${SOURCES_DIR}/Internal/StartupExceptionRegistry.cpp
  ${SOURCES_DIR}/Internal/StdStreams.cpp
  ${SOURCES_DIR}/Internal/StringManip.cpp
  ${SOURCES_DIR}/Internal/StringRef.cpp
  ${SOURCES_DIR}/Internal/TagAliasRegistry.cpp
  ${SOURCES_DIR}/Internal/TestCaseInfoHasher.cpp
  ${SOURCES_DIR}/Internal/TestCaseRegistryImpl.cpp
  ${SOURCES_DIR}/Internal/TestCaseTracker.cpp
  ${SOURCES_DIR}/Internal/TestFailureException.cpp
  ${SOURCES_DIR}/Internal/TestRegistry.cpp
  ${SOURCES_DIR}/Internal/TestSpecParser.cpp
  ${SOURCES_DIR}/Internal/UncaughtExceptions.cpp
  ${SOURCES_DIR}/Internal/WildcardPattern.cpp
)
set(INTERNAL_FILES ${IMPL_SOURCES} ${IMPL_HEADERS})

set(INTERFACE_HEADERS
  ${SOURCES_DIR}/Interfaces/All.hpp
  ${SOURCES_DIR}/Interfaces/IResultCapture.hpp
  ${SOURCES_DIR}/Interfaces/IConfig.hpp
  ${SOURCES_DIR}/Interfaces/IExceptionTranslator.hpp
  ${SOURCES_DIR}/Interfaces/IGeneratorTracker.hpp
  ${SOURCES_DIR}/Interfaces/IRegistryHub.hpp
  ${SOURCES_DIR}/Interfaces/IEventListener.hpp
  ${SOURCES_DIR}/Interfaces/EventListenerFactory.hpp
  ${SOURCES_DIR}/Interfaces/ITagAliasRegistry.hpp
  ${SOURCES_DIR}/Interfaces/ITestInvoker.hpp
  ${SOURCES_DIR}/Interfaces/ITestCaseRegistry.hpp
)
set(INTERFACE_SOURCES
  ${SOURCES_DIR}/Interfaces/IResultCapture.cpp
  ${SOURCES_DIR}/Interfaces/IConfig.cpp
  ${SOURCES_DIR}/Interfaces/IExceptionTranslator.cpp
  ${SOURCES_DIR}/Interfaces/IGeneratorTracker.cpp
  ${SOURCES_DIR}/Interfaces/IRegistryHub.cpp
  ${SOURCES_DIR}/Interfaces/IEventListener.cpp
  ${SOURCES_DIR}/Interfaces/EventListenerFactory.cpp
  ${SOURCES_DIR}/Interfaces/ITestCaseRegistry.cpp
)
set(INTERFACE_FILES ${INTERFACE_HEADERS} ${INTERFACE_SOURCES})

set(GENERATOR_HEADERS
  ${SOURCES_DIR}/Generators/GeneratorException.hpp
  ${SOURCES_DIR}/Generators/Generators.hpp
  ${SOURCES_DIR}/Generators/Adapters.hpp
  ${SOURCES_DIR}/Generators/All.hpp
  ${SOURCES_DIR}/Generators/Random.hpp
  ${SOURCES_DIR}/Generators/Range.hpp
  ${SOURCES_DIR}/Generators/Throw.hpp
)
set(GENERATOR_SOURCES
  ${SOURCES_DIR}/Generators/GeneratorException.cpp
  ${SOURCES_DIR}/Generators/Generators.cpp
  ${SOURCES_DIR}/Generators/Random.cpp
  ${SOURCES_DIR}/Generators/Throw.cpp
)
set(GENERATOR_FILES ${GENERATOR_HEADERS} ${GENERATOR_SOURCES})

set(MATCHER_HEADERS
  ${SOURCES_DIR}/Matchers/Matchers.hpp
  ${SOURCES_DIR}/Matchers/All.hpp
  ${SOURCES_DIR}/Matchers/ContainerProperties.hpp
  ${SOURCES_DIR}/Matchers/Contains.hpp
  ${SOURCES_DIR}/Matchers/RangeEquals.hpp
  ${SOURCES_DIR}/Matchers/Exception.hpp
  ${SOURCES_DIR}/Matchers/FloatingPoint.hpp
  ${SOURCES_DIR}/Matchers/Predicate.hpp
  ${SOURCES_DIR}/Matchers/Quantifiers.hpp
  ${SOURCES_DIR}/Matchers/String.hpp
  ${SOURCES_DIR}/Matchers/Templated.hpp
  ${SOURCES_DIR}/Matchers/Vector.hpp
  ${SOURCES_DIR}/Matchers/Internal/Impl.hpp
)
set(MATCHER_SOURCES
  ${SOURCES_DIR}/Matchers/Matchers.cpp
  ${SOURCES_DIR}/Matchers/ContainerProperties.cpp
  ${SOURCES_DIR}/Matchers/Exception.cpp
  ${SOURCES_DIR}/Matchers/FloatingPoint.cpp
  ${SOURCES_DIR}/Matchers/Predicate.cpp
  ${SOURCES_DIR}/Matchers/Quantifiers.cpp
  ${SOURCES_DIR}/Matchers/String.cpp
  ${SOURCES_DIR}/Matchers/Templated.cpp
  ${SOURCES_DIR}/Matchers/Internal/Impl.cpp
)
set(MATCHER_FILES ${MATCHER_HEADERS} ${MATCHER_SOURCES})

set(REPORTER_HEADERS
  ${SOURCES_DIR}/Reporters/EventListener.hpp
  ${SOURCES_DIR}/Reporters/Multi.hpp
  ${SOURCES_DIR}/Reporters/Registrars.hpp
  ${SOURCES_DIR}/Reporters/All.hpp
)
set(REPORTER_SOURCES
  ${SOURCES_DIR}/Reporters/EventListener.cpp
  ${SOURCES_DIR}/Reporters/Multi.cpp
  ${SOURCES_DIR}/Reporters/Registrars.cpp
)
set(REPORTER_FILES ${REPORTER_HEADERS} ${REPORTER_SOURCES})

# The Catch3 runner, which replaced Catch2's session, command line and
# reporters. It is built into the library so that Catch::Session reaches it.
set(PROGMASOFT_RUNNER_FILES
  "${CATCH_DIR}/src/Progmasoft/Catch3/Results.hpp"
  "${CATCH_DIR}/src/Progmasoft/Catch3/Runner.hpp"
  "${CATCH_DIR}/src/Progmasoft/Catch3/Version.hpp"
  "${CATCH_DIR}/src/Progmasoft/Catch3/XmlWriter.hpp"
  "${CATCH_DIR}/src/Progmasoft/Results.cpp"
  "${CATCH_DIR}/src/Progmasoft/XmlWriter.cpp"
  "${CATCH_DIR}/src/Progmasoft/PugiXml.inl"
  "${CATCH_DIR}/src/Progmasoft/Runner/Analysis.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Catalog.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Console.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Execution.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Expected.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Files.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Git.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Hash.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/History.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Json.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Options.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Planner.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Process.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/ProjectConfig.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Protocol.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/ResultsJson.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Scheduler.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/SchedulerModel.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/TemporaryDirectory.hpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Analysis.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Catalog.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Console.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Execution.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Files.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Git.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Hash.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/History.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Json.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Yyjson.inl"
  "${CATCH_DIR}/src/Progmasoft/Runner/Options.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Planner.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Process.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/ProjectConfig.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Protocol.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/ResultsJson.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Runner.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Scheduler.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/TemporaryDirectory.cpp"
)

# Property testing and snapshots.
set(PROGMASOFT_LIBRARY_FILES
  "${CATCH_DIR}/src/Progmasoft/ChoiceShrinker.cpp"
  "${CATCH_DIR}/src/Progmasoft/StrategyContext.cpp"
  "${CATCH_DIR}/src/Progmasoft/PropertyEngine.cpp"
  "${CATCH_DIR}/src/Progmasoft/Snapshot.cpp"
)

set(ALL_FILES
  ${BENCHMARK_FILES}
  ${GENERATOR_FILES}
  ${REPORTER_FILES}
  ${INTERFACE_FILES}
  ${INTERNAL_FILES}
  ${MATCHER_FILES}
  ${PROGMASOFT_RUNNER_FILES}
  ${PROGMASOFT_LIBRARY_FILES}
)

set(FILTERED_FILES ${ALL_FILES})
list(REMOVE_ITEM FILTERED_FILES "${PROJECT_BINARY_DIR}/generated-includes/Progmasoft/Catch3/UserConfig.hpp")
list(REMOVE_ITEM FILTERED_FILES ${PROGMASOFT_RUNNER_FILES} ${PROGMASOFT_LIBRARY_FILES})
source_group(
  TREE ${SOURCES_DIR}
  PREFIX sources
  FILES ${FILTERED_FILES}
)
source_group(
  TREE "${CATCH_DIR}/src/Progmasoft"
  PREFIX Progmasoft
  FILES ${PROGMASOFT_RUNNER_FILES} ${PROGMASOFT_LIBRARY_FILES}
)
source_group("generated headers"
  FILES
    "${PROJECT_BINARY_DIR}/generated-includes/Progmasoft/Catch3/UserConfig.hpp"
)

add_library(Catch3 ${ALL_FILES})
if(CATCH3_ENABLE_REPRODUCIBLE_BUILD)
  add_build_reproducibility_settings(Catch3)
endif()
add_library(Catch3::Catch3 ALIAS Catch3)

if(ANDROID)
  target_link_libraries(Catch3 PRIVATE log)
endif()

# The JSON and XML libraries of the runner are compiled into its own
# translation units, so only the build needs them.
target_link_libraries(Catch3
  PRIVATE
    $<BUILD_INTERFACE:Catch3Yyjson>
    $<BUILD_INTERFACE:Catch3Pugixml>
)

set_target_properties(Catch3 PROPERTIES
  DEBUG_POSTFIX "d"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION}
)

# Code that includes the engine's headers needs C++14, and the other
# Progmasoft/ headers C++20; the library itself, with the runner, is built as C++20.
target_compile_features(Catch3 PUBLIC cxx_std_14)
set_target_properties(Catch3 PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

configure_file(
  "${SOURCES_DIR}/UserConfig.hpp.in"
  "${PROJECT_BINARY_DIR}/generated-includes/Progmasoft/Catch3/UserConfig.hpp"
)

target_include_directories(Catch3
  PUBLIC
    $<BUILD_INTERFACE:${CATCH_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated-includes>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


add_library(Catch3WithMain
  ${SOURCES_DIR}/Internal/Main.cpp
)
if(CATCH3_ENABLE_REPRODUCIBLE_BUILD)
  add_build_reproducibility_settings(Catch3WithMain)
endif()
add_library(Catch3::Catch3WithMain ALIAS Catch3WithMain)
target_link_libraries(Catch3WithMain PUBLIC Catch3)
set_target_properties(Catch3WithMain
  PROPERTIES
    OUTPUT_NAME "Catch3Main"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
)
set_target_properties(Catch3WithMain PROPERTIES DEBUG_POSTFIX "d")

if(NOT_SUBPROJECT)
  # Create and install an export set for the targets as Catch3::Catch3 and
  # Catch3::Catch3WithMain.
  install(
    TARGETS
      Catch3
      Catch3WithMain
    EXPORT
      Catch3Targets
    LIBRARY DESTINATION
      ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION
      ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION
      ${CMAKE_INSTALL_BINDIR}
  )

  install(
    EXPORT
      Catch3Targets
    NAMESPACE
      Catch3::
    DESTINATION
      ${CATCH3_CMAKE_CONFIG_DESTINATION}
  )
  # Install the headers, with the generated one, below include/Progmasoft.
  install(
    DIRECTORY
      "${CATCH_DIR}/src/Progmasoft"
      "${PROJECT_BINARY_DIR}/generated-includes/Progmasoft"
    DESTINATION
      "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING
      PATTERN "*.hpp"
  )
endif()

# Some tests require a full recompilation of the library with different
# compilation flags. They can link against this target to recompile all
# the sources into the binary.
if(CATCH3_BUILD_EXAMPLES OR CATCH3_BUILD_EXTRA_TESTS)
  add_library(Catch3_buildall_interface INTERFACE)
  target_sources(Catch3_buildall_interface INTERFACE
    ${ALL_FILES}
    # Also include main entry point
    ${SOURCES_DIR}/Internal/Main.cpp
  )
  target_include_directories(Catch3_buildall_interface
    INTERFACE
      $<BUILD_INTERFACE:${CATCH_DIR}/src>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated-includes>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_compile_definitions(Catch3_buildall_interface INTERFACE CATCH_CONFIG_STATIC)
  target_link_libraries(Catch3_buildall_interface
    INTERFACE
      Catch3Yyjson
      Catch3Pugixml
  )
  # These targets compile the library's sources themselves, the runner's
  # among them, which need C++20.
  target_compile_features(Catch3_buildall_interface INTERFACE cxx_std_20)
endif()

list(APPEND CATCH_IMPL_TARGETS Catch3 Catch3WithMain)
set(CATCH_IMPL_TARGETS ${CATCH_IMPL_TARGETS} PARENT_SCOPE)

# We still do not support building dynamic library with hidden visibility
# so we want to check & warn users if they do this. However, we won't abort
# the configuration step so that we don't have to also provide an override.
if(BUILD_SHARED_LIBS)
  set_target_properties(Catch3 Catch3WithMain
    PROPERTIES
      WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

  get_target_property(_VisPreset Catch3 CXX_VISIBILITY_PRESET)
  if(NOT MSVC AND _VisPreset STREQUAL "hidden")
    set_target_properties(Catch3 Catch3WithMain
      PROPERTIES
        CXX_VISIBILITY_PRESET "default"
        VISIBILITY_INLINES_HIDDEN OFF
    )
    message(WARNING "Setting Catch3's visibility to default."
                    " Hidden visibility is not supported.")
  endif()
endif()
