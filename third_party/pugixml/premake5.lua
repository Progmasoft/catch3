-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

-- pugixml (MIT), the XML writer of the reports.
-- The version is pinned in MODULE.bazel. Catch3
-- compiles the library into one of its own translation units with
-- internal linkage, so the workspace only needs its directory.
catch3_third_party = catch3_third_party or {}
catch3_third_party.pugixml = _SCRIPT_DIR
