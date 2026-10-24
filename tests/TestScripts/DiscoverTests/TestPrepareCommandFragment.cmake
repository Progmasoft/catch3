# SPDX-License-Identifier: BSL-1.0

# Unit tests for `catch3_prepare_command_fragment` helper in `extras/Catch3AddTests.cmake`.
#
# Yes, we are at the stage where the script helpers need unit tests.
#
# Run as
#     cmake -DCATCH3_ADD_TESTS_SCRIPT=/path/to/extras/Catch3AddTests.cmake \
#           -P TestPrepareCommandFragment.cmake


cmake_minimum_required(VERSION 3.19)

if(NOT DEFINED CATCH3_ADD_TESTS_SCRIPT)
  message(FATAL_ERROR "Missing argument `CATCH3_ADD_TESTS_SCRIPT`")
endif()

if(NOT EXISTS "${CATCH3_ADD_TESTS_SCRIPT}")
  message(FATAL_ERROR "Cannot find Catch3AddTests.cmake at '${CATCH3_ADD_TESTS_SCRIPT}'")
endif()

# Pull in the helper functions. Without `TEST_EXECUTABLE` being defined,
# `catch3_discover_tests_impl` is not called.
include("${CATCH3_ADD_TESTS_SCRIPT}")

set(_failures 0)

function(expect_equal description actual expected)
  if(actual STREQUAL expected)
    message("  [PASS] ${description}")
  else()
    message("  [FAIL] ${description}")
    message("         expected: [${expected}]")
    message("         actual:   [${actual}]")
    math(EXPR _n "${_failures} + 1")
    set(_failures "${_n}" PARENT_SCOPE)
  endif()
endfunction()

catch3_prepare_command_fragment(test_fragment SimpleName /path/to/tests)
expect_equal("Simple arg, no quotes" "${test_fragment}" " SimpleName /path/to/tests")

catch3_prepare_command_fragment(test_fragment "Name with spaces")
expect_equal("Spaces in arg, needs quotes" "${test_fragment}" " [==[Name with spaces]==]")

catch3_prepare_command_fragment(test_fragment Foo PROPERTIES LABELS "tagA\;tagB\;tagC")
expect_equal("semicolons in argument are kept and quoted"
  "${test_fragment}" " Foo PROPERTIES LABELS [==[tagA\;tagB\;tagC]==]")

set(test_fragment "PRE-EXISTING")
catch3_prepare_command_fragment(test_fragment Foo PROPERTIES BAR baz)
expect_equal("out var does no accumulate commands"
  "${test_fragment}" " Foo PROPERTIES BAR baz")



if(_failures GREATER 0)
  message(FATAL_ERROR "${_failures} catch3_prepare_command_fragment test(s) failed")
else()
  message(STATUS "All catch3_prepare_command_fragment tests passed")
endif()
