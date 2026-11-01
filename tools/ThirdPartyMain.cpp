// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// bazelisk run //tools:third_party -- --check
// bazelisk run //tools:third_party -- --update
//
// Keeps third_party/ equal to the archives MODULE.bazel pins by version and
// sha256. BUILD.bazel passes each file as <vendored path>=<runfile>.

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rules_cc/cc/runfiles/runfiles.h"
#include "tools/Repository.hpp"

int
main(int argc, char **argv)
{
    using namespace Progmasoft::Catch3::Tools;
    using rules_cc::cc::runfiles::Runfiles;

    std::vector<std::string_view> files;
    std::string_view mode;
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument{ argv[index] };
        if (argument == "--check" || argument == "--update")
        {
            mode = argument;
        }
        else if (argument.find('=') != std::string_view::npos)
        {
            files.push_back(argument);
        }
        else
        {
            mode = {};
            break;
        }
    }
    if (mode.empty())
    {
        std::cerr << "usage: bazelisk run //tools:third_party -- "
                     "--check|--update\n";
        return 2;
    }
    const auto root = RepositoryRoot();
    if (!root)
    {
        std::cerr << "run this tool with bazelisk run\n";
        return 2;
    }
    std::string error;
    const std::unique_ptr<Runfiles> runfiles{
        Runfiles::Create(argv[0], std::string{}, &error)
    };
    if (runfiles == nullptr)
    {
        std::cerr << error << '\n';
        return 2;
    }
    Problems problems;
    for (const auto file : files)
    {
        const auto separator = file.find('=');
        const auto vendored = file.substr(0, separator);
        const auto archived = ReadOrReport(
            runfiles->Rlocation(std::string{ file.substr(separator + 1) }),
            problems);
        if (mode == "--check")
        {
            if (ReadText(*root / vendored) != archived)
            {
                problems.push_back(std::string{ vendored }
                                   + " differs from its pinned archive: run "
                                     "bazelisk run //tools:third_party -- "
                                     "--update");
            }
        }
        else if (!WriteText(*root / vendored, archived))
        {
            problems.push_back("cannot write " + std::string{ vendored });
        }
    }
    return Finish(problems);
}
