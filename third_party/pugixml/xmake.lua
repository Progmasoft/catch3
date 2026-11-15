-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: LGPL-3.0-or-later

-- pugixml (MIT), the XML writer of the reports.
-- The version is pinned in MODULE.bazel. Catch3
-- compiles the library into one of its own translation units with
-- internal linkage, so this target only provides the files.
target("catch3_pugixml")
    set_kind("headeronly")
    add_includedirs(os.scriptdir(), {public = true})
