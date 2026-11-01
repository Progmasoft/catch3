<a id="top"></a>

# Release notes

These are the releases of Progmasoft Catch3. Its compatibility engine is
based on Catch2 3.16.1; Catch2's own history is in
[its release notes](https://github.com/catchorg/Catch2/blob/devel/docs/release-notes.md).

**Contents**<br>
[0.3.0.1](#0301)<br>
[0.3.0](#030)<br>
[0.2.0](#020)<br>
[0.1.0](#010)<br>


## 0.3.0.1

Released 2026-11-01.

Versions of Catch3 have four numbers from this release on.

### The Catch3 runner

* `--import=FILE` reads the reports of other test frameworks instead of running
  tests: JUnit, xUnit, TRX, TAP, CTRF, Catch3's own, and a small JSON Lines
  format for anything else. Their tests get the history, flaky detection,
  quarantine, analytics and reports of a run.


## 0.3.0

Released 2026-11-01.

Tags of Catch3 are `catch3-<version>` from this release on.

### Language and license

Catch3 is C++17, and its headers work in C++17 and later. Progmasoft-authored
files move to the Progmasoft Linking Exception, Version 2.0; Catch2-derived code
keeps the Boost Software License.

### The Catch3 runner

Catch3 keeps Catch2's API for writing tests and replaces its session, command
line and reporters with [a runner of its own](progmasoft-runner.md#top):

* Tests run in parallel worker processes; a crash, an exit or a hang ends one
  test, not the run.
* Failures are retried alone to tell deterministic, flaky and order-dependent
  failures apart; the test that leaves state behind is found by bisection.
* A history of verdicts and durations drives quarantine, learned time limits,
  predictive ordering, incremental runs, replay and analytics.
* Tests are selected by change, by past failure or by prediction, and sharded
  by a stable hash or by recorded duration.
* Tags declare serial and isolated tests, shared resources, dependencies,
  time limits and retries.
* Every failure gets a category, a signature, an analysis and a replay command.
* Reports are console, JUnit XML, JSON and a Chrome trace; `--list=json` is a
  versioned discovery format.
* Every test gets a temporary directory of its own.
* A project keeps its options in a
  [`catch3.json`](progmasoft-runner.md#project-configuration), found up to
  the root of the repository; the command line wins over it, and
  `--config=FILE` and `--no-config` choose another file or none.
* JSON is read and written with [yyjson](https://github.com/ibireme/yyjson)
  and JUnit XML is written with [pugixml](https://github.com/zeux/pugixml),
  both MIT and vendored in `third_party/`. They are compiled into Catch3's
  own translation units with internal linkage or a renamed namespace, so a
  project that links its own copy of either library is not affected. The
  amalgamated sources include them from a third file,
  `extras/catch_amalgamated_third_party.hpp`.

### Breaking changes

* Catch3 has its own build and header names, so that it is never mistaken
  for Catch2 and both can be used in one project:
  * CMake: `find_package(Catch3)` with `Catch3::Catch3` and
    `Catch3::Catch3WithMain`, options `CATCH3_*`, and the scripts
    `Catch3.cmake` (`catch3_discover_tests`) and `Catch3ShardTests.cmake`
    (`catch3_add_sharded_tests`). The `Progmasoft::Catch3` target is gone:
    its code is part of `Catch3::Catch3`.
  * Bazel `//:catch3` and `//:catch3_main`, Meson `catch3` and
    `catch3-with-main`, pkg-config `catch3.pc` and `catch3-with-main.pc`.
  * Headers moved from `catch2/` to `Progmasoft/Catch3/` with Progmasoft
    names: `<catch2/catch_test_macros.hpp>` is
    `<Progmasoft/Catch3/TestMacros.hpp>`, `<catch2/matchers/catch_matchers_string.hpp>`
    is `<Progmasoft/Catch3/Matchers/String.hpp>`, and so on. Test bodies do
    not change: the macros and the `Catch::` namespace keep their names.
  * `ParseAndAddCatchTests.cmake`, deprecated upstream, was removed.
* Catch2's command line, reporters, `CATCH_REGISTER_REPORTER` and listing
  were removed; see [the command line](command-line.md#top) and
  [reports](reporters.md#top). Listeners keep working.
* C++17 is the minimum, for the library and for the code that uses it.
* The results model in `Progmasoft/Catch3/Results.hpp` was rewritten around
  tests and attempts.
* Conan packaging was dropped.

### Fixes

* The property engine's example database can be written by several
  processes at once.


## 0.2.0

Released 2026-09-24.

* Snapshot comparison and update APIs.
* Framework-neutral test result summaries and deterministic JUnit XML.
* The Progmasoft additions are a static library in every supported build
  system.


## 0.1.0

Released 2026-09-24.

* The first release: a deterministic property-testing API next to the
  Catch2-compatible engine.
* Bazel module metadata, and CMake, Meson, Xmake and Premake builds.

---

[Home](Readme.md#top)
