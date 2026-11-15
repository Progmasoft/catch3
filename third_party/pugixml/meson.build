# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: LGPL-3.0-or-later

# pugixml (MIT), the XML writer of the reports.
# The version is pinned in MODULE.bazel. Catch3
# compiles the library into one of its own translation units with
# internal linkage, so this only provides the include directory.
pugixml_dep = declare_dependency(
  include_directories: include_directories('.', is_system: true),
)
