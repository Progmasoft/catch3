<a id="top"></a>

> **Catch3 compatibility reference:** This page describes the bundled Catch2-compatible API. Names such as `Catch2`, `catch2/`, and `Catch2::Catch2WithMain` remain where they are actual compatibility interfaces. See [Catch3 additions](progmasoft-catch3.md) for Progmasoft APIs.

# CMake integration

**Contents**<br>
[CMake targets](#cmake-targets)<br>
[Automatic test registration](#automatic-test-registration)<br>
[CMake project options](#cmake-project-options)<br>
[`CATCH_CONFIG_*` customization options in CMake](#catch_config_-customization-options-in-cmake)<br>
[Using Progmasoft Catch3 from a checkout](#using-progmasoft-catch3-from-a-checkout)<br>
[Using an installed Catch3 package](#using-an-installed-catch3-package)<br>
[Upstream Catch2 packages](#upstream-catch2-packages)<br>

Catch3 builds its Progmasoft C++20 extension library alongside the
Catch2-compatible C++14 engine. CMake exposes both API families:

1. `Progmasoft::Catch3` contains the new compiled extensions and depends on
   `Catch2::Catch2`.
2. `Catch2::Catch2WithMain` supplies the existing test runner and `main`.
3. The bundled Catch2 scripts register `TEST_CASE`s with CTest.

Do not use the upstream `catchorg/Catch2` checkout or its vcpkg/Bazel package
when you need `Progmasoft::Catch3`; those packages do not contain this fork's
extensions.

## CMake targets

The fork exports `Progmasoft::Catch3`, `Catch2::Catch2`, and
`Catch2::Catch2WithMain`. Link the Progmasoft target when using extension
headers; link `Catch2::Catch2WithMain` unless you provide a custom `main`.
For a CMake-installed fork:

```cmake
find_package(Catch2 3.16 CONFIG REQUIRED)
find_package(ProgmasoftCatch3 3.16 CONFIG REQUIRED)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Progmasoft::Catch3 Catch2::Catch2WithMain)

add_executable(custom-main-tests test.cpp test-main.cpp)
target_link_libraries(custom-main-tests PRIVATE Progmasoft::Catch3)
```

If the fork is checked out in `external/catch3`, build it as a subdirectory:

```cmake
add_subdirectory(external/catch3)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Progmasoft::Catch3 Catch2::Catch2WithMain)
```


## Automatic test registration

Catch2's repository also contains three CMake scripts that help users
with automatically registering their `TEST_CASE`s with CTest. They
can be found in the `extras` folder, and are

1) `Catch.cmake` (and its dependency `CatchAddTests.cmake`)
2) `ParseAndAddCatchTests.cmake` (deprecated)
3) `CatchShardTests.cmake` (and its dependency `CatchShardTestsImpl.cmake`)

If Catch2 has been installed in system, both of these can be used after
doing `find_package(Catch2 REQUIRED)`. Otherwise you need to add them
to your CMake module path.

<a id="catch_discover_tests"></a>
### `Catch.cmake` and `CatchAddTests.cmake`

`Catch.cmake` provides function `catch_discover_tests` to get tests from
a target. This function works by running the resulting executable with
`--list-test` flag, and then parsing the output to find all existing tests.

#### Usage
```cmake
cmake_minimum_required(VERSION 3.16)

project(baz LANGUAGES CXX VERSION 0.0.1)

find_package(Catch2 REQUIRED)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(tests)
```

When using `FetchContent`, `include(Catch)` will fail unless
`CMAKE_MODULE_PATH` is explicitly updated to include the extras
directory.

```cmake
# ... FetchContent ...
#
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)
```

#### Customization
`catch_discover_tests` can be given several extra arguments:
```cmake
catch_discover_tests(target
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
calls to `catch_discover_tests()`, with different `TEST_SPEC` or `EXTRA_ARGS`.


* `TEST_SUFFIX suffix`

Same as `TEST_PREFIX`, except it specific the _suffix_ for the test names.
Both `TEST_PREFIX` and `TEST_SUFFIX` can be specified at the same time.


* `PROPERTIES name1 value1...`

Specifies additional properties to be set on all tests discovered by this
invocation of `catch_discover_tests`.


* `TEST_LIST var`

Make the list of tests available in the variable `var`, rather than the
default `<target>_TESTS`.  This can be useful when the same test
executable is being used in multiple calls to `catch_discover_tests()`.
Note that this variable is only available in CTest.

* `REPORTER reporter`

Use the specified reporter when running the test case. The reporter will
be passed to the test runner as `--reporter reporter`.

* `OUTPUT_DIR dir`

If specified, the parameter is passed along as
`--out dir/<test_name>` to test executable. The actual file name is the
same as the test name. This should be used instead of
`EXTRA_ARGS --out foo` to avoid race conditions writing the result output
when using parallel test execution.

* `OUTPUT_PREFIX prefix`

May be used in conjunction with `OUTPUT_DIR`.
If specified, `prefix` is added to each output file name, like so
`--out dir/prefix<test_name>`.

* `OUTPUT_SUFFIX suffix`

May be used in conjunction with `OUTPUT_DIR`.
If specified, `suffix` is added to each output file name, like so
`--out dir/<test_name>suffix`. This can be used to add a file extension to
the output file name e.g. ".xml".

* `DISCOVERY_MODE mode`

If specified allows control over when test discovery is performed.
For a value of `POST_BUILD` (default) test discovery is performed at build time.
For a value of `PRE_TEST` test discovery is delayed until just prior to test
execution (useful e.g. in cross-compilation environments).
``DISCOVERY_MODE`` defaults to the value of the
``CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE`` variable if it is not passed when
calling ``catch_discover_tests``. This provides a mechanism for globally
selecting a preferred test discovery behavior.

_Note that on Apple Silicon with the Xcode generator you must use `PRE_TEST`,
e.g. `catch_discover_tests(tests DISCOVERY_MODE PRE_TEST)`. With the default
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


### `ParseAndAddCatchTests.cmake`

⚠ This script is [deprecated](https://github.com/catchorg/Catch2/pull/2120)
in Catch2 2.13.4 and superseded by the above approach using `catch_discover_tests`.
See [#2092](https://github.com/catchorg/Catch2/issues/2092) for details.

`ParseAndAddCatchTests` works by parsing all implementation files
associated with the provided target, and registering them via CTest's
`add_test`. This approach has some limitations, such as the fact that
commented-out tests will be registered anyway. More serious, only a
subset of the assertion macros currently available in Catch can be
detected by this script and tests with any macros that cannot be
parsed are *silently ignored*.


#### Usage

```cmake
cmake_minimum_required(VERSION 3.16)

project(baz LANGUAGES CXX VERSION 0.0.1)

find_package(Catch2 REQUIRED)
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2)

include(CTest)
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(tests)
```


#### Customization

`ParseAndAddCatchTests` provides some customization points:
* `PARSE_CATCH_TESTS_VERBOSE` -- When `ON`, the script prints debug
messages. Defaults to `OFF`.
* `PARSE_CATCH_TESTS_NO_HIDDEN_TESTS` -- When `ON`, hidden tests (tests
tagged with either of `[.]` or `[.foo]`) will not be registered.
Defaults to `OFF`.
* `PARSE_CATCH_TESTS_ADD_FIXTURE_IN_TEST_NAME` -- When `ON`, adds fixture
class name to the test name in CTest. Defaults to `ON`.
* `PARSE_CATCH_TESTS_ADD_TARGET_IN_TEST_NAME` -- When `ON`, adds target
name to the test name in CTest. Defaults to `ON`.
* `PARSE_CATCH_TESTS_ADD_TO_CONFIGURE_DEPENDS` -- When `ON`, adds test
file to `CMAKE_CONFIGURE_DEPENDS`. This means that the CMake configuration
step will be re-ran when the test files change, letting new tests be
automatically discovered. Defaults to `OFF`.


Optionally, one can specify a launching command to run tests by setting the
variable `OptionalCatchTestLauncher` before calling `ParseAndAddCatchTests`. For
instance to run some tests using `MPI` and other sequentially, one can write
```cmake
set(OptionalCatchTestLauncher ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${NUMPROC})
ParseAndAddCatchTests(mpi_foo)
unset(OptionalCatchTestLauncher)
ParseAndAddCatchTests(bar)
```


### `CatchShardTests.cmake`

> `CatchShardTests.cmake` was introduced in Catch2 3.1.0.

`CatchShardTests.cmake` provides a function
`catch_add_sharded_tests(TEST_BINARY)` that splits tests from `TEST_BINARY`
into multiple shards. The tests in each shard and their order is randomized,
and the seed changes every invocation of CTest.

Currently there are 3 customization points for this script:

 * SHARD_COUNT - number of shards to split target's tests into
 * REPORTER    - reporter spec to use for tests
 * TEST_SPEC   - test spec used for filtering tests

Example usage:

```
include(CatchShardTests)

catch_add_sharded_tests(foo-tests
  SHARD_COUNT 4
  REPORTER "xml::out=-"
  TEST_SPEC "A"
)

catch_add_sharded_tests(tests
  SHARD_COUNT 8
  REPORTER "xml::out=-"
  TEST_SPEC "B"
)
```

This registers total of 12 CTest tests (4 + 8 shards) to run shards
from `foo-tests` test binary, filtered by a test spec.

_Note that this script is currently a proof-of-concept for reseeding
shards per CTest run, and thus does not support (nor does it currently
aim to support) all customization points from
[`catch_discover_tests`](#catch_discover_tests)._


## CMake project options

Catch2's CMake project also provides some options for other projects
that consume it. These are:

* `BUILD_TESTING` -- When `ON` and the project is not used as a subproject,
Catch2's test binary will be built. Defaults to `ON`.
* `CATCH_INSTALL_DOCS` -- When `ON`, Catch2's documentation will be
included in the installation. Defaults to `ON`.
* `CATCH_INSTALL_EXTRAS` -- When `ON`, Catch2's extras folder (the CMake
scripts mentioned above, debugger helpers) will be included in the
installation. Defaults to `ON`.
* `CATCH_DEVELOPMENT_BUILD` -- When `ON`, configures the build for development
of Catch2. This means enabling test projects, warnings and so on.
Defaults to `OFF`.


Enabling `CATCH_DEVELOPMENT_BUILD` also enables further configuration
customization options:

* `CATCH_BUILD_TESTING` -- When `ON`, Catch2's SelfTest project will be
built. Defaults to `ON`. Note that Catch2 also obeys `BUILD_TESTING` CMake
variable, so _both_ of them need to be `ON` for the SelfTest to be built,
and either of them can be set to `OFF` to disable building SelfTest.
* `CATCH_BUILD_EXAMPLES` -- When `ON`, Catch2's usage examples will be
built. Defaults to `OFF`.
* `CATCH_BUILD_EXTRA_TESTS` -- When `ON`, Catch2's extra tests will be
built. Defaults to `OFF`.
* `CATCH_BUILD_FUZZERS` -- When `ON`, Catch2 fuzzing entry points will
be built. Defaults to `OFF`.
* `CATCH_ENABLE_WERROR` -- When `ON`, adds `-Werror` or equivalent flag
to the compilation. Defaults to `ON`.
* `CATCH_BUILD_SURROGATES` -- When `ON`, each header in Catch2 will be
compiled separately to ensure that they are self-sufficient.
Defaults to `OFF`.


## `CATCH_CONFIG_*` customization options in CMake

> CMake support for `CATCH_CONFIG_*` options was introduced in Catch2 3.0.1

Due to the new separate compilation model, all the options from the
[Compile-time configuration docs](configuration.md#top) can also be set
through Catch2's CMake. To set them, define the option you want as `ON`,
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



## Using Progmasoft Catch3 from a checkout

Clone [Progmasoft/catch3](https://github.com/Progmasoft/catch3) and use
`add_subdirectory` as shown above. This builds the compatible runner and the
compiled Progmasoft extension library together. Keep the fork at a reviewed
commit or release tag in production builds.

## Using an installed Catch3 package

Configure and install the fork with CMake, then point `CMAKE_PREFIX_PATH` at
the install prefix when consuming it. Both `Catch2` and
`ProgmasoftCatch3` package configurations must come from the same Catch3
build. Do not mix a separately installed upstream Catch2 library with this
fork's extension library.

## Upstream Catch2 packages

The upstream Catch2 vcpkg port and Bazel Central Registry module install only
the compatibility engine. They do not provide `Progmasoft::Catch3`,
`Progmasoft/Catch3.hpp`, snapshots, properties, or result/XML extensions.
Use them only for applications that intentionally need the upstream API alone.

---

[Home](Readme.md#top)
