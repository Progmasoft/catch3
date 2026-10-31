<a id="top"></a>

> **Catch3 compatibility reference:** This page describes the test-writing API Catch3 shares with Catch2: the macros, such as `TEST_CASE` and `REQUIRE`, and the `Catch::` namespace keep Catch2's names. Headers and build names are Catch3's own (`<Progmasoft/Catch3/TestMacros.hpp>`, `find_package(Catch3)`, `Catch3::Catch3WithMain`). See [Catch3 additions](progmasoft-catch3.md) for Progmasoft APIs.

# CMake integration

**Contents**<br>
[CMake targets](#cmake-targets)<br>
[Automatic test registration](#automatic-test-registration)<br>
[CMake project options](#cmake-project-options)<br>
[`CATCH_CONFIG_*` customization options in CMake](#catch_config_-customization-options-in-cmake)<br>
[Using Catch3 from a checkout](#using-catch3-from-a-checkout)<br>
[Using an installed Catch3 package](#using-an-installed-catch3-package)<br>
[Catch3 next to Catch2](#catch3-next-to-catch2)<br>

Catch3's CMake package is `Catch3`. It exports two targets:

1. `Catch3::Catch3`, the library: the Catch2-compatible engine, the Catch3
   runner, property testing and snapshots. It is C++17, and its headers
   work in C++17 and later.
2. `Catch3::Catch3WithMain`, the library with the default `main`.

`Catch3.cmake` registers each `TEST_CASE` with CTest.

## CMake targets

Link `Catch3::Catch3WithMain` unless you provide a custom `main`. For an
installed Catch3:

```cmake
find_package(Catch3 0.3 CONFIG REQUIRED)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch3::Catch3WithMain)

add_executable(custom-main-tests test.cpp test-main.cpp)
target_link_libraries(custom-main-tests PRIVATE Catch3::Catch3)
```

If Catch3 is checked out in `external/catch3`, build it as a subdirectory:

```cmake
add_subdirectory(external/catch3)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch3::Catch3WithMain)
```


## Automatic test registration

Catch3 also contains two CMake scripts that register `TEST_CASE`s with
CTest. They are in the `extras` folder:

1) `Catch3.cmake` (and its dependency `Catch3AddTests.cmake`)
2) `Catch3ShardTests.cmake` (and its dependency `Catch3ShardTestsImpl.cmake`)

If Catch3 is installed, both can be used after `find_package(Catch3 REQUIRED)`.
Otherwise add the `extras` folder to your CMake module path. Their names, and
the names of the functions they define, start with `Catch3`/`catch3_`, so they
do not meet Catch2's `Catch.cmake` in a project that uses both.

<a id="catch3_discover_tests"></a>
### `Catch3.cmake` and `Catch3AddTests.cmake`

`Catch3.cmake` provides function `catch3_discover_tests` to get tests from
a target. This function works by running the resulting executable with
`--list=json`, and then reading the listing to find all existing tests.

#### Usage
```cmake
cmake_minimum_required(VERSION 3.16)

project(baz LANGUAGES CXX VERSION 0.0.1)

find_package(Catch3 REQUIRED)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch3::Catch3)

include(CTest)
include(Catch3)
catch3_discover_tests(tests)
```

When using `FetchContent`, `include(Catch3)` will fail unless
`CMAKE_MODULE_PATH` is explicitly updated to include the extras
directory.

```cmake
# ... FetchContent ...
#
list(APPEND CMAKE_MODULE_PATH ${catch3_SOURCE_DIR}/extras)
include(CTest)
include(Catch3)
catch3_discover_tests(tests)
```

#### Customization
`catch3_discover_tests` can be given several extra arguments:
```cmake
catch3_discover_tests(target
                     [TEST_SPEC arg1...]
                     [EXTRA_ARGS arg1...]
                     [WORKING_DIRECTORY dir]
                     [TEST_PREFIX prefix]
                     [TEST_SUFFIX suffix]
                     [PROPERTIES name1 value1...]
                     [TEST_LIST var]
                     [REPORTER reporter]
                     [OUTPUT_DIR dir]
                     [OUTPUT_PREFIX prefix]
                     [OUTPUT_SUFFIX suffix]
                     [DISCOVERY_MODE <POST_BUILD|PRE_TEST>]
                     [SKIP_IS_FAILURE]
                     [ADD_TAGS_AS_LABELS]
)
```

* `TEST_SPEC arg1...`

Specifies test cases, wildcarded test cases, tags and tag expressions to
pass to the Catch executable alongside the `--list-test-names-only` flag.


* `EXTRA_ARGS arg1...`

Any extra arguments to pass on the command line to each test case.


* `WORKING_DIRECTORY dir`

Specifies the directory in which to run the discovered test cases.  If this
option is not provided, the current binary directory is used.


* `TEST_PREFIX prefix`

Specifies a _prefix_ to be added to the name of each discovered test case.
This can be useful when the same test executable is being used in multiple
calls to `catch3_discover_tests()`, with different `TEST_SPEC` or `EXTRA_ARGS`.


* `TEST_SUFFIX suffix`

Same as `TEST_PREFIX`, except it specific the _suffix_ for the test names.
Both `TEST_PREFIX` and `TEST_SUFFIX` can be specified at the same time.


* `PROPERTIES name1 value1...`

Specifies additional properties to be set on all tests discovered by this
invocation of `catch3_discover_tests`.


* `TEST_LIST var`

Make the list of tests available in the variable `var`, rather than the
default `<target>_TESTS`.  This can be useful when the same test
executable is being used in multiple calls to `catch3_discover_tests()`.
Note that this variable is only available in CTest.

* `REPORTER reporter`

Write a report of each test: `junit` passes `--junit=<file>` and `json`
passes `--json=<file>` to the test executable; `console` writes no file.
`junit` and `json` need `OUTPUT_DIR`.

* `OUTPUT_DIR dir`

The directory the reports of `REPORTER` go to, one file per test, named
after the test. Use it instead of `EXTRA_ARGS --junit=foo` so that tests
that run at the same time do not write the same file.

* `OUTPUT_PREFIX prefix`

May be used in conjunction with `OUTPUT_DIR`.
If specified, `prefix` is added to each output file name, like so
`dir/prefix<test_name>`.

* `OUTPUT_SUFFIX suffix`

May be used in conjunction with `OUTPUT_DIR`.
If specified, `suffix` is added to each output file name, like so
`dir/<test_name>suffix`. This can be used to add a file extension to
the output file name e.g. ".xml".

* `DISCOVERY_MODE mode`

If specified allows control over when test discovery is performed.
For a value of `POST_BUILD` (default) test discovery is performed at build time.
For a value of `PRE_TEST` test discovery is delayed until just prior to test
execution (useful e.g. in cross-compilation environments).
``DISCOVERY_MODE`` defaults to the value of the
``CMAKE_CATCH3_DISCOVER_TESTS_DISCOVERY_MODE`` variable if it is not passed when
calling ``catch3_discover_tests``. This provides a mechanism for globally
selecting a preferred test discovery behavior.

_Note that on Apple Silicon with the Xcode generator you must use `PRE_TEST`,
e.g. `catch3_discover_tests(tests DISCOVERY_MODE PRE_TEST)`. With the default
`POST_BUILD` mode the build fails with `Result: Subprocess killed`, because
macOS on Apple Silicon refuses to run unsigned binaries and Xcode code-signs
the test executable only **after** the post-build script that `POST_BUILD`
mode uses to run it for test discovery. `PRE_TEST` avoids this by delaying
discovery until test time, when the executable is already signed. The same
limitation affects CMake's `gtest_discover_tests`; see
[Catch2 #2411](https://github.com/catchorg/Catch2/issues/2411) and
[CMake #21845](https://gitlab.kitware.com/cmake/cmake/-/issues/21845)._

* `SKIP_IS_FAILURE`

Skipped tests will be marked as failed instead.

* `ADD_TAGS_AS_LABELS`

Add the tags from tests as labels to CTest.


### `Catch3ShardTests.cmake`

`Catch3ShardTests.cmake` provides a function
`catch3_add_sharded_tests(TEST_BINARY)` that splits tests from `TEST_BINARY`
into multiple shards. The tests in each shard and their order is randomized,
and the seed changes every invocation of CTest.

Currently there are 3 customization points for this script:

 * SHARD_COUNT - number of shards to split target's tests into
 * REPORTER    - report options for the shards, such as --junit=FILE
 * TEST_SPEC   - test spec used for filtering tests

Example usage:

```
include(Catch3ShardTests)

catch3_add_sharded_tests(foo-tests
  SHARD_COUNT 4
  TEST_SPEC "A"
)

catch3_add_sharded_tests(tests
  SHARD_COUNT 8
  TEST_SPEC "B"
)
```

This registers total of 12 CTest tests (4 + 8 shards) to run shards
from `foo-tests` test binary, filtered by a test spec.

_Note that this script is currently a proof-of-concept for reseeding
shards per CTest run, and thus does not support (nor does it currently
aim to support) all customization points from
[`catch3_discover_tests`](#catch3_discover_tests)._


## CMake project options

Catch3's CMake project also provides some options for other projects
that consume it. These are:

* `BUILD_TESTING` -- When `ON` and the project is not used as a subproject,
Catch3's test binaries will be built. Defaults to `ON`.
* `CATCH3_INSTALL_DOCS` -- When `ON`, Catch3's documentation will be
included in the installation. Defaults to `ON`.
* `CATCH3_INSTALL_EXTRAS` -- When `ON`, Catch3's extras folder (the CMake
scripts mentioned above, debugger helpers) will be included in the
installation. Defaults to `ON`.
* `CATCH3_DEVELOPMENT_BUILD` -- When `ON`, configures the build for development
of Catch3. This means enabling test projects, warnings and so on.
Defaults to `OFF`.


Enabling `CATCH3_DEVELOPMENT_BUILD` also enables further configuration
customization options:

* `CATCH3_BUILD_TESTING` -- When `ON`, Catch3's SelfTest project will be
built. Defaults to `ON`. Note that Catch3 also obeys `BUILD_TESTING` CMake
variable, so _both_ of them need to be `ON` for the SelfTest to be built,
and either of them can be set to `OFF` to disable building SelfTest.
* `CATCH3_BUILD_EXAMPLES` -- When `ON`, Catch3's usage examples will be
built. Defaults to `OFF`.
* `CATCH3_BUILD_EXTRA_TESTS` -- When `ON`, Catch3's extra tests will be
built. Defaults to `OFF`.
* `CATCH3_BUILD_FUZZERS` -- When `ON`, Catch3's fuzzing entry points will
be built. Defaults to `OFF`.
* `CATCH3_ENABLE_WERROR` -- When `ON`, adds `-Werror` or equivalent flag
to the compilation. Defaults to `ON`.
* `CATCH3_BUILD_SURROGATES` -- When `ON`, each header of the engine will be
compiled separately to ensure that they are self-sufficient.
Defaults to `OFF`.


## `CATCH_CONFIG_*` customization options in CMake


Due to the new separate compilation model, all the options from the
[Compile-time configuration docs](configuration.md#top) can also be set
through Catch3's CMake. To set them, define the option you want as `ON`,
e.g. `-DCATCH_CONFIG_NOSTDOUT=ON`.

Note that setting the option to `OFF` doesn't disable it. To force disable
an option, you need to set the `_NO_` form of it to `ON`, e.g.
`-DCATCH_CONFIG_NO_COLOUR_WIN32=ON`.


To summarize the configuration option behaviour with an example:

| `-DCATCH_CONFIG_COLOUR_WIN32` | `-DCATCH_CONFIG_NO_COLOUR_WIN32` |      Result |
|-------------------------------|----------------------------------|-------------|
|                          `ON` |                             `ON` |       error |
|                          `ON` |                            `OFF` |    force-on |
|                         `OFF` |                             `ON` |   force-off |
|                         `OFF` |                            `OFF` | auto-detect |



## Using Catch3 from a checkout

Clone [Progmasoft/catch3](https://github.com/Progmasoft/catch3) and use
`add_subdirectory` as shown above. Keep it at a reviewed commit or release
tag in production builds.

## Using an installed Catch3 package

Configure and install Catch3 with CMake, then point `CMAKE_PREFIX_PATH` at
the install prefix and use `find_package(Catch3)`. The package installs
`lib/cmake/Catch3`, the headers below `include/Progmasoft`, and the
pkg-config files `catch3.pc` and `catch3-with-main.pc`.

## Catch3 next to Catch2

Catch3 is not Catch2's package: `find_package(Catch2)` finds Catch2, and
`find_package(Catch3)` finds Catch3. Their packages, targets, CMake scripts,
headers and pkg-config files have different names, so both can be installed
into the same prefix and used by different test binaries of one project. One
test binary links one of them: both define the `Catch::` namespace.

---

[Home](Readme.md#top)
