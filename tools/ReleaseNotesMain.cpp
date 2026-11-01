// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// bazelisk run //tools:release_notes -- [version]
//
// Prints a section of docs/release-notes.md, of the current version without
// an argument, for the text of a GitHub release.

#include <iostream>
#include <string>

#include "tools/Repository.hpp"
#include "tools/Versions.hpp"

int
main(int argc, char **argv)
{
    using namespace Progmasoft::Catch3::Tools;

    if (argc > 2)
    {
        std::cerr << "usage: bazelisk run //tools:release_notes -- [version]\n";
        return 2;
    }
    const auto root = RepositoryRoot();
    if (!root)
    {
        std::cerr << "run this tool with bazelisk run\n";
        return 2;
    }
    Problems problems;
    const auto version = argc == 2 ? std::optional<std::string>{ argv[1] }
                                   : CurrentVersion(*root, problems);
    const auto notes
        = ReadOrReport(*root / "docs" / "release-notes.md", problems);
    if (!version || !problems.empty())
    {
        return Finish(problems);
    }
    const auto section = ReleaseNotesOf(notes, *version);
    if (!section)
    {
        problems.push_back("no section for " + *version
                           + " in docs/release-notes.md");
        return Finish(problems);
    }
    std::cout << *section;
    return 0;
}
