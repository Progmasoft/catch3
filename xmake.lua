-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

set_project("Catch3")
set_version("3.16.0")
set_languages("cxx14")
set_warnings("all")

local repository_root = os.projectdir()
local generated_include = path.join(repository_root, ".xmake", "generated")
add_moduledirs(path.join(repository_root, "buildsupport"))

-- Keep upstream Catch2's compilation baseline and project identity intact.
target("catch2")
    set_kind("static")
    set_languages("cxx14")
    on_load(function(target)
        -- File I/O is restricted to Xmake's script domain, so generate here
        -- rather than while the declarative project file is being evaluated.
        local configure_catch2 = import("catch2_config")
        configure_catch2.main(generated_include, repository_root)
        target:add("includedirs", generated_include, {public = true})
    end)
    add_files("src/catch2/**.cpp")
    remove_files("src/catch2/internal/catch_main.cpp")
    add_includedirs("src", {public = true})
    add_headerfiles("src/catch2/**.hpp", {prefixdir = ""})

target("catch2_main")
    set_kind("static")
    set_languages("cxx14")
    add_files("src/catch2/internal/catch_main.cpp")
    add_deps("catch2")
    add_includedirs("src", generated_include, {public = true})

-- Compile Progmasoft's implementation separately from the compatibility engine.
target("ProgmasoftCatch3")
    set_kind("static")
    set_languages("cxx20")
    add_headerfiles("src/Progmasoft/**.hpp", {prefixdir = ""})
    add_files(
        "src/Progmasoft/Results.cpp",
        "src/Progmasoft/Snapshot.cpp",
        "src/Progmasoft/XmlWriter.cpp"
    )
    add_includedirs("src", {public = true})
    add_deps("catch2", {public = true})

target("ProgmasoftPropertyTests")
    set_kind("binary")
    set_languages("cxx20")
    add_files(
        "tests/Progmasoft/Property.tests.cpp",
        "tests/Progmasoft/Results.tests.cpp",
        "tests/Progmasoft/Snapshot.tests.cpp"
    )
    add_includedirs("src", generated_include)
    add_deps("ProgmasoftCatch3", "catch2_main")
