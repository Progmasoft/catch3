// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later

// bazelisk run //tools:check

#include <iostream>

#include "tools/Amalgamation.hpp"
#include "tools/Checks.hpp"
#include "tools/Repository.hpp"
#include "tools/Versions.hpp"

int
main(int argc, char **)
{
    using namespace Progmasoft::Catch3::Tools;

    if (argc != 1)
    {
        std::cerr << "usage: bazelisk run //tools:check\n";
        return 2;
    }
    const auto root = RepositoryRoot();
    if (!root)
    {
        std::cerr << "run this tool with bazelisk run\n";
        return 2;
    }
    Problems problems;
    CheckLicenses(*root, problems);
    CheckConvenienceHeaders(*root, problems);
    CheckPragmaOnce(*root, problems);
    CheckDuplicateFilenames(*root, problems);
    CheckVersions(*root, problems);
    const auto amalgamation = GenerateAmalgamation(*root, problems);
    if (problems.empty())
    {
        CheckAmalgamation(*root, amalgamation, problems);
    }
    return Finish(problems);
}
