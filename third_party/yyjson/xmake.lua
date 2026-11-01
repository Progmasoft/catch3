-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

-- yyjson (MIT), the JSON reader and writer of the runner.
-- The version is pinned in MODULE.bazel. Catch3
-- compiles the library into one of its own translation units with
-- internal linkage, so this target only provides the files.
target("catch3_yyjson")
    set_kind("headeronly")
    add_includedirs(os.scriptdir(), {public = true})
