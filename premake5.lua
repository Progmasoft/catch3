-- Copyright (c) 2026 Progmasoft.
-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

local repository_root = path.getabsolute(".")
local generated_include = path.join(repository_root, "build", "premake", "generated")
local configure_catch2 = dofile(path.join(repository_root, "buildsupport", "catch2_config.lua"))
configure_catch2.generate(generated_include, repository_root)

workspace "Catch3"
    location "build/premake"
    configurations { "Debug", "Release" }
    startproject "ProgmasoftPropertyTests"

    filter "system:windows"
        architecture "x86_64"
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

project "Catch2"
    kind "StaticLib"
    language "C++"
    cppdialect "C++14"
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/Catch2/%{cfg.buildcfg}"
    files {
        "src/catch2/**.cpp",
        "src/catch2/**.hpp",
        path.join(generated_include, "catch2", "catch_user_config.hpp"),
    }
    removefiles { "src/catch2/internal/catch_main.cpp" }
    includedirs { "src", generated_include }

project "Catch2WithMain"
    kind "StaticLib"
    language "C++"
    cppdialect "C++14"
    -- Visual Studio injects UNICODE for Windows projects, which would select
    -- Catch2's wmain entry point instead of the normal console main.
    defines { "DO_NOT_USE_WMAIN" }
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/Catch2WithMain/%{cfg.buildcfg}"
    files { "src/catch2/internal/catch_main.cpp" }
    includedirs { "src", generated_include }
    links { "Catch2" }

project "ProgmasoftCatch3"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/ProgmasoftCatch3/%{cfg.buildcfg}"
    files {
        "src/Progmasoft/Results.cpp",
        "src/Progmasoft/Results.hpp",
        "src/Progmasoft/Snapshot.cpp",
        "src/Progmasoft/Snapshot.hpp",
        "src/Progmasoft/XmlWriter.cpp",
        "src/Progmasoft/XmlWriter.hpp",
        "src/Progmasoft/Catch3/**.hpp",
        "src/Progmasoft/Catch3.hpp",
    }
    includedirs { "src", generated_include }
    links { "Catch2" }

project "ProgmasoftPropertyTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir "build/premake/bin/%{cfg.buildcfg}"
    objdir "build/premake/obj/ProgmasoftPropertyTests/%{cfg.buildcfg}"
    files {
        "tests/Progmasoft/Property.tests.cpp",
        "tests/Progmasoft/Results.tests.cpp",
        "tests/Progmasoft/Snapshot.tests.cpp",
        "src/Progmasoft/**.hpp",
    }
    includedirs { "src", generated_include }
    links { "ProgmasoftCatch3", "Catch2WithMain", "Catch2" }
