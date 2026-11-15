# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: LGPL-3.0-or-later

# yyjson (MIT), the JSON reader and writer of the runner.
# The version is pinned in MODULE.bazel. Catch3
# compiles the library into one of its own translation units with
# internal linkage, so this target only provides the include directory.
# Catch3 links it through $<BUILD_INTERFACE:...>; it is never installed.
add_library(Catch3Yyjson INTERFACE)
target_include_directories(Catch3Yyjson SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
