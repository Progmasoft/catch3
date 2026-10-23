include(CatchMiscFunctions)

if(CATCH_BUILD_SURROGATES)
  message(STATUS "Configuring targets for surrogate TUs")

  # If the folder does not exist before we ask for output redirect to
  # a file, it won't work.
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/surrogates)

  # Creates target to generate the surrogate TU for provided header.
  # Returns the path to the generated file.
  function(createSurrogateFileTarget sourceHeader pathToFile)
    set(pathPrefix ${PROJECT_SOURCE_DIR}/src)
    file(RELATIVE_PATH includePath ${pathPrefix} ${sourceHeader})

    get_filename_component(basicFileName "${sourceHeader}" NAME_WE)
    set(surrogateFilePath ${CMAKE_CURRENT_BINARY_DIR}/surrogates/surrogate_${basicFileName}.cpp)

    add_custom_command(
      OUTPUT ${surrogateFilePath}
      COMMAND cmake -E echo "\#include <${includePath}>" > "${surrogateFilePath}"
      VERBATIM
    )

    set(${pathToFile} ${surrogateFilePath} PARENT_SCOPE)
  endfunction()

  # Extracts all non-helper (e.g. catch_all.hpp) headers from the
  # Catch2 target, and returns them through the argument.
  function(ExtractCatch2Headers OutArg)
    get_target_property(targetSources Catch2 SOURCES)
    foreach(Source ${targetSources})
      string(REGEX MATCH "^.*\\.hpp$" isHeader ${Source})
      string(REGEX MATCH "_all.hpp" isAllHeader ${Source})
      # The runner's headers need C++20; the surrogates check Catch2's
      # headers at the language level of the build.
      string(REGEX MATCH "/Progmasoft/" isRunnerHeader ${Source})
      if(isHeader AND NOT isAllHeader AND NOT isRunnerHeader)
        list(APPEND AllHeaders ${Source})
      endif()
    endforeach()
    set(${OutArg} ${AllHeaders} PARENT_SCOPE)
  endfunction()

  ExtractCatch2Headers(mainHeaders)

  if(NOT mainHeaders)
    message(FATAL_ERROR "No headers in the main target were detected. Something is broken.")
  endif()

  foreach(header ${mainHeaders})
    createSurrogateFileTarget(${header} pathToGeneratedFile)
    list(APPEND surrogateFiles ${pathToGeneratedFile})
  endforeach()

  add_executable(Catch2SurrogateTarget
    ${surrogateFiles}
  )
  target_link_libraries(Catch2SurrogateTarget PRIVATE Catch2WithMain)
endif()

####
# Temporary workaround for VS toolset changes in 2017
# We need to disable <UseFullPaths> property, but CMake doesn't support it
# until 3.13 (not yet released)
####
if(MSVC)
  configure_file(${CATCH_DIR}/tools/misc/SelfTest.vcxproj.user ${CMAKE_BINARY_DIR}/tests COPYONLY)
endif() #Temporary workaround

# define the sources of the self test
# Please keep these ordered alphabetically
set(TEST_SOURCES
  ${SELF_TEST_DIR}/TestRegistrations.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/Algorithms.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/AssertionHandler.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/Details.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/FloatingPoint.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/GeneratorsImpl.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/Integer.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/InternalBenchmark.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/Parse.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/PartTracker.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/RandomNumberGeneration.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/Tag.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/TestCaseInfoHasher.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/TestSpec.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/TestSpecParser.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/String.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/StringManip.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/Traits.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/ToString.tests.cpp
  ${SELF_TEST_DIR}/IntrospectiveTests/UniquePtr.tests.cpp
  ${SELF_TEST_DIR}/helpers/parse_test_spec.cpp
  ${SELF_TEST_DIR}/TimingTests/Sleep.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Approx.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/BDD.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Benchmark.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Class.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Compilation.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Condition.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Decomposition.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/EnumToString.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Exception.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Generators.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Message.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Misc.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Skip.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringByte.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringChrono.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringGeneral.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringOptional.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringPair.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringTuple.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringVariant.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringVector.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/ToStringWhich.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Tricky.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/VariadicMacros.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/Matchers.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/MatchersConstexpr.tests.cpp
  ${SELF_TEST_DIR}/UsageTests/MatchersRanges.tests.cpp
)

set(TEST_HEADERS
  ${SELF_TEST_DIR}/helpers/parse_test_spec.hpp
  ${SELF_TEST_DIR}/helpers/range_test_helpers.hpp
  ${SELF_TEST_DIR}/helpers/type_with_lit_0_comparisons.hpp
)

# Specify the headers, too, so CLion recognises them as project files
set(HEADERS
  ${TOP_LEVEL_HEADERS}
  ${EXTERNAL_HEADERS}
  ${INTERNAL_HEADERS}
  ${REPORTER_HEADERS}
  ${BENCHMARK_HEADERS}
  ${BENCHMARK_SOURCES}
)

# Provide some groupings for IDEs
#source_group("benchmark" FILES ${BENCHMARK_HEADERS} ${BENCHMARK_SOURCES})
#source_group("Tests" FILES ${TEST_SOURCES})

include(CTest)

add_executable(SelfTest ${TEST_SOURCES} ${TEST_HEADERS})
target_include_directories(SelfTest PRIVATE ${SELF_TEST_DIR})
target_link_libraries(SelfTest PRIVATE Catch2WithMain)

# Exercise the C++20 Progmasoft layer independently so the upstream Catch2
# SelfTest continues to compile at its supported baseline language level.
add_executable(ProgmasoftSelfTest
  Progmasoft/Generators.tests.cpp
  Progmasoft/Property.tests.cpp
  Progmasoft/ForAll.tests.cpp
  Progmasoft/Results.tests.cpp
  Progmasoft/Runner.tests.cpp
  Progmasoft/RunnerUnits.tests.cpp
  Progmasoft/Snapshot.tests.cpp
  Progmasoft/Stateful.tests.cpp
  Progmasoft/Strategy.tests.cpp
)
target_link_libraries(ProgmasoftSelfTest PRIVATE Progmasoft::Catch3 Catch2WithMain)
target_compile_features(ProgmasoftSelfTest PRIVATE cxx_std_20)

# Tests that misbehave on purpose, for the runner's integration tests.
add_executable(RunnerFixture Progmasoft/RunnerFixture.cpp)
target_link_libraries(RunnerFixture PRIVATE Catch2WithMain)
target_compile_features(RunnerFixture PRIVATE cxx_std_20)
add_dependencies(ProgmasoftSelfTest RunnerFixture)

add_test(NAME Progmasoft::PropertyTests COMMAND $<TARGET_FILE:ProgmasoftSelfTest> --no-history)
set_tests_properties(Progmasoft::PropertyTests PROPERTIES
  ENVIRONMENT "CATCH3_RUNNER_FIXTURE=$<TARGET_FILE:RunnerFixture>"
)
if(BUILD_SHARED_LIBS AND WIN32)
  add_custom_command(TARGET SelfTest PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Catch2>
    $<TARGET_FILE:Catch2WithMain> $<TARGET_FILE_DIR:SelfTest>
  )
endif()

if(CATCH_ENABLE_COVERAGE)
  set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage build." FORCE)
  find_package(codecov)
  add_coverage(SelfTest)
  list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")
  coverage_evaluate()
endif()

# configure unit tests via CTest
add_test(NAME RunTests COMMAND $<TARGET_FILE:SelfTest> --order=random --warn=infinite-generators --no-history)
set_tests_properties(RunTests PROPERTIES COST 15)

# Because CTest does not allow us to check both return code _and_ expected
# output in one test, we run these commands twice. First time we check
# the output, the second time we check the exit code.
add_test(NAME List::Tests::Output COMMAND $<TARGET_FILE:SelfTest> --list)
set_tests_properties(List::Tests::Output PROPERTIES
  PASS_REGULAR_EXPRESSION "[0-9]+ matching test\\(s\\)"
  FAIL_REGULAR_EXPRESSION "Hidden Test"
)
add_test(NAME List::Tests::ExitCode COMMAND $<TARGET_FILE:SelfTest> --list)

add_test(NAME List::Tests::Names COMMAND $<TARGET_FILE:SelfTest> --list=names)
# CTest regexes cannot anchor at the start of a line, so a name indented by
# space or tab fails the test.
set_tests_properties(List::Tests::Names PROPERTIES
  PASS_REGULAR_EXPRESSION "#1905 -- test spec parser properly clears internal state between compound tests[\r\n]"
  FAIL_REGULAR_EXPRESSION "[ \t]#1905 -- test spec parser properly clears internal state between compound tests"
)

add_test(NAME List::Tests::Json COMMAND $<TARGET_FILE:SelfTest> --list=json)
set_tests_properties(List::Tests::Json PROPERTIES
  PASS_REGULAR_EXPRESSION "\"format\": \"catch3-tests\""
)

add_test(NAME List::Tags::Output COMMAND $<TARGET_FILE:SelfTest> --list-tags)
set_tests_properties(List::Tags::Output PROPERTIES
  PASS_REGULAR_EXPRESSION "[0-9]+ tag\\(s\\)"
  FAIL_REGULAR_EXPRESSION "\\[\\.\\]")
add_test(NAME List::Tags::ExitCode COMMAND $<TARGET_FILE:SelfTest> --list-tags)

add_test(NAME NoAssertions COMMAND $<TARGET_FILE:SelfTest> --no-history --warn=no-assertions "An empty test with no assertions")
set_tests_properties(NoAssertions PROPERTIES PASS_REGULAR_EXPRESSION "no assertion ran")

add_test(NAME TestSpecs::CombiningMatchingAndNonMatchingIsOk-1 COMMAND $<TARGET_FILE:SelfTest> --no-history Tracker, "___nonexistent_test___")

add_test(NAME TestSpecs::CombiningMatchingAndNonMatchingIsOk-2 COMMAND $<TARGET_FILE:SelfTest> --no-history Tracker, "___nonexistent_test___")
set_tests_properties(TestSpecs::CombiningMatchingAndNonMatchingIsOk-2 PROPERTIES
  PASS_REGULAR_EXPRESSION "no test matches '\"___nonexistent_test___\"'"
  FAIL_REGULAR_EXPRESSION "none ran"
)

add_test(NAME TestSpecs::NoMatchedTestsFail
  COMMAND $<TARGET_FILE:SelfTest> --no-history "___nonexistent_test___"
)
set_tests_properties(TestSpecs::NoMatchedTestsFail
  PROPERTIES
    WILL_FAIL ON
)
add_test(NAME TestSpecs::OverrideFailureWithNoMatchedTests
  COMMAND $<TARGET_FILE:SelfTest> --no-history "___nonexistent_test___" --allow-no-tests
)

add_test(NAME TestSpecs::OverrideAllSkipFailure
  COMMAND $<TARGET_FILE:SelfTest> --no-history "tests can be skipped dynamically at runtime" --allow-no-tests
)

add_test(NAME TestSpecs::NonMatchingTestSpecIsRoundTrippable
  COMMAND $<TARGET_FILE:SelfTest> --no-history Tracker, "this test does not exist" "[nor does this tag]"
)
set_tests_properties(TestSpecs::NonMatchingTestSpecIsRoundTrippable
  PROPERTIES
    PASS_REGULAR_EXPRESSION "no test matches '\"this test does not exist\" \\[nor does this tag\\]'"
)

add_test(NAME Warnings::UnmatchedTestSpecIsAccepted
  COMMAND $<TARGET_FILE:SelfTest> --no-history Tracker --warn=unmatched-specs
)

add_test(NAME Warnings::MultipleWarningsCanBeSpecified
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history Tracker
      --warn=no-assertions
      --warn=unmatched-specs
)

add_test(NAME TestSpecs::WarnUnmatchedTestSpecFailsWithUnmatchedTestSpec
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history Tracker, "___nonexistent_test___" --warn=unmatched-specs
)
set_tests_properties(TestSpecs::WarnUnmatchedTestSpecFailsWithUnmatchedTestSpec
  PROPERTIES
    WILL_FAIL ON
)

add_test(NAME UnmatchedOutputFilter COMMAND $<TARGET_FILE:SelfTest> --no-history [this-tag-does-not-exist])
set_tests_properties(UnmatchedOutputFilter
  PROPERTIES
    PASS_REGULAR_EXPRESSION "no test matches '\\[this-tag-does-not-exist\\]'"
)

add_test(NAME FilteredSections::SimpleExample::1 COMMAND $<TARGET_FILE:SelfTest> --no-history \#1394 --section=RunSection)
set_tests_properties(FilteredSections::SimpleExample::1 PROPERTIES FAIL_REGULAR_EXPRESSION "none ran")
add_test(NAME FilteredSections::SimpleExample::2 COMMAND $<TARGET_FILE:SelfTest> --no-history \#1394\ nested --section=NestedRunSection --section=s1)
set_tests_properties(FilteredSections::SimpleExample::2 PROPERTIES FAIL_REGULAR_EXPRESSION "none ran")

add_test(
  NAME
    FilteredSections::GeneratorsDontCauseInfiniteLoop
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history --verbose "#2025: original repro" --section=fov_0
)
set_tests_properties(FilteredSections::GeneratorsDontCauseInfiniteLoop
  PROPERTIES
    PASS_REGULAR_EXPRESSION "inside with fov: 0" # This should happen
    FAIL_REGULAR_EXPRESSION "inside with fov: 1" # This would mean there was no filtering
)

add_test(NAME "FilteredSections::HittingGeneratorsWithSectionFilterCausesSkip"
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history "baz" --path-filter=c:A
)
set_tests_properties("FilteredSections::HittingGeneratorsWithSectionFilterCausesSkip"
  PROPERTIES
    PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 skipped"
)

add_test(NAME RegressionCheck-1670 COMMAND $<TARGET_FILE:SelfTest> --no-history "#1670 regression check" --section=A)
set_tests_properties(RegressionCheck-1670 PROPERTIES PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 passed")

add_test(NAME VersionCheck COMMAND $<TARGET_FILE:SelfTest> --version)
set_tests_properties(VersionCheck PROPERTIES PASS_REGULAR_EXPRESSION "Catch3 [0-9]+\\.[0-9]+\\.[0-9]+")

add_test(NAME HelpCheck COMMAND $<TARGET_FILE:SelfTest> --help)
set_tests_properties(HelpCheck PROPERTIES PASS_REGULAR_EXPRESSION "--jobs=N")

add_test(NAME UnknownOptionHasHint COMMAND $<TARGET_FILE:SelfTest> --jbos=2)
set_tests_properties(UnknownOptionHasHint PROPERTIES PASS_REGULAR_EXPRESSION "did you mean --jobs\\?")

add_test(NAME FilenameAsTagsTest COMMAND $<TARGET_FILE:SelfTest> -\# --list-tags)
set_tests_properties(FilenameAsTagsTest PROPERTIES PASS_REGULAR_EXPRESSION "\\[#Approx.tests\\]")

# Check that the filename tags can also be matched against (#2064)
add_test(NAME FilenameAsTagsMatching COMMAND $<TARGET_FILE:SelfTest> -\# --list-tags [\#Approx.tests])
set_tests_properties(FilenameAsTagsMatching
  PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[#Approx.tests\\]"
    # Avoids false positives by looking for start of line (newline) before the 0
    FAIL_REGULAR_EXPRESSION "[\r\n]0 tag"
)

add_test(NAME EscapeSpecialCharactersInTestNames COMMAND $<TARGET_FILE:SelfTest> --no-history "Test with special\\, characters \"in name")
set_tests_properties(EscapeSpecialCharactersInTestNames PROPERTIES PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 passed")

add_test(NAME NegativeSpecNoHiddenTests COMMAND $<TARGET_FILE:SelfTest> --list ~[approval])
set_tests_properties(NegativeSpecNoHiddenTests PROPERTIES FAIL_REGULAR_EXPRESSION "\\[\\.\\]")

add_test(NAME TestsInFile::SimpleSpecs COMMAND $<TARGET_FILE:SelfTest> --no-history "--input-file=${SELF_TEST_DIR}/Misc/plain-old-tests.input")
set_tests_properties(TestsInFile::SimpleSpecs PROPERTIES PASS_REGULAR_EXPRESSION "2 test\\(s\\): 2 passed")

add_test(NAME TestsInFile::EscapeSpecialCharacters COMMAND $<TARGET_FILE:SelfTest> --no-history "--input-file=${SELF_TEST_DIR}/Misc/special-characters-in-file.input")
set_tests_properties(TestsInFile::EscapeSpecialCharacters PROPERTIES PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 passed")

add_test(NAME TestsInFile::InvalidTestNames-1 COMMAND $<TARGET_FILE:SelfTest> --no-history "--input-file=${SELF_TEST_DIR}/Misc/invalid-test-names.input")
set_tests_properties(TestsInFile::InvalidTestNames-1
  PROPERTIES
    PASS_REGULAR_EXPRESSION "invalid test spec"
    FAIL_REGULAR_EXPRESSION "none ran"
)

add_test(NAME TagAlias COMMAND $<TARGET_FILE:SelfTest> [@tricky] --list)
set_tests_properties(TagAlias PROPERTIES
  PASS_REGULAR_EXPRESSION "[1-9][0-9]* matching test\\(s\\)"
)

add_test(NAME CheckConvenienceHeaders
  COMMAND
    Python3::Interpreter ${CATCH_DIR}/tools/scripts/checkConvenienceHeaders.py
)
set_tests_properties(CheckConvenienceHeaders
  PROPERTIES
    LABELS "uses-python"
)

add_test(NAME "Benchmarking::SkipBenchmarkMacros"
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history "Skip benchmark macros"
      --skip-benchmarks
      --verbose
      --color=never
)
set_tests_properties("Benchmarking::SkipBenchmarkMacros"
  PROPERTIES
    PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 passed"
    FAIL_REGULAR_EXPRESSION "benchmark name"
)

add_test(NAME "Benchmarking::FailureReporting::ShouldFailIsRespected"
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history "Failing benchmark respects should-fail"
)
set_tests_properties("Benchmarking::FailureReporting::ShouldFailIsRespected"
  PROPERTIES
    PASS_REGULAR_EXPRESSION "1 failed as expected"
)

add_test(NAME "ErrorHandling::InvalidTestSpecExitsEarly"
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history "[aa,a]"
)
set_tests_properties("ErrorHandling::InvalidTestSpecExitsEarly"
  PROPERTIES
    PASS_REGULAR_EXPRESSION "invalid test spec: \\[aa,a\\]"
    FAIL_REGULAR_EXPRESSION "none ran"
)

add_test(NAME "Runner::SeedIsReported"
  COMMAND
    $<TARGET_FILE:SelfTest> --no-history "Factorials are computed" --seed=18181818
)
set_tests_properties("Runner::SeedIsReported"
  PROPERTIES
    PASS_REGULAR_EXPRESSION "seed 18181818"
)

if(CATCH_ENABLE_CONFIGURE_TESTS)
  foreach(testName "Disable" "DisableStringification"
                    "ExperimentalRedirect")
    add_test(NAME "CMakeConfig::${testName}"
      COMMAND
        Python3::Interpreter "${CMAKE_CURRENT_LIST_DIR}/TestScripts/testConfigure${testName}.py" "${CATCH_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties("CMakeConfig::${testName}"
      PROPERTIES
        COST 240
        LABELS "uses-python"
    )
    if(CMAKE_TOOLCHAIN_FILE)
      set_tests_properties("CMakeConfig::${testName}"
        PROPERTIES ENVIRONMENT "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
      )
    endif()
  endforeach()
endif()

if(CATCH_ENABLE_CMAKE_HELPER_TESTS)
  add_test(NAME "CMakeHelper::DiscoverTests"
    COMMAND
      Python3::Interpreter "${CMAKE_CURRENT_LIST_DIR}/TestScripts/DiscoverTests/VerifyRegistration.py" "${CATCH_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
  )
  set_tests_properties("CMakeHelper::DiscoverTests"
    PROPERTIES
      COST 240
      LABELS "uses-python"
  )

  add_test(NAME "CMakeHelper::PrepareCommandFragment"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DCATCH_ADD_TESTS_SCRIPT=${CATCH_DIR}/extras/CatchAddTests.cmake"
      -P "${CMAKE_CURRENT_LIST_DIR}/TestScripts/DiscoverTests/TestPrepareCommandFragment.cmake"
  )

  add_test(NAME "CMakeHelper::DecomposeJsonArray"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DCATCH_ADD_TESTS_SCRIPT=${CATCH_DIR}/extras/CatchAddTests.cmake"
      -P "${CMAKE_CURRENT_LIST_DIR}/TestScripts/DiscoverTests/TestDecomposeJsonArray.cmake"
  )
endif()

add_test(NAME "Bazel::RngSeedEnvVar::JustEnv"
  COMMAND
    $<TARGET_FILE:SelfTest> "Factorials are computed"
)
set_tests_properties("Bazel::RngSeedEnvVar::JustEnv"
  PROPERTIES
    ENVIRONMENT "BAZEL_TEST=1;TEST_RANDOM_SEED=18181818"
    PASS_REGULAR_EXPRESSION "seed 18181818"
)

add_test(NAME "Bazel::RngSeedEnvVar::EnvHasPriorityOverCLI"
  COMMAND
    $<TARGET_FILE:SelfTest> "Factorials are computed"
    --seed=17171717
)
set_tests_properties("Bazel::RngSeedEnvVar::EnvHasPriorityOverCLI"
  PROPERTIES
    ENVIRONMENT "BAZEL_TEST=1;TEST_RANDOM_SEED=18181818"
    PASS_REGULAR_EXPRESSION "seed 18181818"
)

add_test(NAME "Bazel::RngSeedEnvVar::MalformedValueIsIgnored"
  COMMAND
    $<TARGET_FILE:SelfTest> "Factorials are computed"
    --seed=17171717
)
set_tests_properties("Bazel::RngSeedEnvVar::MalformedValueIsIgnored"
  PROPERTIES
    ENVIRONMENT "BAZEL_TEST=1;TEST_RANDOM_SEED=XOXOXOXO"
    PASS_REGULAR_EXPRESSION "seed 17171717"
)

list(APPEND CATCH_TEST_TARGETS SelfTest)
set(CATCH_TEST_TARGETS ${CATCH_TEST_TARGETS} PARENT_SCOPE)
