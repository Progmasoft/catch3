-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

-- Materialize Catch2's checked-in CMake template for non-CMake build tools.
-- Keeping the template as the source of truth avoids maintaining a second
-- copy of upstream configuration declarations under a different license.
local M = {}

local function read_file(file_path)
    local file = assert(io.open(file_path, "rb"), "cannot read " .. file_path)
    local contents = file:read("*a")
    file:close()
    return contents
end

local function write_file(file_path, contents)
    local file = assert(io.open(file_path, "wb"), "cannot write " .. file_path)
    file:write(contents)
    file:close()
end

--- Generates the default Catch2 user configuration header under output_root.
--- The generated path is output_root/catch2/catch_user_config.hpp.
function M.generate(output_root, repository_root)
    local template_path = repository_root .. "/src/catch2/catch_user_config.hpp.in"
    local template = read_file(template_path)

    -- Neither Xmake nor Premake interprets CMake's #cmakedefine directives.
    -- All feature toggles stay at Catch2's defaults; the two value settings
    -- match Catch2's own CMake defaults and the root Bazel configuration.
    local configured = template:gsub(
        "#cmakedefine%s+([%w_]+)%s*([^\r\n]*)",
        function(name)
            if name == "CATCH_CONFIG_DEFAULT_REPORTER" then
                return '#define CATCH_CONFIG_DEFAULT_REPORTER "console"'
            elseif name == "CATCH_CONFIG_CONSOLE_WIDTH" then
                return "#define CATCH_CONFIG_CONSOLE_WIDTH 80"
            end
            return "/* #undef " .. name .. " */"
        end
    )

    configured = configured:gsub('"@CATCH_CONFIG_DEFAULT_REPORTER@"', '"console"')
    configured = configured:gsub("@CATCH_CONFIG_CONSOLE_WIDTH@", "80")
    configured = configured:gsub("@CATCH_CONFIG_FALLBACK_STRINGIFIER@", "")

    local output_dir = output_root .. "/catch2"
    os.mkdir(output_dir)
    write_file(output_dir .. "/catch_user_config.hpp", configured)
end

-- Xmake imports call the module's main entry point; Premake can use generate.
function main(output_root, repository_root)
    return M.generate(output_root, repository_root)
end

return M
