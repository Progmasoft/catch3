// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// bazelisk run //tools:repo-info [-- --json]
//
// Reports the checkout and the tools on PATH, without changing anything.

#include <iostream>
#include <string_view>

#include "tools/Repository.hpp"
#include "tools/RepositoryReport.hpp"

int
main(int argc, char **argv)
{
    using namespace Progmasoft::Catch3::Tools;

    const bool json = argc == 2 && std::string_view{ argv[1] } == "--json";
    if (argc > 2 || (argc == 2 && !json))
    {
        std::cerr << "usage: bazelisk run //tools:repo-info [-- --json]\n";
        return 2;
    }
    const auto root = RepositoryRoot();
    if (!root)
    {
        std::cerr << "run this tool with bazelisk run\n";
        return 2;
    }
    const auto report = CollectRepositoryReport(*root);
    std::cout << (json ? ReportToJson(report) : FormatReport(report));
    return 0;
}
