# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

# pugixml (MIT), the XML writer of the reports.
# The version is pinned in MODULE.bazel. Catch3
# compiles the library into one of its own translation units with
# internal linkage, so this only provides the include directory.
pugixml_dep = declare_dependency(
  include_directories: include_directories('.', is_system: true),
)
