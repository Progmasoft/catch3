<a id="top"></a>

> **Catch3 compatibility reference:** This page describes the test-writing API Catch3 shares with Catch2: the macros, such as `TEST_CASE` and `REQUIRE`, and the `Catch::` namespace keep Catch2's names. Headers and build names are Catch3's own (`<Progmasoft/Catch3/TestMacros.hpp>`, `find_package(Catch3)`, `Catch3::Catch3WithMain`). See [Catch3 additions](progmasoft-catch3.md) for Progmasoft APIs.

# Tooling integration (CI, test runners and so on)

**Contents**<br>
[Continuous Integration systems](#continuous-integration-systems)<br>
[Bazel test runner integration](#bazel-test-runner-integration)<br>
[Low-level tools](#low-level-tools)<br>
[CMake](#cmake)<br>

This page talks about Catch2's integration with other related tooling,
like Continuous Integration and 3rd party test runners.


## Continuous Integration systems

The [Catch3 runner](progmasoft-runner.md#top) writes the reports build
servers read: `--junit=FILE` for JUnit XML, which most CI systems display,
with flaky tests and retries in the `<flakyFailure>` and `<rerunFailure>`
elements Jenkins and GitLab understand, and `--json=FILE` for everything
else. Under `bazel test` the JUnit report goes to the file Bazel names. See
[reports](reporters.md#top).

## Bazel test runner integration

Catch2 understands some of the environment variables Bazel uses to control
test execution. Specifically it understands

 * JUnit output path via `XML_OUTPUT_FILE`
 * Test filtering via `TESTBRIDGE_TEST_ONLY`
 * Test sharding via `TEST_SHARD_INDEX`, `TEST_TOTAL_SHARDS`, and `TEST_SHARD_STATUS_FILE`
 * Creating a file to signal premature test exit via `TEST_PREMATURE_EXIT_FILE`
 * Setting the RNG seed via `TEST_RANDOM_SEED`

> Support for `XML_OUTPUT_FILE` was [introduced](https://github.com/catchorg/Catch2/pull/2399) in Catch2 3.0.1

> Support for `TESTBRIDGE_TEST_ONLY` and sharding was introduced in Catch2 3.2.0

> Support for `TEST_PREMATURE_EXIT_FILE` and `TEST_RANDOM_SEED` was introduced in Catch2 3.11.0

This integration is enabled via either a [compile time configuration
option](configuration.md#bazel-support), or via `BAZEL_TEST` environment
variable set to "1".

> Support for `BAZEL_TEST` was [introduced](https://github.com/catchorg/Catch2/pull/2459) in Catch2 3.1.0

Note that if both the Bazel environment var and command line option for
something are used, the environment variable wins.


## Low-level tools

### CodeCoverage module (GCOV, LCOV...)

If you are using GCOV tool to get testing coverage of your code, and are not sure how to integrate it with CMake and Catch, there should be an external example over at https://github.com/claremacrae/catch_cmake_coverage


### pkg-config

Catch2 provides a rudimentary pkg-config integration, by registering itself
under the name `catch2`. This means that after Catch2 is installed, you
can use `pkg-config` to get its include path: `pkg-config --cflags catch2`.

### gdb and lldb scripts

Catch2's `extras` folder also contains two simple debugger scripts,
`gdbinit` for `gdb` and `lldbinit` for `lldb`. If loaded into their
respective debugger, these will tell it to step over Catch2's internals
when stepping through code.


## CMake

[As it has been getting kinda long, the documentation of Catch2's
integration with CMake has been moved to its own page.](cmake-integration.md#top)


---

[Home](Readme.md#top)
