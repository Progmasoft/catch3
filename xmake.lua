-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

set_project("Catch3")
set_version("0.3.0.1")
set_languages("cxx17")
set_warnings("all")

local repository_root = os.projectdir()
local generated_include = path.join(repository_root, ".xmake", "generated")
add_moduledirs(path.join(repository_root, "buildsupport"))

-- yyjson and pugixml, compiled into the library.
includes("third_party/yyjson", "third_party/pugixml")

-- The Catch2-compatible engine, the Catch3 runner, which replaced Catch2's
-- session, command line and reporters, property testing and snapshots. The
-- library and the code that includes it need C++17.
target("catch3")
    set_kind("static")
    on_load(function(target)
        -- File I/O is restricted to Xmake's script domain, so generate here
        -- rather than while the declarative project file is being evaluated.
        local configure_catch3 = import("catch3_config")
        configure_catch3.main(generated_include, repository_root)
        target:add("includedirs", generated_include, {public = true})
    end)
    add_files("src/Progmasoft/Catch3/**.cpp")
    remove_files("src/Progmasoft/Catch3/Internal/Main.cpp")
    add_files(
        "src/Progmasoft/Runner/*.cpp",
        "src/Progmasoft/Results.cpp",
        "src/Progmasoft/XmlWriter.cpp",
        "src/Progmasoft/ChoiceShrinker.cpp",
        "src/Progmasoft/Mock.cpp",
        "src/Progmasoft/StrategyContext.cpp",
        "src/Progmasoft/PropertyEngine.cpp",
        "src/Progmasoft/Snapshot.cpp"
    )
    add_includedirs("src", {public = true})
    add_headerfiles("src/(Progmasoft/**.hpp)")
    -- The JSON and XML libraries of the runner are compiled into its own
    -- translation units.
    add_deps("catch3_yyjson", "catch3_pugixml")

target("catch3_main")
    set_kind("static")
    add_files("src/Progmasoft/Catch3/Internal/Main.cpp")
    add_deps("catch3")
    add_includedirs("src", generated_include, {public = true})

target("ProgmasoftPropertyTests")
    set_kind("binary")
    add_files(
        "tests/Progmasoft/Generators.tests.cpp",
        "tests/Progmasoft/Property.tests.cpp",
        "tests/Progmasoft/ForAll.tests.cpp",
        "tests/Progmasoft/Results.tests.cpp",
        "tests/Progmasoft/Runner.tests.cpp",
        "tests/Progmasoft/RunnerUnits.tests.cpp",
        "tests/Progmasoft/RunnerPlanner.tests.cpp",
        "tests/Progmasoft/RunnerOptions.tests.cpp",
        "tests/Progmasoft/RunnerConfig.tests.cpp",
        "tests/Progmasoft/RunnerAnalysis.tests.cpp",
        "tests/Progmasoft/RunnerImport.tests.cpp",
        "tests/Progmasoft/RunnerOutput.tests.cpp",
        "tests/Progmasoft/Snapshot.tests.cpp",
        "tests/Progmasoft/Stateful.tests.cpp",
        "tests/Progmasoft/Strategy.tests.cpp",
        "src/Progmasoft/Catch3/Internal/Main.cpp"
    )
    add_includedirs("src", generated_include)
    -- MSVC's linker does not discover an otherwise unreferenced main() in a
    -- static archive. Compile the compatibility entry point into this test
    -- executable while retaining catch3_main for downstream consumers.
    add_deps("catch3")
