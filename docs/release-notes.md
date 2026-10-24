<a id="top"></a>

# Release notes

These are the releases of Progmasoft Catch3. Its compatibility engine is
based on Catch2 3.16.1; Catch2's own history is in
[its release notes](https://github.com/catchorg/Catch2/blob/devel/docs/release-notes.md).

**Contents**<br>
[0.3.0 (unreleased)](#030-unreleased)<br>
[0.2.0](#020)<br>
[0.1.0](#010)<br>


## 0.3.0 (unreleased)

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

### Breaking changes

* Catch2's command line, reporters, `CATCH_REGISTER_REPORTER` and listing
  were removed; see [the command line](command-line.md#top) and
  [reports](reporters.md#top). Listeners keep working.
* The Catch2 library is compiled as C++20; code that includes only its
  headers still needs C++14.
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
