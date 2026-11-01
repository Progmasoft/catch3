// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// bazelisk run //tools:version                  prints the current version
// bazelisk run //tools:version -- --next        moves to the next version
// bazelisk run //tools:version -- 0.3.1.0       moves to that version
//
// Moving writes every place that states the version, and the amalgamation.

#include <iostream>
#include <string>
#include <string_view>

#include "tools/Amalgamation.hpp"
#include "tools/Repository.hpp"
#include "tools/Versions.hpp"

int
main(int argc, char **argv)
{
    using namespace Progmasoft::Catch3::Tools;

    if (argc > 2)
    {
        std::cerr
            << "usage: bazelisk run //tools:version [-- --next|<version>]\n";
        return 2;
    }
    const auto root = RepositoryRoot();
    if (!root)
    {
        std::cerr << "run this tool with bazelisk run\n";
        return 2;
    }
    Problems problems;
    const auto current = CurrentVersion(*root, problems);
    if (!current)
    {
        return Finish(problems);
    }
    if (argc == 1)
    {
        std::cout << *current << '\n';
        return 0;
    }
    const std::string_view argument{ argv[1] };
    const auto parsed = ParseVersion(
        argument == "--next" ? *current : std::string{ argument });
    if (!parsed)
    {
        std::cerr << "not a version of three or four numbers: " << argument
                  << '\n';
        return 2;
    }
    const auto version
        = FormatVersion(argument == "--next" ? NextVersion(*parsed) : *parsed);
    SetVersion(*root, version, problems);
    if (problems.empty())
    {
        WriteAmalgamation(*root,
                          GenerateAmalgamation(*root, problems),
                          problems);
    }
    if (problems.empty())
    {
        std::cout << *current << " -> " << version << '\n';
    }
    return Finish(problems);
}
