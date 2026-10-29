-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

-- yyjson (MIT), the JSON reader and writer of the runner.
-- The version is pinned in tools/scripts/updateThirdParty.py. Catch3
-- compiles the library into one of its own translation units with
-- internal linkage, so the workspace only needs its directory.
catch3_third_party = catch3_third_party or {}
catch3_third_party.yyjson = _SCRIPT_DIR
