-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

local repository_root = path.getabsolute(".")
local generated_include = path.join(repository_root, "build", "premake", "generated")
local configure_catch3 = dofile(path.join(repository_root, "buildsupport", "catch3_config.lua"))
configure_catch3.generate(generated_include, repository_root)

-- yyjson and pugixml, compiled into the library.
include "third_party/yyjson"
include "third_party/pugixml"

workspace "Catch3"
    location "build/premake"
    configurations { "Debug", "Release" }
    startproject "ProgmasoftPropertyTests"

    filter "system:windows"
        architecture "x86_64"
        toolset "clang"
    filter {}

    filter "system:macosx"
        toolset "clang"
    filter {}

    filter "configurations:Debug"
        symbols "On"
        runtime "Debug"
    filter "configurations:Release"
        optimize "Speed"
        runtime "Release"
    filter {}

-- The Catch2-compatible engine, the Catch3 runner, property testing and
-- snapshots; the library and the code that includes it need C++17.
project "Catch3"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/Catch3/%{cfg.buildcfg}"
    files {
        "src/Progmasoft/Catch3/**.cpp",
        "src/Progmasoft/Runner/*.cpp",
        "src/Progmasoft/Runner/*.hpp",
        "src/Progmasoft/Results.cpp",
        "src/Progmasoft/XmlWriter.cpp",
        "src/Progmasoft/ChoiceShrinker.cpp",
        "src/Progmasoft/StrategyContext.cpp",
        "src/Progmasoft/PropertyEngine.cpp",
        "src/Progmasoft/Snapshot.cpp",
        "src/Progmasoft/Catch3/**.hpp",
        "src/Progmasoft/Catch3.hpp",
        path.join(generated_include, "Progmasoft", "Catch3", "UserConfig.hpp"),
    }
    removefiles { "src/Progmasoft/Catch3/Internal/Main.cpp" }
    includedirs { "src", generated_include }
    -- The JSON and XML libraries of the runner are compiled into its own
    -- translation units.
    externalincludedirs { catch3_third_party.yyjson, catch3_third_party.pugixml }

project "Catch3WithMain"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    -- Visual Studio injects UNICODE for Windows projects, which would select
    -- Catch2's wmain entry point instead of the normal console main.
    defines { "DO_NOT_USE_WMAIN" }
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/Catch3WithMain/%{cfg.buildcfg}"
    files { "src/Progmasoft/Catch3/Internal/Main.cpp" }
    includedirs { "src", generated_include }
    links { "Catch3" }

project "ProgmasoftPropertyTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/ProgmasoftPropertyTests/%{cfg.buildcfg}"
    files {
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
        "tests/Progmasoft/RunnerOutput.tests.cpp",
        "tests/Progmasoft/Snapshot.tests.cpp",
        "tests/Progmasoft/Stateful.tests.cpp",
        "tests/Progmasoft/Strategy.tests.cpp",
        "src/Progmasoft/**.hpp",
    }
    includedirs { "src", generated_include }
    links { "Catch3WithMain", "Catch3" }
