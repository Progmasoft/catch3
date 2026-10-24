include(CatchMiscFunctions)

# CMake derives a Visual Studio project GUID from the file path but can be overridden via a property
# (see https://gitlab.kitware.com/cmake/cmake/-/commit/c85367f4).  Using a non-constant GUID
# can cause problems if other projects/repos want to reference the vcxproj file,
# so we force a constant GUID here.
set(Catch2_GUID_CMAKE "8d538cbe-01bf-4a2e-a98a-6c368fdf13d7" CACHE INTERNAL "Project GUID")
set(Catch2WithMain_GUID_CMAKE "8bd3552a-2cfb-4a59-ab15-2031b97ada1e" CACHE INTERNAL "Project GUID")

set(BENCHMARK_HEADERS
  ${SOURCES_DIR}/benchmark/catch_benchmark.hpp
  ${SOURCES_DIR}/benchmark/catch_benchmark_all.hpp
  ${SOURCES_DIR}/benchmark/catch_chronometer.hpp
  ${SOURCES_DIR}/benchmark/catch_clock.hpp
  ${SOURCES_DIR}/benchmark/catch_constructor.hpp
  ${SOURCES_DIR}/benchmark/catch_environment.hpp
  ${SOURCES_DIR}/benchmark/catch_estimate.hpp
  ${SOURCES_DIR}/benchmark/catch_execution_plan.hpp
  ${SOURCES_DIR}/benchmark/catch_optimizer.hpp
  ${SOURCES_DIR}/benchmark/catch_outlier_classification.hpp
  ${SOURCES_DIR}/benchmark/catch_sample_analysis.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_analyse.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_benchmark_function.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_benchmark_stats.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_benchmark_stats_fwd.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_complete_invoke.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_estimate_clock.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_measure.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_repeat.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_run_for_at_least.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_stats.hpp
  ${SOURCES_DIR}/benchmark/detail/catch_timing.hpp
)
set(BENCHMARK_SOURCES
  ${SOURCES_DIR}/benchmark/catch_benchmark.cpp
  ${SOURCES_DIR}/benchmark/catch_chronometer.cpp
  ${SOURCES_DIR}/benchmark/detail/catch_analyse.cpp
  ${SOURCES_DIR}/benchmark/detail/catch_benchmark_function.cpp
  ${SOURCES_DIR}/benchmark/detail/catch_run_for_at_least.cpp
  ${SOURCES_DIR}/benchmark/detail/catch_stats.cpp
)
set(BENCHMARK_FILES ${BENCHMARK_HEADERS} ${BENCHMARK_SOURCES})

set(IMPL_HEADERS
 "${PROJECT_BINARY_DIR}/generated-includes/catch2/catch_user_config.hpp"
  ${SOURCES_DIR}/catch_user_config.hpp.in
  ${SOURCES_DIR}/catch_all.hpp
  ${SOURCES_DIR}/catch_approx.hpp
  ${SOURCES_DIR}/catch_assertion_info.hpp
  ${SOURCES_DIR}/catch_assertion_result.hpp
  ${SOURCES_DIR}/catch_case_sensitive.hpp
  ${SOURCES_DIR}/catch_config.hpp
  ${SOURCES_DIR}/catch_get_random_seed.hpp
  ${SOURCES_DIR}/catch_message.hpp
  ${SOURCES_DIR}/catch_section_info.hpp
  ${SOURCES_DIR}/catch_session.hpp
  ${SOURCES_DIR}/catch_tag_alias.hpp
  ${SOURCES_DIR}/catch_tag_alias_autoregistrar.hpp
  ${SOURCES_DIR}/catch_template_test_macros.hpp
  ${SOURCES_DIR}/catch_test_case_info.hpp
  ${SOURCES_DIR}/catch_test_macros.hpp
  ${SOURCES_DIR}/catch_test_run_info.hpp
  ${SOURCES_DIR}/catch_test_spec.hpp
  ${SOURCES_DIR}/catch_timer.hpp
  ${SOURCES_DIR}/catch_tostring.hpp
  ${SOURCES_DIR}/catch_totals.hpp
  ${SOURCES_DIR}/catch_translate_exception.hpp
  ${SOURCES_DIR}/catch_version.hpp
  ${SOURCES_DIR}/catch_version_macros.hpp
  ${SOURCES_DIR}/internal/catch_assertion_handler.hpp
  ${SOURCES_DIR}/internal/catch_case_insensitive_comparisons.hpp
  ${SOURCES_DIR}/internal/catch_compare_traits.hpp
  ${SOURCES_DIR}/internal/catch_compiler_capabilities.hpp
  ${SOURCES_DIR}/internal/catch_config_android_logwrite.hpp
  ${SOURCES_DIR}/internal/catch_config_counter.hpp
  ${SOURCES_DIR}/internal/catch_config_prefix_messages.hpp
  ${SOURCES_DIR}/internal/catch_config_static_analysis_support.hpp
  ${SOURCES_DIR}/internal/catch_config_uncaught_exceptions.hpp
  ${SOURCES_DIR}/internal/catch_config_wchar.hpp
  ${SOURCES_DIR}/internal/catch_container_nonmembers.hpp
  ${SOURCES_DIR}/internal/catch_context.hpp
  ${SOURCES_DIR}/internal/catch_debugger.hpp
  ${SOURCES_DIR}/internal/catch_decomposer.hpp
  ${SOURCES_DIR}/internal/catch_deprecation_macro.hpp
  ${SOURCES_DIR}/internal/catch_enforce.hpp
  ${SOURCES_DIR}/internal/catch_enum_info.hpp
  ${SOURCES_DIR}/internal/catch_errno_guard.hpp
  ${SOURCES_DIR}/internal/catch_exception_translator_registry.hpp
  ${SOURCES_DIR}/internal/catch_fatal_condition_handler.hpp
  ${SOURCES_DIR}/internal/catch_floating_point_helpers.hpp
  ${SOURCES_DIR}/internal/catch_getenv.hpp
  ${SOURCES_DIR}/internal/catch_is_permutation.hpp
  ${SOURCES_DIR}/internal/catch_lazy_expr.hpp
  ${SOURCES_DIR}/internal/catch_leak_detector.hpp
  ${SOURCES_DIR}/internal/catch_lifetimebound.hpp
  ${SOURCES_DIR}/internal/catch_logical_traits.hpp
  ${SOURCES_DIR}/internal/catch_message_info.hpp
  ${SOURCES_DIR}/internal/catch_meta.hpp
  ${SOURCES_DIR}/internal/catch_move_and_forward.hpp
  ${SOURCES_DIR}/internal/catch_noncopyable.hpp
  ${SOURCES_DIR}/internal/catch_optional.hpp
  ${SOURCES_DIR}/internal/catch_output_redirect.hpp
  ${SOURCES_DIR}/internal/catch_parse_numbers.hpp
  ${SOURCES_DIR}/internal/catch_path_filter.hpp
  ${SOURCES_DIR}/internal/catch_platform.hpp
  ${SOURCES_DIR}/internal/catch_polyfills.hpp
  ${SOURCES_DIR}/internal/catch_preprocessor.hpp
  ${SOURCES_DIR}/internal/catch_preprocessor_remove_parens.hpp
  ${SOURCES_DIR}/internal/catch_random_floating_point_helpers.hpp
  ${SOURCES_DIR}/internal/catch_random_integer_helpers.hpp
  ${SOURCES_DIR}/internal/catch_random_number_generator.hpp
  ${SOURCES_DIR}/internal/catch_random_seed_generation.hpp
  ${SOURCES_DIR}/internal/catch_reporter_registry.hpp
  ${SOURCES_DIR}/internal/catch_result_type.hpp
  ${SOURCES_DIR}/internal/catch_reusable_string_stream.hpp
  ${SOURCES_DIR}/internal/catch_run_context.hpp
  ${SOURCES_DIR}/internal/catch_section.hpp
  ${SOURCES_DIR}/internal/catch_singletons.hpp
  ${SOURCES_DIR}/internal/catch_source_line_info.hpp
  ${SOURCES_DIR}/internal/catch_startup_exception_registry.hpp
  ${SOURCES_DIR}/internal/catch_stdstreams.hpp
  ${SOURCES_DIR}/internal/catch_stream_end_stop.hpp
  ${SOURCES_DIR}/internal/catch_string_manip.hpp
  ${SOURCES_DIR}/internal/catch_stringref.hpp
  ${SOURCES_DIR}/internal/catch_tag_alias_registry.hpp
  ${SOURCES_DIR}/internal/catch_template_test_registry.hpp
  ${SOURCES_DIR}/internal/catch_test_case_info_hasher.hpp
  ${SOURCES_DIR}/internal/catch_test_case_registry_impl.hpp
  ${SOURCES_DIR}/internal/catch_test_case_tracker.hpp
  ${SOURCES_DIR}/internal/catch_test_failure_exception.hpp
  ${SOURCES_DIR}/internal/catch_test_macro_impl.hpp
  ${SOURCES_DIR}/internal/catch_test_registry.hpp
  ${SOURCES_DIR}/internal/catch_test_spec_parser.hpp
  ${SOURCES_DIR}/internal/catch_thread_local.hpp
  ${SOURCES_DIR}/internal/catch_thread_support.hpp
  ${SOURCES_DIR}/internal/catch_to_string.hpp
  ${SOURCES_DIR}/internal/catch_uncaught_exceptions.hpp
  ${SOURCES_DIR}/internal/catch_uniform_floating_point_distribution.hpp
  ${SOURCES_DIR}/internal/catch_uniform_integer_distribution.hpp
  ${SOURCES_DIR}/internal/catch_unique_name.hpp
  ${SOURCES_DIR}/internal/catch_unique_ptr.hpp
  ${SOURCES_DIR}/internal/catch_unreachable.hpp
  ${SOURCES_DIR}/internal/catch_void_type.hpp
  ${SOURCES_DIR}/internal/catch_wildcard_pattern.hpp
  ${SOURCES_DIR}/internal/catch_windows_h_proxy.hpp
)
set(IMPL_SOURCES
  ${SOURCES_DIR}/catch_approx.cpp
  ${SOURCES_DIR}/catch_assertion_result.cpp
  ${SOURCES_DIR}/catch_config.cpp
  ${SOURCES_DIR}/catch_get_random_seed.cpp
  ${SOURCES_DIR}/catch_message.cpp
  ${SOURCES_DIR}/catch_registry_hub.cpp
  ${SOURCES_DIR}/catch_session.cpp
  ${SOURCES_DIR}/catch_tag_alias_autoregistrar.cpp
  ${SOURCES_DIR}/catch_test_case_info.cpp
  ${SOURCES_DIR}/catch_test_spec.cpp
  ${SOURCES_DIR}/catch_timer.cpp
  ${SOURCES_DIR}/catch_tostring.cpp
  ${SOURCES_DIR}/catch_totals.cpp
  ${SOURCES_DIR}/catch_translate_exception.cpp
  ${SOURCES_DIR}/catch_version.cpp
  ${SOURCES_DIR}/internal/catch_assertion_handler.cpp
  ${SOURCES_DIR}/internal/catch_case_insensitive_comparisons.cpp
  ${SOURCES_DIR}/internal/catch_context.cpp
  ${SOURCES_DIR}/internal/catch_debugger.cpp
  ${SOURCES_DIR}/internal/catch_decomposer.cpp
  ${SOURCES_DIR}/internal/catch_enforce.cpp
  ${SOURCES_DIR}/internal/catch_enum_info.cpp
  ${SOURCES_DIR}/internal/catch_errno_guard.cpp
  ${SOURCES_DIR}/internal/catch_exception_translator_registry.cpp
  ${SOURCES_DIR}/internal/catch_fatal_condition_handler.cpp
  ${SOURCES_DIR}/internal/catch_floating_point_helpers.cpp
  ${SOURCES_DIR}/internal/catch_getenv.cpp
  ${SOURCES_DIR}/internal/catch_lazy_expr.cpp
  ${SOURCES_DIR}/internal/catch_leak_detector.cpp
  ${SOURCES_DIR}/internal/catch_message_info.cpp
  ${SOURCES_DIR}/internal/catch_output_redirect.cpp
  ${SOURCES_DIR}/internal/catch_parse_numbers.cpp
  ${SOURCES_DIR}/internal/catch_polyfills.cpp
  ${SOURCES_DIR}/internal/catch_random_number_generator.cpp
  ${SOURCES_DIR}/internal/catch_random_seed_generation.cpp
  ${SOURCES_DIR}/internal/catch_reporter_registry.cpp
  ${SOURCES_DIR}/internal/catch_reusable_string_stream.cpp
  ${SOURCES_DIR}/internal/catch_run_context.cpp
  ${SOURCES_DIR}/internal/catch_section.cpp
  ${SOURCES_DIR}/internal/catch_singletons.cpp
  ${SOURCES_DIR}/internal/catch_source_line_info.cpp
  ${SOURCES_DIR}/internal/catch_startup_exception_registry.cpp
  ${SOURCES_DIR}/internal/catch_stdstreams.cpp
  ${SOURCES_DIR}/internal/catch_string_manip.cpp
  ${SOURCES_DIR}/internal/catch_stringref.cpp
  ${SOURCES_DIR}/internal/catch_tag_alias_registry.cpp
  ${SOURCES_DIR}/internal/catch_test_case_info_hasher.cpp
  ${SOURCES_DIR}/internal/catch_test_case_registry_impl.cpp
  ${SOURCES_DIR}/internal/catch_test_case_tracker.cpp
  ${SOURCES_DIR}/internal/catch_test_failure_exception.cpp
  ${SOURCES_DIR}/internal/catch_test_registry.cpp
  ${SOURCES_DIR}/internal/catch_test_spec_parser.cpp
  ${SOURCES_DIR}/internal/catch_uncaught_exceptions.cpp
  ${SOURCES_DIR}/internal/catch_wildcard_pattern.cpp
)
set(INTERNAL_FILES ${IMPL_SOURCES} ${IMPL_HEADERS})

set(INTERFACE_HEADERS
  ${SOURCES_DIR}/interfaces/catch_interfaces_all.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_capture.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_config.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_exception.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_generatortracker.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_registry_hub.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_reporter.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_reporter_factory.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_tag_alias_registry.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_test_invoker.hpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_testcase.hpp
)
set(INTERFACE_SOURCES
  ${SOURCES_DIR}/interfaces/catch_interfaces_capture.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_config.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_exception.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_generatortracker.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_registry_hub.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_reporter.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_reporter_factory.cpp
  ${SOURCES_DIR}/interfaces/catch_interfaces_testcase.cpp
)
set(INTERFACE_FILES ${INTERFACE_HEADERS} ${INTERFACE_SOURCES})

set(GENERATOR_HEADERS
  ${SOURCES_DIR}/generators/catch_generator_exception.hpp
  ${SOURCES_DIR}/generators/catch_generators.hpp
  ${SOURCES_DIR}/generators/catch_generators_adapters.hpp
  ${SOURCES_DIR}/generators/catch_generators_all.hpp
  ${SOURCES_DIR}/generators/catch_generators_random.hpp
  ${SOURCES_DIR}/generators/catch_generators_range.hpp
  ${SOURCES_DIR}/generators/catch_generators_throw.hpp
)
set(GENERATOR_SOURCES
  ${SOURCES_DIR}/generators/catch_generator_exception.cpp
  ${SOURCES_DIR}/generators/catch_generators.cpp
  ${SOURCES_DIR}/generators/catch_generators_random.cpp
  ${SOURCES_DIR}/generators/catch_generators_throw.cpp
)
set(GENERATOR_FILES ${GENERATOR_HEADERS} ${GENERATOR_SOURCES})

set(MATCHER_HEADERS
  ${SOURCES_DIR}/matchers/catch_matchers.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_all.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_container_properties.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_contains.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_range_equals.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_exception.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_floating_point.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_predicate.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_quantifiers.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_string.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_templated.hpp
  ${SOURCES_DIR}/matchers/catch_matchers_vector.hpp
  ${SOURCES_DIR}/matchers/internal/catch_matchers_impl.hpp
)
set(MATCHER_SOURCES
  ${SOURCES_DIR}/matchers/catch_matchers.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_container_properties.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_exception.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_floating_point.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_predicate.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_quantifiers.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_string.cpp
  ${SOURCES_DIR}/matchers/catch_matchers_templated.cpp
  ${SOURCES_DIR}/matchers/internal/catch_matchers_impl.cpp
)
set(MATCHER_FILES ${MATCHER_HEADERS} ${MATCHER_SOURCES})

set(REPORTER_HEADERS
  ${SOURCES_DIR}/reporters/catch_reporter_event_listener.hpp
  ${SOURCES_DIR}/reporters/catch_reporter_multi.hpp
  ${SOURCES_DIR}/reporters/catch_reporter_registrars.hpp
  ${SOURCES_DIR}/reporters/catch_reporters_all.hpp
)
set(REPORTER_SOURCES
  ${SOURCES_DIR}/reporters/catch_reporter_event_listener.cpp
  ${SOURCES_DIR}/reporters/catch_reporter_multi.cpp
  ${SOURCES_DIR}/reporters/catch_reporter_registrars.cpp
)
set(REPORTER_FILES ${REPORTER_HEADERS} ${REPORTER_SOURCES})

# The Catch3 runner, which replaced Catch2's session, command line and
# reporters. It is built into Catch2 so that Catch::Session reaches it.
set(PROGMASOFT_RUNNER_FILES
  "${CATCH_DIR}/src/Progmasoft/Catch3/Results.hpp"
  "${CATCH_DIR}/src/Progmasoft/Catch3/Runner.hpp"
  "${CATCH_DIR}/src/Progmasoft/Catch3/Version.hpp"
  "${CATCH_DIR}/src/Progmasoft/Catch3/XmlWriter.hpp"
  "${CATCH_DIR}/src/Progmasoft/Results.cpp"
  "${CATCH_DIR}/src/Progmasoft/XmlWriter.cpp"
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
  "${CATCH_DIR}/src/Progmasoft/Runner/Options.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Planner.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Process.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Protocol.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/ResultsJson.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Runner.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/Scheduler.cpp"
  "${CATCH_DIR}/src/Progmasoft/Runner/TemporaryDirectory.cpp"
)

set(ALL_FILES
  ${BENCHMARK_FILES}
  ${GENERATOR_FILES}
  ${REPORTER_FILES}
  ${INTERFACE_FILES}
  ${INTERNAL_FILES}
  ${MATCHER_FILES}
  ${PROGMASOFT_RUNNER_FILES}
)

set(FILTERED_FILES ${ALL_FILES})
list(REMOVE_ITEM FILTERED_FILES "${PROJECT_BINARY_DIR}/generated-includes/catch2/catch_user_config.hpp")
list(REMOVE_ITEM FILTERED_FILES ${PROGMASOFT_RUNNER_FILES})
source_group(
  TREE ${SOURCES_DIR}
  PREFIX sources
  FILES ${FILTERED_FILES}
)
source_group(
  TREE "${CATCH_DIR}/src/Progmasoft"
  PREFIX runner
  FILES ${PROGMASOFT_RUNNER_FILES}
)
source_group("generated headers"
  FILES
    "${PROJECT_BINARY_DIR}/generated-includes/catch2/catch_user_config.hpp"
)

add_library(Catch2 ${ALL_FILES})
if(CATCH_ENABLE_REPRODUCIBLE_BUILD)
  add_build_reproducibility_settings(Catch2)
endif()
add_library(Catch2::Catch2 ALIAS Catch2)

# The property engine and the other Progmasoft test utilities. Their headers
# need C++20 in the code that includes them.
add_library(ProgmasoftCatch3 STATIC
  "${CATCH_DIR}/src/Progmasoft/ChoiceShrinker.cpp"
  "${CATCH_DIR}/src/Progmasoft/Context.cpp"
  "${CATCH_DIR}/src/Progmasoft/PropertyEngine.cpp"
  "${CATCH_DIR}/src/Progmasoft/Snapshot.cpp"
)
add_library(Progmasoft::Catch3 ALIAS ProgmasoftCatch3)
set_target_properties(ProgmasoftCatch3 PROPERTIES EXPORT_NAME Catch3)
target_link_libraries(ProgmasoftCatch3 PUBLIC Catch2::Catch2)
target_include_directories(ProgmasoftCatch3
  PUBLIC
    $<BUILD_INTERFACE:${CATCH_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(ProgmasoftCatch3 PUBLIC cxx_std_20)

if(ANDROID)
  target_link_libraries(Catch2 PRIVATE log)
endif()

set_target_properties(Catch2 PROPERTIES
  DEBUG_POSTFIX "d"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION}
)

# Code that includes Catch2's headers needs C++14; the library itself, with
# the Catch3 runner, is built as C++20.
target_compile_features(Catch2 PUBLIC cxx_std_14)
set_target_properties(Catch2 PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

configure_file(
  "${SOURCES_DIR}/catch_user_config.hpp.in"
  "${PROJECT_BINARY_DIR}/generated-includes/catch2/catch_user_config.hpp"
)

target_include_directories(Catch2
  PUBLIC
    $<BUILD_INTERFACE:${SOURCES_DIR}/..>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated-includes>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


add_library(Catch2WithMain
  ${SOURCES_DIR}/internal/catch_main.cpp
)
if(CATCH_ENABLE_REPRODUCIBLE_BUILD)
  add_build_reproducibility_settings(Catch2WithMain)
endif()
add_library(Catch2::Catch2WithMain ALIAS Catch2WithMain)
target_link_libraries(Catch2WithMain PUBLIC Catch2)
set_target_properties(Catch2WithMain
  PROPERTIES
    OUTPUT_NAME "Catch2Main"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
)
set_target_properties(Catch2WithMain PROPERTIES DEBUG_POSTFIX "d")

if(NOT_SUBPROJECT)
  # create and install an export set for catch target as Catch2::Catch
  install(
    TARGETS
      Catch2
      Catch2WithMain
    EXPORT
      Catch2Targets
    LIBRARY DESTINATION
      ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION
      ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION
      ${CMAKE_INSTALL_BINDIR}
  )

  install(
    EXPORT
      Catch2Targets
    NAMESPACE
      Catch2::
    DESTINATION
      ${CATCH_CMAKE_CONFIG_DESTINATION}
  )
  # Install the headers
  install(
    DIRECTORY
      "${SOURCES_DIR}"
      "${PROJECT_BINARY_DIR}/generated-includes/catch2" # Also install the generated header
    DESTINATION
      "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING
      PATTERN "*.hpp"
  )

  # Keep the independently licensed Progmasoft headers beside Catch2's public
  # headers while preserving a separate CMake target and package namespace.
  install(
    DIRECTORY "${CATCH_DIR}/src/Progmasoft"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.hpp"
  )
  install(TARGETS ProgmasoftCatch3 EXPORT ProgmasoftCatch3Targets)
  install(
    EXPORT ProgmasoftCatch3Targets
    NAMESPACE Progmasoft::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ProgmasoftCatch3"
  )
  configure_package_config_file(
    "${CATCH_DIR}/src/Progmasoft/ProgmasoftCatch3Config.cmake.in"
    "${PROJECT_BINARY_DIR}/ProgmasoftCatch3Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ProgmasoftCatch3"
  )
  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/ProgmasoftCatch3ConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
  )
  install(
    FILES
      "${PROJECT_BINARY_DIR}/ProgmasoftCatch3Config.cmake"
      "${PROJECT_BINARY_DIR}/ProgmasoftCatch3ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ProgmasoftCatch3"
  )
endif()

# Some tests require a full recompilation of Catch2 lib with different
# compilation flags. They can link against this target to recompile all
# the sources into the binary.
if(CATCH_BUILD_EXAMPLES OR CATCH_BUILD_EXTRA_TESTS)
  add_library(Catch2_buildall_interface INTERFACE)
  target_sources(Catch2_buildall_interface INTERFACE
    ${ALL_FILES}
    # Also include main entry point
    ${SOURCES_DIR}/internal/catch_main.cpp
  )
  target_include_directories(Catch2_buildall_interface
    INTERFACE
      $<BUILD_INTERFACE:${SOURCES_DIR}/..>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated-includes>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_compile_definitions(Catch2_buildall_interface INTERFACE CATCH_CONFIG_STATIC)
  # These targets compile the library's sources themselves, the runner's
  # among them, which need C++20.
  target_compile_features(Catch2_buildall_interface INTERFACE cxx_std_20)
endif()

list(APPEND CATCH_IMPL_TARGETS Catch2 Catch2WithMain)
set(CATCH_IMPL_TARGETS ${CATCH_IMPL_TARGETS} PARENT_SCOPE)

# We still do not support building dynamic library with hidden visibility
# so we want to check & warn users if they do this. However, we won't abort
# the configuration step so that we don't have to also provide an override.
if(BUILD_SHARED_LIBS)
  set_target_properties(Catch2 Catch2WithMain
    PROPERTIES
      WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

  get_target_property(_VisPreset Catch2 CXX_VISIBILITY_PRESET)
  if(NOT MSVC AND _VisPreset STREQUAL "hidden")
    set_target_properties(Catch2 Catch2WithMain
      PROPERTIES
        CXX_VISIBILITY_PRESET "default"
        VISIBILITY_INLINES_HIDDEN OFF
    )
    message(WARNING "Setting Catch2's visibility to default."
                    " Hidden visibility is not supported.")
  endif()
endif()
