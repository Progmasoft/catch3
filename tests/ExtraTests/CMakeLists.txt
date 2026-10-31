cmake_minimum_required(VERSION 3.16)
project(Catch3ExtraTests LANGUAGES CXX)

message(STATUS "Extra tests included")

add_test(
  NAME TestShardingIntegration
  COMMAND Python3::Interpreter ${CATCH_DIR}/tests/TestScripts/testSharding.py $<TARGET_FILE:SelfTest>
)
set_tests_properties(TestShardingIntegration
  PROPERTIES
    LABELS "uses-python"
)

add_test(
  NAME TestSharding::OverlyLargeShardIndex
  COMMAND $<TARGET_FILE:SelfTest> --shard=5/5
)
set_tests_properties(TestSharding::OverlyLargeShardIndex
  PROPERTIES
    PASS_REGULAR_EXPRESSION "--shard takes INDEX/COUNT with INDEX below COUNT"
)

# The slowest tests are listed with their durations. The tests sleep, so
# they run less often, as extra tests.
add_test(NAME Durations::Slowest COMMAND $<TARGET_FILE:SelfTest> --no-history --slowest=1 [min_duration_test])
set_tests_properties(Durations::Slowest
  PROPERTIES
    PASS_REGULAR_EXPRESSION "slowest tests:[\r\n]+ +[0-9.]+ s  sleep_for_1000ms"
    FAIL_REGULAR_EXPRESSION "  sleep_for_100ms"
)

# define folders used:
set(TESTS_DIR ${CATCH_DIR}/tests/ExtraTests)

add_executable(PrefixedMacros ${TESTS_DIR}/X01-PrefixedMacros.cpp)
target_compile_definitions(PrefixedMacros PRIVATE CATCH_CONFIG_PREFIX_ALL CATCH_CONFIG_RUNTIME_STATIC_REQUIRE)
# We want to verify that the main library can also be built with prefixed
# macros, regression test for #3087.
target_link_libraries(PrefixedMacros Catch3_buildall_interface)

add_test(NAME CATCH_CONFIG_PREFIX_ALL COMMAND PrefixedMacros --no-history --success)
set_tests_properties(CATCH_CONFIG_PREFIX_ALL
  PROPERTIES
    PASS_REGULAR_EXPRESSION "CATCH_"
    FAIL_REGULAR_EXPRESSION
    # The spaces are important -> They disambiguate between CATCH_REQUIRE
    # and REQUIRE without prefix.
    " REQUIRE; REQUIRE_FALSE; REQUIRE_THROWS; REQUIRE_THROWS_AS; REQUIRE_THROWS_WITH; REQUIRE_THROWS_MATCHES; REQUIRE_NOTHROW; CHECK; CHECK_FALSE; CHECKED_IF; CHECKED_ELSE; CHECK_NOFAIL; CHECK_THROWS; CHECK_THROWS_AS; CHECK_THROWS_WITH; CHECK_THROWS_MATCHES; CHECK_NOTHROW; REQUIRE_THAT; CHECK_THAT"
)

add_executable(DisabledMacros ${TESTS_DIR}/X02-DisabledMacros.cpp)
target_compile_definitions(DisabledMacros PRIVATE CATCH_CONFIG_DISABLE)
# Macro configuration does not touch the compiled parts, so we can link
# it against the main library
target_link_libraries(DisabledMacros Catch3WithMain)

add_test(NAME CATCH_CONFIG_DISABLE-1 COMMAND DisabledMacros --no-history --success)
set_tests_properties(CATCH_CONFIG_DISABLE-1
  PROPERTIES
    PASS_REGULAR_EXPRESSION "none ran"
    FAIL_REGULAR_EXPRESSION "This should not happen"
)
add_test(NAME CATCH_CONFIG_DISABLE-2 COMMAND DisabledMacros --list)
set_tests_properties(CATCH_CONFIG_DISABLE-2
  PROPERTIES
    PASS_REGULAR_EXPRESSION "0 matching test\\(s\\)"
)

add_executable(DisabledExceptions-DefaultHandler ${TESTS_DIR}/X03-DisabledExceptions-DefaultHandler.cpp)
add_executable(DisabledExceptions-CustomHandler ${TESTS_DIR}/X04-DisabledExceptions-CustomHandler.cpp)

# clang-cl reports the Clang compiler ID but takes MSVC's options, so the
# front end decides which spelling turns exceptions off.
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" OR MSVC)
  set(CATCH_NO_EXCEPTIONS_OPTIONS /EHs-c- /D_HAS_EXCEPTIONS=0)
else()
  set(CATCH_NO_EXCEPTIONS_OPTIONS -fno-exceptions)
endif()
foreach(target DisabledExceptions-DefaultHandler DisabledExceptions-CustomHandler)
  target_compile_options(${target} PUBLIC ${CATCH_NO_EXCEPTIONS_OPTIONS})
  target_link_libraries(${target} Catch3_buildall_interface)
endforeach()
target_compile_definitions(DisabledExceptions-CustomHandler PUBLIC CATCH_CONFIG_DISABLE_EXCEPTIONS_CUSTOM_HANDLER)


add_test(NAME CATCH_CONFIG_DISABLE_EXCEPTIONS-1 COMMAND DisabledExceptions-DefaultHandler --no-history --retries=off "Tests that run")
set_tests_properties(CATCH_CONFIG_DISABLE_EXCEPTIONS-1
  PROPERTIES
    PASS_REGULAR_EXPRESSION "4 assertion\\(s\\)"
    FAIL_REGULAR_EXPRESSION "abort;terminate;fatal"
)

# Bazel passes its wishes in the environment: a JUnit file, a filter, a
# shard, a seed.
add_executable(BazelReporterNoCatchConfig ${TESTS_DIR}/X30-BazelReporter.cpp)
target_link_libraries(BazelReporterNoCatchConfig Catch3WithMain)
add_test(NAME NO_CATCH_CONFIG_BAZEL_REPORTER-1
  COMMAND
  Python3::Interpreter "${CATCH_DIR}/tests/TestScripts/testBazelReporter.py" $<TARGET_FILE:BazelReporterNoCatchConfig> "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(NO_CATCH_CONFIG_BAZEL_REPORTER-1
  PROPERTIES
    LABELS "uses-python"
    ENVIRONMENT "BAZEL_TEST=1"
)

add_test(NAME BazelEnv::TESTBRIDGE_TEST_ONLY
  COMMAND
      $<TARGET_FILE:BazelReporterNoCatchConfig>
)
set_tests_properties(BazelEnv::TESTBRIDGE_TEST_ONLY
  PROPERTIES
    ENVIRONMENT "BAZEL_TEST=1;TESTBRIDGE_TEST_ONLY=Passing test case"
    PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 passed"
)

add_test(NAME BazelEnv::Sharding
  COMMAND
    Python3::Interpreter "${CATCH_DIR}/tests/TestScripts/testBazelSharding.py"
      $<TARGET_FILE:BazelReporterNoCatchConfig>
      "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(BazelEnv::Sharding
  PROPERTIES
    LABELS "uses-python"
)

# The default handler on Windows leads to the just-in-time debugger firing,
# which makes this test unsuitable for CI and headless runs, as it opens
# up an interactive dialog.
if(NOT WIN32)
  add_test(NAME CATCH_CONFIG_DISABLE_EXCEPTIONS-2 COMMAND DisabledExceptions-DefaultHandler --no-history --retries=off "Tests that abort")
  set_tests_properties(CATCH_CONFIG_DISABLE_EXCEPTIONS-2
    PROPERTIES
      PASS_REGULAR_EXPRESSION "Catch will terminate"
  )
endif()

add_test(NAME CATCH_CONFIG_DISABLE_EXCEPTIONS-3 COMMAND DisabledExceptions-CustomHandler --no-history --retries=off "Tests that run")
set_tests_properties(CATCH_CONFIG_DISABLE_EXCEPTIONS-3
  PROPERTIES
    PASS_REGULAR_EXPRESSION "4 assertion\\(s\\)"
    FAIL_REGULAR_EXPRESSION "====== CUSTOM HANDLER ======"
)

add_test(NAME CATCH_CONFIG_DISABLE_EXCEPTIONS-4 COMMAND DisabledExceptions-CustomHandler --no-history --retries=off "Tests that abort")
set_tests_properties(CATCH_CONFIG_DISABLE_EXCEPTIONS-4
  PROPERTIES
    PASS_REGULAR_EXPRESSION "====== CUSTOM HANDLER ======"
)

add_executable(DeferredStaticChecks ${TESTS_DIR}/X05-DeferredStaticChecks.cpp)
target_link_libraries(DeferredStaticChecks PRIVATE Catch3WithMain)
target_compile_definitions(DeferredStaticChecks PRIVATE "CATCH_CONFIG_RUNTIME_STATIC_REQUIRE")

add_test(NAME DeferredStaticChecks COMMAND DeferredStaticChecks --no-history --retries=off)
set_tests_properties(DeferredStaticChecks
  PROPERTIES
    PASS_REGULAR_EXPRESSION "1 test\\(s\\): 1 failed[\r\n]+[34] assertion\\(s\\)"
)

add_executable(MixingClearedAndUnclearedMessages ${TESTS_DIR}/X06-MixingClearedAndUnclearedMessages.cpp)
target_link_libraries(MixingClearedAndUnclearedMessages PRIVATE Catch3WithMain)

add_test(NAME MixingClearedAndUnclearedMessages
  COMMAND
    MixingClearedAndUnclearedMessages
    --no-history --success)
set_tests_properties(MixingClearedAndUnclearedMessages
  PROPERTIES
    PASS_REGULAR_EXPRESSION "FAILED: REQUIRE\\( false \\)[\r\n]+ +with context: b[\r\n]"
    FAIL_REGULAR_EXPRESSION "FAILED: REQUIRE\\( false \\)[\r\n]+ +with context: a"
)

add_executable(FallbackStringifier ${TESTS_DIR}/X10-FallbackStringifier.cpp)
target_compile_definitions(FallbackStringifier PRIVATE CATCH_CONFIG_FALLBACK_STRINGIFIER=fallbackStringifier)
target_link_libraries(FallbackStringifier Catch3WithMain)

add_test(NAME FallbackStringifier COMMAND FallbackStringifier --no-history --success)
set_tests_properties(FallbackStringifier
  PROPERTIES
    PASS_REGULAR_EXPRESSION "foo{} \\) for: { !!! }"
)

add_executable(DisableStringification ${TESTS_DIR}/X11-DisableStringification.cpp)
target_compile_definitions(DisableStringification PRIVATE CATCH_CONFIG_DISABLE_STRINGIFICATION)
target_link_libraries(DisableStringification Catch3WithMain)
add_test(NAME CATCH_CONFIG_DISABLE_STRINGIFICATION COMMAND DisableStringification --no-history --success)
set_tests_properties(CATCH_CONFIG_DISABLE_STRINGIFICATION
  PROPERTIES
    PASS_REGULAR_EXPRESSION "Disabled by CATCH_CONFIG_DISABLE_STRINGIFICATION"
    FAIL_REGULAR_EXPRESSION  "Hidden{} == Hidden{}"
)

# This test touches windows.h, so it should only be compiled under msvc
if(MSVC)
  # This test fails if it does not compile and succeeds otherwise
  add_executable(WindowsHeader ${TESTS_DIR}/X90-WindowsHeaderInclusion.cpp)
  target_link_libraries(WindowsHeader Catch3WithMain)
  add_test(NAME WindowsHeader COMMAND WindowsHeader --no-history)
  list(APPEND CATCH_TEST_TARGETS ${EXTRA_TEST_BINARIES} WindowsHeader)
endif()

add_executable(PartialTestCaseEvents ${TESTS_DIR}/X21-PartialTestCaseEvents.cpp)
target_link_libraries(PartialTestCaseEvents PRIVATE Catch3WithMain)
add_test(
  NAME PartialTestCaseEvents
  COMMAND Python3::Interpreter ${CATCH_DIR}/tests/TestScripts/testPartialTestCaseEvent.py $<TARGET_FILE:PartialTestCaseEvents>
)
set_tests_properties(PartialTestCaseEvents
  PROPERTIES
    LABELS "uses-python"
)

add_executable(CapturedStdoutInTestCaseEvents ${TESTS_DIR}/X27-CapturedStdoutInTestCaseEvents.cpp)
target_link_libraries(CapturedStdoutInTestCaseEvents PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME Listeners::CapturedStdOutInEvents
  COMMAND CapturedStdoutInTestCaseEvents --no-history
)
set_tests_properties(Listeners::CapturedStdOutInEvents
  PROPERTIES
    PASS_REGULAR_EXPRESSION "X27 - listener saw 0 error\\(s\\)"
)

add_executable(ListenerStdoutCaptureInMultireporter ${TESTS_DIR}/X24-ListenerStdoutCaptureInMultireporter.cpp)
target_link_libraries(ListenerStdoutCaptureInMultireporter PRIVATE Catch3::Catch3WithMain)

# The runner keeps what a passing test prints, whatever a listener prefers;
# what the listener prints outside a test reaches the output.
add_test(
  NAME Listeners::NoncapturingListenerDoesntCauseStdoutPassThrough
  COMMAND ListenerStdoutCaptureInMultireporter --no-history
)
set_tests_properties(Listeners::NoncapturingListenerDoesntCauseStdoutPassThrough
  PROPERTIES
    PASS_REGULAR_EXPRESSION "X24 - NonCapturingListener initialized"
    FAIL_REGULAR_EXPRESSION "X24 - FooBarBaz"
)

add_executable(ListenerCanAskForCapturedStdout ${TESTS_DIR}/X25-ListenerCanAskForCapturedStdout.cpp)
target_link_libraries(ListenerCanAskForCapturedStdout PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME Listeners::CapturingListenerCausesStdoutCapture
  COMMAND ListenerCanAskForCapturedStdout --no-history
)
set_tests_properties(Listeners::CapturingListenerCausesStdoutCapture
  PROPERTIES
    PASS_REGULAR_EXPRESSION "CapturingListener initialized"
    FAIL_REGULAR_EXPRESSION "X25 - ERROR"
)

add_executable(ListenersGetEventsBeforeReporters ${TESTS_DIR}/X28-ListenersGetEventsBeforeReporters.cpp)
target_link_libraries(ListenersGetEventsBeforeReporters PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME ListenersGetRunEvents
  COMMAND ListenersGetEventsBeforeReporters --no-history
)
set_tests_properties(ListenersGetRunEvents
  PROPERTIES
    PASS_REGULAR_EXPRESSION "X28 - listener received every event"
    FAIL_REGULAR_EXPRESSION "X28 - ERROR"
)

add_executable(DuplicatedTestCases-SameNameAndTags ${TESTS_DIR}/X31-DuplicatedTestCases.cpp)
target_link_libraries(DuplicatedTestCases-SameNameAndTags PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME DuplicatedTestCases::SameNameAndTags
  COMMAND $<TARGET_FILE:DuplicatedTestCases-SameNameAndTags> --no-history
)
set_tests_properties(DuplicatedTestCases::SameNameAndTags
  PROPERTIES
     PASS_REGULAR_EXPRESSION "error: .* already defined\\."
)

add_executable(DuplicatedTestCases-SameNameDifferentTags ${TESTS_DIR}/X32-DuplicatedTestCasesDifferentTags.cpp)
target_link_libraries(DuplicatedTestCases-SameNameDifferentTags PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME DuplicatedTestCases::SameNameDifferentTags
  COMMAND $<TARGET_FILE:DuplicatedTestCases-SameNameDifferentTags> --no-history
)
set_tests_properties(DuplicatedTestCases::SameNameDifferentTags
  PROPERTIES
     FAIL_REGULAR_EXPRESSION "error: .* already defined\\."
)

add_executable(DuplicatedTestCases-DuplicatedTestCaseMethods ${TESTS_DIR}/X33-DuplicatedTestCaseMethods.cpp)
target_link_libraries(DuplicatedTestCases-DuplicatedTestCaseMethods PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME DuplicatedTestCases::DuplicatedTestCaseMethods
  COMMAND $<TARGET_FILE:DuplicatedTestCases-DuplicatedTestCaseMethods> --no-history
)
set_tests_properties(DuplicatedTestCases::DuplicatedTestCaseMethods
  PROPERTIES
     PASS_REGULAR_EXPRESSION "error: .* already defined\\."
)

add_executable(DuplicatedTestCases-DifferentFixtures ${TESTS_DIR}/X34-DuplicatedTestCaseMethodsDifferentFixtures.cpp)
target_link_libraries(DuplicatedTestCases-DifferentFixtures PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME DuplicatedTestCases::DuplicatedTestCaseMethodsDifferentFixtures
  COMMAND $<TARGET_FILE:DuplicatedTestCases-DifferentFixtures> --no-history
)
set_tests_properties(DuplicatedTestCases::DuplicatedTestCaseMethodsDifferentFixtures
  PROPERTIES
    FAIL_REGULAR_EXPRESSION "error: .* already defined\\."
)

add_executable(ReportingCrashWithJunitReporter ${TESTS_DIR}/X36-ReportingCrashWithJunitReporter.cpp)
target_link_libraries(ReportingCrashWithJunitReporter PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME Runner::CrashIsReported
  COMMAND $<TARGET_FILE:ReportingCrashWithJunitReporter> --no-history --retries=off
)
set_tests_properties(Runner::CrashIsReported
  PROPERTIES
    PASS_REGULAR_EXPRESSION "CRASH raises signal"
    LABELS "uses-signals"
)


add_executable(ReportingInvalidTestCaseTags ${TESTS_DIR}/X37-ReportingInvalidTestCaseTags.cpp)
target_link_libraries(ReportingInvalidTestCaseTags PRIVATE Catch3::Catch3WithMain)
add_test(
  NAME Validation::TestCaseTags
  COMMAND $<TARGET_FILE:ReportingInvalidTestCaseTags> --no-history
)
# Passing multiple PASS_REGULAR_EXPRESSION does not work, so we join multiple ones with '.*'
set(regexp
  "Found an empty tag while registering test case 'Empty tag is not allowed'"
  "Tag name: \\[!abc\\] is not allowed."
  "Tag names starting with non-alphanumeric characters are reserved."
  "Tag name: \\[@def\\] is not allowed."
  "Tag names starting with non-alphanumeric characters are reserved."
)
list(JOIN regexp ".*" regexp)
set_tests_properties(Validation::TestCaseTags
  PROPERTIES
    PASS_REGULAR_EXPRESSION ${regexp}
)


add_executable(QuickExitInTest ${TESTS_DIR}/X40-QuickExit.cpp)
target_link_libraries(QuickExitInTest PRIVATE Catch3::Catch3WithMain)
# A test that ends its process early cannot pass: the exit of its worker is
# reported as a crash of the test.
add_test(
  NAME Runner::ExitInTestIsACrash
  COMMAND $<TARGET_FILE:QuickExitInTest> --no-history --retries=off
)
set_tests_properties(Runner::ExitInTestIsACrash
  PROPERTIES
    PASS_REGULAR_EXPRESSION "crashed with exit code 0"
)
add_test(
  NAME BazelEnv::TEST_PREMATURE_EXIT_FILE
  COMMAND
    Python3::Interpreter
    "${CATCH_DIR}/tests/TestScripts/testBazelExitGuardFile.py"
    $<TARGET_FILE:QuickExitInTest>
    "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(BazelEnv::TEST_PREMATURE_EXIT_FILE
  PROPERTIES
    LABELS "uses-python"
)


add_executable(AssertionStartingEventGoesBeforeAssertionIsEvaluated
  X20-AssertionStartingEventGoesBeforeAssertionIsEvaluated.cpp
)
target_link_libraries(AssertionStartingEventGoesBeforeAssertionIsEvaluated
  PRIVATE Catch3::Catch3WithMain
)
add_test(
  NAME ReporterEvents::AssertionStartingHappensBeforeAssertionIsEvaluated
  COMMAND $<TARGET_FILE:AssertionStartingEventGoesBeforeAssertionIsEvaluated> --no-history
)

add_executable(NoTests ${TESTS_DIR}/X92-NoTests.cpp)
target_link_libraries(NoTests PRIVATE Catch3::Catch3WithMain)

add_test(
  NAME TestSpecs::EmptySpecWithNoTestsFails
  COMMAND $<TARGET_FILE:NoTests> --no-history
)
set_tests_properties(TestSpecs::EmptySpecWithNoTestsFails
  PROPERTIES
    WILL_FAIL ON
)

add_test(
  NAME TestSpecs::OverrideFailureWithEmptySpec
  COMMAND $<TARGET_FILE:NoTests> --no-history --allow-no-tests
)

add_executable(AllSkipped ${TESTS_DIR}/X93-AllSkipped.cpp)
target_link_libraries(AllSkipped PRIVATE Catch3::Catch3WithMain)

add_test(
  NAME TestSpecs::SkippingAllTestsFails
  COMMAND $<TARGET_FILE:AllSkipped> --no-history
)
set_tests_properties(TestSpecs::SkippingAllTestsFails
  PROPERTIES
    WILL_FAIL ON
)

add_executable(FastCompileMacros ${TESTS_DIR}/X07-FastCompileMacros.cpp)
target_link_libraries(FastCompileMacros PRIVATE Catch3_buildall_interface)
target_compile_definitions(FastCompileMacros PRIVATE CATCH_CONFIG_FAST_COMPILE)

add_test(
  NAME CompileConfiguration::FastCompile
  COMMAND $<TARGET_FILE:FastCompileMacros> --no-history --retries=off
)
set_tests_properties(CompileConfiguration::FastCompile
  PROPERTIES
    PASS_REGULAR_EXPRESSION "6 test\\(s\\): 1 passed, 1 failed, 4 failed as expected[\r\n]+13 assertion\\(s\\)"
)

# This sets up a one-off executable that compiles against the amalgamated
# files, and then runs it for a super simple check that the amalgamated
# files are usable.
add_executable(AmalgamatedTestCompilation
  ${TESTS_DIR}/X91-AmalgamatedCatch.cpp
  ${CATCH_DIR}/extras/catch_amalgamated.hpp
  ${CATCH_DIR}/extras/catch_amalgamated.cpp
  ${CATCH_DIR}/extras/catch_amalgamated_third_party.hpp
)
target_include_directories(AmalgamatedTestCompilation PRIVATE ${CATCH_DIR}/extras)
# The amalgamated files hold the runner, which needs C++20.
target_compile_features(AmalgamatedTestCompilation PRIVATE cxx_std_17)

add_test(NAME AmalgamatedFileTest COMMAND AmalgamatedTestCompilation --no-history)
set_tests_properties(AmalgamatedFileTest
  PROPERTIES
    PASS_REGULAR_EXPRESSION "3 test\\(s\\): 3 passed[\r\n]+14 assertion\\(s\\)"
)

add_executable(ThreadSafetyTests
  ${TESTS_DIR}/X94-ThreadSafetyTests.cpp
)
target_link_libraries(ThreadSafetyTests Catch3_buildall_interface)
target_compile_definitions(ThreadSafetyTests PUBLIC CATCH_CONFIG_THREAD_SAFE_ASSERTIONS)

add_test(NAME ThreadSafetyTests::ScopedMessagesAndAssertions
  COMMAND ThreadSafetyTests --no-history "Failed REQUIRE in the main thread is fine"
)
set_tests_properties(ThreadSafetyTests::ScopedMessagesAndAssertions
  PROPERTIES
    PASS_REGULAR_EXPRESSION "1 failed as expected[\r\n]+801 assertion\\(s\\)"
    RUN_SERIAL ON
)
add_test(NAME ThreadSafetyTests::UnscopedMessagesAndAssertions
  COMMAND ThreadSafetyTests --no-history "Using unscoped messages in sibling threads"
)
set_tests_properties(ThreadSafetyTests::UnscopedMessagesAndAssertions
  PROPERTIES
    PASS_REGULAR_EXPRESSION "1 failed as expected[\r\n]+401 assertion\\(s\\)"
    RUN_SERIAL ON
)

add_executable(InfiniteGenerators ${TESTS_DIR}/X95-InfiniteGenerators.cpp)
target_link_libraries(InfiniteGenerators PRIVATE Catch3::Catch3WithMain)

add_test(
  NAME Warnings::InfiniteGenerators::NoFilterWarns
  COMMAND $<TARGET_FILE:InfiniteGenerators> --no-history --retries=off --warn=infinite-generators
)
set_tests_properties(Warnings::InfiniteGenerators::NoFilterWarns
  PROPERTIES
    # One test case fails with infinite generator, but the other one runs
    PASS_REGULAR_EXPRESSION "2 test\\(s\\): 1 passed, 1 failed"
    TIMEOUT 10
)

add_test(
  NAME Warnings::InfiniteGenerators::MatchAllFilterWarns
  COMMAND $<TARGET_FILE:InfiniteGenerators>
    --no-history --retries=off
    --warn=infinite-generators
    --path-filter=g:*
)
set_tests_properties(Warnings::InfiniteGenerators::MatchAllFilterWarns
  PROPERTIES
    # One test case fails with infinite generator, but the other one runs
    PASS_REGULAR_EXPRESSION "2 test\\(s\\): 1 passed, 1 failed"
    TIMEOUT 10
)

add_test(
  NAME Warnings::InfiniteGenerators::MatchOneFilterDoesntWarn
  COMMAND $<TARGET_FILE:InfiniteGenerators>
    --no-history --retries=off
    --warn=infinite-generators
    --path-filter=g:1
)
set_tests_properties(Warnings::InfiniteGenerators::MatchOneFilterDoesntWarn
  PROPERTIES
    PASS_REGULAR_EXPRESSION "2 test\\(s\\): 2 passed"
    TIMEOUT 10
)

add_test(
  NAME Warnings::InfiniteGenerators::SectionFilterWarns
  COMMAND $<TARGET_FILE:InfiniteGenerators>
    --no-history --retries=off
    --warn=infinite-generators
    --section=FooBarBaz
)
set_tests_properties(Warnings::InfiniteGenerators::SectionFilterWarns
  PROPERTIES
    # One test case fails with infinite generator, but the other one runs
    PASS_REGULAR_EXPRESSION "2 test\\(s\\): 1 passed, 1 failed"
    TIMEOUT 10
)


set(EXTRA_TEST_BINARIES
  AllSkipped
  PrefixedMacros
  DisabledMacros
  DisabledExceptions-DefaultHandler
  DisabledExceptions-CustomHandler
  FallbackStringifier
  DisableStringification
  PartialTestCaseEvents
  DuplicatedTestCases-SameNameAndTags
  DuplicatedTestCases-SameNameDifferentTags
  DuplicatedTestCases-DuplicatedTestCaseMethods
  NoTests
  ListenersGetEventsBeforeReporters
  MixingClearedAndUnclearedMessages
  FastCompileMacros
  InfiniteGenerators
  ThreadSafetyTests
#  DebugBreakMacros
)

# Notice that we are modifying EXTRA_TEST_BINARIES destructively, do not
# use it after this point!
list(FILTER EXTRA_TEST_BINARIES EXCLUDE REGEX "DisabledExceptions.*")
list(APPEND CATCH_TEST_TARGETS ${EXTRA_TEST_BINARIES})
set(CATCH_TEST_TARGETS ${CATCH_TEST_TARGETS} PARENT_SCOPE)
