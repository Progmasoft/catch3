-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: LGPL-3.0-or-later

-- yyjson (MIT), the JSON reader and writer of the runner.
-- The version is pinned in MODULE.bazel. Catch3
-- compiles the library into one of its own translation units with
-- internal linkage, so the workspace only needs its directory.
catch3_third_party = catch3_third_party or {}
catch3_third_party.yyjson = _SCRIPT_DIR
