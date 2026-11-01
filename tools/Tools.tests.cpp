// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <Progmasoft/Catch3/TestMacros.hpp>
#include <string>

#include "tools/Amalgamation.hpp"
#include "tools/Repository.hpp"
#include "tools/RepositoryReport.hpp"
#include "tools/Versions.hpp"

using namespace Progmasoft::Catch3::Tools;

TEST_CASE("Versions have three or four numbers", "[tools]")
{
    CHECK(ParseVersion("0.3.0") == Version{ 0, 3, 0, 0 });
    CHECK(ParseVersion("0.3.0.2") == Version{ 0, 3, 0, 2 });
    for (const char *wrong :
         { "0.3", "0.3.0.2.1", "01.3.0", "0.3.0.", "a.3.0", "", "0.3.0-rc" })
    {
        INFO(wrong);
        CHECK_FALSE(ParseVersion(wrong));
    }
}

TEST_CASE("The next version counts releases", "[tools]")
{
    const auto next = [](const char *version) {
        return FormatVersion(NextVersion(*ParseVersion(version)));
    };
    CHECK(next("0.3.0") == "0.3.0.1");
    CHECK(next("0.4.0.8") == "0.4.0.9");
    CHECK(next("0.4.0.9") == "0.4.1.0");
    CHECK(next("0.4.9.9") == "0.5.0.0");
    CHECK(next("0.19.9.9") == "1.0.0.0");
}

TEST_CASE("A version site is read and written in place", "[tools]")
{
    const VersionSite site{ "file", "version: '", "'" };
    const std::string text = "name\n  version: '1.2.3',\n  version: '9'\n";
    CHECK(ReadVersionSite(text, site) == "1.2.3");
    CHECK(WriteVersionSite(text, site, "1.2.3.4")
          == "name\n  version: '1.2.3.4',\n  version: '9'\n");
    CHECK_FALSE(ReadVersionSite("name", site));
    CHECK_FALSE(WriteVersionSite("name", site, "1"));
}

TEST_CASE("Release notes are found by their heading", "[tools]")
{
    const std::string notes = "# Release notes\n\n## 0.2\n\nNew.\n\n"
                              "### Part\n\n* One.\n\n\n## 0.1\n\nOld.\n";
    CHECK(ReleaseNotesOf(notes, "0.2") == "New.\n\n### Part\n\n* One.\n");
    CHECK(ReleaseNotesOf(notes, "0.1") == "Old.\n");
    CHECK_FALSE(ReleaseNotesOf(notes, "0.3"));
}

TEST_CASE("Includes are read as they are written", "[tools]")
{
    const auto angled = ParseInclude("  #include <a/b.hpp> // why\n");
    REQUIRE(angled);
    CHECK(angled->Name == "a/b.hpp");
    CHECK(angled->Angled);
    const auto quoted = ParseInclude("#include \"b.hpp\"\n");
    REQUIRE(quoted);
    CHECK_FALSE(quoted->Angled);
    CHECK_FALSE(ParseInclude("#  include <a.hpp>\n"));
    CHECK_FALSE(ParseInclude("// #include <a.hpp>\n"));
    CHECK_FALSE(ParseInclude("#include MACRO\n"));
}

TEST_CASE("The engine version comes from its declaration", "[tools]")
{
    CHECK(ParseEngineVersion("  static Version version( 3, 16, 1, \"\", 0 );")
          == "3.16.1");
    CHECK(ParseEngineVersion("static Version version( 3, 17, 0, \"rc\", 2 );")
          == "3.17.0-rc.2");
    CHECK_FALSE(ParseEngineVersion("nothing here"));
}

TEST_CASE("A report becomes JSON", "[tools]")
{
    RepositoryReport report;
    report.Root = "C:\\repos\\catch3";
    report.Os = "windows";
    report.Branch = "devel";
    report.Dirty = true;
    report.Tools.push_back({ "git", "/usr/bin/git", true });
    report.Tools.push_back({ "ninja", "", false });
    CHECK(ReportToJson(report) == R"({
  "root": "C:\\repos\\catch3",
  "os": "windows",
  "branch": "devel",
  "dirty": true,
  "tools": [
    {
      "name": "git",
      "path": "/usr/bin/git",
      "available": true
    },
    {
      "name": "ninja",
      "available": false
    }
  ]
}
)");
}

TEST_CASE("Text helpers keep line ends", "[tools]")
{
    CHECK(SplitLines("a\nb") == std::vector<std::string>{ "a\n", "b" });
    CHECK(SplitLines("").empty());
    CHECK(Trimmed("  a b\r\n") == "a b");
    CHECK(Trimmed(" \n").empty());
}
