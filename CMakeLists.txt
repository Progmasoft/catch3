cmake_minimum_required(VERSION 3.16)

# detect if Catch is being bundled,
# disable testsuite in that case
if(NOT DEFINED PROJECT_NAME)
  set(NOT_SUBPROJECT ON)
else()
  set(NOT_SUBPROJECT OFF)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(CATCH3_INSTALL_DOCS "Install documentation alongside library" ON)
option(CATCH3_INSTALL_EXTRAS "Install extras (CMake scripts, debugger helpers) alongside library" ON)
option(CATCH3_DEVELOPMENT_BUILD "Build tests, enable warnings, enable Werror, etc" OFF)
option(CATCH3_ENABLE_REPRODUCIBLE_BUILD "Add compiler flags for improving build reproducibility" ON)

include(CMakeDependentOption)
cmake_dependent_option(CATCH3_BUILD_TESTING "Build the SelfTest project" ON "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_BUILD_EXAMPLES "Build code examples" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_BUILD_EXTRA_TESTS "Build extra tests" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_BUILD_FUZZERS "Build fuzzers" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_BUILD_BENCHMARKS "Build the benchmarks" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_ENABLE_COVERAGE "Generate coverage for codecov.io" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_ENABLE_WERROR "Enables Werror during build" ON "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_BUILD_SURROGATES "Enable generating and building surrogate TUs for the main headers" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_ENABLE_CONFIGURE_TESTS "Enable CMake configuration tests. WARNING: VERY EXPENSIVE" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)
cmake_dependent_option(CATCH3_ENABLE_CMAKE_HELPER_TESTS "Enable CMake helper tests. WARNING: VERY EXPENSIVE" OFF "CATCH3_DEVELOPMENT_BUILD" OFF)

# The build breaks if done in-tree. You probably should not build
# things in tree anyway, but we can allow projects that include Catch3
# as a subproject to build in-tree as long as it is not in our tree.
if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

# The version is Catch3's; the Catch2 engine it is built on keeps its own in
# CATCH_VERSION_MAJOR and the other macros of Progmasoft/Catch3/EngineVersionMacros.hpp.
project(Catch3
  VERSION 0.3.0.1
  LANGUAGES CXX
  HOMEPAGE_URL "https://github.com/Progmasoft/catch3"
  DESCRIPTION "A C++ test framework with a Catch2-compatible API, a parallel runner, property testing and snapshots."
)

# Provide path for scripts. We first add path to the scripts we don't use,
# but projects including us might, and set the path up to parent scope.
# Then we also add path that we use to configure the project, but is of
# no use to top level projects.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/extras")
if(NOT NOT_SUBPROJECT)
  set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CatchConfigOptions)
if(CATCH3_DEVELOPMENT_BUILD)
  include(CTest)
endif()

# This variable is used in some subdirectories, so we need it here, rather
# than later in the install block
set(CATCH3_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Catch3")

# We have some Windows builds that test `wmain` entry point,
# and we need this change to be present in all binaries that
# are built during these tests, so this is required here, before
# the subdirectories are added.
if(CATCH3_TEST_USE_WMAIN)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:wmainCRTStartup")
endif()

# Basic paths
set(CATCH_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES_DIR ${CATCH_DIR}/src/Progmasoft/Catch3)
set(SELF_TEST_DIR ${CATCH_DIR}/tests/SelfTest)

# yyjson and pugixml, compiled into the library; nothing of them is installed.
add_subdirectory(third_party/yyjson)
add_subdirectory(third_party/pugixml)

# We need to bring-in the variables defined there to this scope
add_subdirectory(src)

if (CATCH3_BUILD_BENCHMARKS)
  set(CMAKE_FOLDER "benchmarks")
  add_subdirectory(benchmarks)
endif()

# Build tests only if requested
if(BUILD_TESTING AND CATCH3_BUILD_TESTING AND NOT_SUBPROJECT)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  if(NOT TARGET Python3::Interpreter)
    message(FATAL_ERROR "Python not found, but required for tests")
  endif()
  set(CMAKE_FOLDER "tests")
  add_subdirectory(tests)
endif()

if(CATCH3_BUILD_EXAMPLES)
  set(CMAKE_FOLDER "Examples")
  add_subdirectory(examples)
endif()

if(CATCH3_BUILD_EXTRA_TESTS)
  set(CMAKE_FOLDER "tests/ExtraTests")
  add_subdirectory(tests/ExtraTests)
endif()

if(CATCH3_BUILD_FUZZERS)
  set(CMAKE_FOLDER "fuzzing")
  add_subdirectory(fuzzing)
endif()

if(CATCH3_DEVELOPMENT_BUILD)
  set(CATCH_ALL_TARGETS ${CATCH_IMPL_TARGETS} ${CATCH_TEST_TARGETS})
  add_warnings_to_targets("${CATCH_ALL_TARGETS}")
  # After we added the noreturn hint to FAIL and SKIP, Clang became
  # extremely good at diagnosing tests that test these macros as being
  # noreturn, but not marked as such. This made the warning useless for
  # our test files.
  add_cxx_flag_if_supported_to_targets("-Wno-missing-noreturn" "${CATCH_TEST_TARGETS}")
endif()

# Only perform the installation steps when Catch is not being used as
# a subproject via `add_subdirectory`, or the destinations will break,
# see https://github.com/catchorg/Catch2/issues/1373
if(NOT_SUBPROJECT)
  configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/CMake/Catch3Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Catch3Config.cmake
    INSTALL_DESTINATION
      ${CATCH3_CMAKE_CONFIG_DESTINATION}
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/Catch3ConfigVersion.cmake"
    COMPATIBILITY
      SameMinorVersion
  )

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/Catch3Config.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/Catch3ConfigVersion.cmake"
    DESTINATION
      ${CATCH3_CMAKE_CONFIG_DESTINATION}
  )

  # Install documentation
  if(CATCH3_INSTALL_DOCS)
    install(
      DIRECTORY
        docs/
      DESTINATION
        "${CMAKE_INSTALL_DOCDIR}"
      PATTERN "doxygen" EXCLUDE
    )
  endif()

  if(CATCH3_INSTALL_EXTRAS)
    # Install CMake scripts
    install(
      FILES
        "extras/Catch3.cmake"
        "extras/Catch3AddTests.cmake"
        "extras/Catch3ShardTests.cmake"
        "extras/Catch3ShardTestsImpl.cmake"
      DESTINATION
        ${CATCH3_CMAKE_CONFIG_DESTINATION}
    )

    # Install debugger helpers
    install(
      FILES
        "extras/gdbinit"
        "extras/lldbinit"
      DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/Catch3
    )
  endif()

  ## Provide some pkg-config integration
  set(PKGCONFIG_INSTALL_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig"
    CACHE PATH "Path where catch3.pc is installed"
  )

  # Generate the pkg-config files
  # To understand the script below, you have to understand that it works in two steps.
  # 1) A CMake script is generated at configuration time
  # 2) It is executed at install time.
  # And both of these have access to different parts of the information we need.
  #
  # Further, the variables before "[[" are expanded at configuration time,
  # while the ones inside the [[]] block are expanded at script execution (install) time.
  string(
    JOIN "\n"
      install_script
       "set(install_pkgconfdir \"${PKGCONFIG_INSTALL_DIR}\")"
       "set(impl_pc_file \"${CMAKE_CURRENT_SOURCE_DIR}/CMake/catch3.pc.in\")"
       "set(main_pc_file \"${CMAKE_CURRENT_SOURCE_DIR}/CMake/catch3-with-main.pc.in\")"
       "set(Catch3_VERSION ${Catch3_VERSION})"
       "set(include_dir \"${CMAKE_INSTALL_INCLUDEDIR}\")"
       "set(lib_dir \"${CMAKE_INSTALL_LIBDIR}\")"
       [[
         message(STATUS "DESTDIR: $ENV{DESTDIR}")
         set(DESTDIR_PREFIX "")
         if (DEFINED ENV{DESTDIR})
           set(DESTDIR_PREFIX "$ENV{DESTDIR}")
         endif ()
         message(STATUS "PREFIX: ${DESTDIR_PREFIX}")
         set(lib_name "$<TARGET_FILE_BASE_NAME:Catch3>")
         configure_file(
           "${impl_pc_file}"
           "${DESTDIR_PREFIX}${CMAKE_INSTALL_PREFIX}/${install_pkgconfdir}/catch3.pc"
           @ONLY
         )

         set(lib_name "$<TARGET_FILE_BASE_NAME:Catch3WithMain>")
         configure_file(
           "${main_pc_file}"
           "${DESTDIR_PREFIX}${CMAKE_INSTALL_PREFIX}/${install_pkgconfdir}/catch3-with-main.pc"
           @ONLY
         )
       ]]
  )
  install(CODE "${install_script}")

  set(CPACK_PACKAGE_CONTACT "support@progmasoft.com")

  include(CPack)
endif()
