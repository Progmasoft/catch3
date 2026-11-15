// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// bazelisk run //tools:amalgamate [-- --check]

#include <iostream>
#include <string_view>

#include "tools/Amalgamation.hpp"
#include "tools/Repository.hpp"

int
main(int argc, char **argv)
{
    using namespace Progmasoft::Catch3::Tools;

    const bool check = argc == 2 && std::string_view{ argv[1] } == "--check";
    if (argc > 2 || (argc == 2 && !check))
    {
        std::cerr << "usage: bazelisk run //tools:amalgamate [-- --check]\n";
        return 2;
    }
    const auto root = RepositoryRoot();
    if (!root)
    {
        std::cerr << "run this tool with bazelisk run\n";
        return 2;
    }
    Problems problems;
    const auto amalgamation = GenerateAmalgamation(*root, problems);
    if (problems.empty())
    {
        if (check)
        {
            CheckAmalgamation(*root, amalgamation, problems);
        }
        else
        {
            WriteAmalgamation(*root, amalgamation, problems);
        }
    }
    return Finish(problems);
}
