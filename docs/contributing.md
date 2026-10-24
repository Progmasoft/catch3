<a id="top"></a>
# Contributing to Catch3

**Contents**<br>
[Where to start](#where-to-start)<br>
[Building and testing](#building-and-testing)<br>
[Two parts, two sets of rules](#two-parts-two-sets-of-rules)<br>
[Tests](#tests)<br>
[Documentation](#documentation)<br>
[Code of Conduct](#code-of-conduct)<br>

Progmasoft maintains Catch3. Issues and pull requests are welcome on
[Progmasoft/catch3](https://github.com/Progmasoft/catch3); pull requests go
to the `devel` branch. Report security problems privately, as described in
[SECURITY.md](../SECURITY.md), not in a public issue.


## Where to start

Open an issue before a large change, so that the direction can be agreed
first. A pull request should do one thing, say why in its description and
come with the tests that show it works.


## Building and testing

Bazel is the main build. With [Bazelisk](https://github.com/bazelbuild/bazelisk):

```
bazelisk test //tests/...
```

On Windows, point Bazel at Git's bash first:
`BAZEL_SH="C:\Program Files\Git\usr\bin\bash.exe"`.

CMake builds every test, including the extra tests that check compile-time
configurations:

```
cmake --preset basic-tests -GNinja -DCATCH3_BUILD_EXTRA_TESTS=ON -DCATCH3_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build -j
```

Meson, Xmake and Premake builds exist as well and are checked in CI.

Before you push, run the repository checks:

```
python tools/scripts/checkLicense.py
python tools/scripts/checkConvenienceHeaders.py
python tools/scripts/checkDuplicateFilenames.py
```

A change to anything under `src/` also regenerates the single-file
distribution with `python tools/scripts/generateAmalgamatedFiles.py`.


## Two parts, two sets of rules

`src/catch2` is the compatibility engine taken from Catch2. Its public API is
what Catch3 promises to keep: code that compiles against Catch2's headers
must keep compiling. It stays under the Boost Software License 1.0, keeps the
`Catch2 Authors` header and follows the style of the surrounding Catch2 code.
Code that includes only its headers needs C++14.

`src/Progmasoft` holds what Progmasoft adds: the runner, the results model,
property testing and snapshots. It is licensed `MPL-2.0 WITH
AdditionRef-Progmasoft-Exception-1.1`, follows the
[Progmasoft C++ Style](https://github.com/Progmasoft/progmasoft-cpp), is
C++20, reports recoverable errors with `Expected` rather than exceptions, and
is formatted with the `.clang-format` beside it.

`tools/scripts/checkLicense.py` checks that every source file carries the
header of its part.


## Tests

`tests/SelfTest` tests the compatibility engine; `tests/ExtraTests` tests
compile-time configurations and the runner's behaviour around them;
`tests/Progmasoft` tests the Progmasoft code, and the runner's integration
tests there start a fixture binary whose tests misbehave on purpose. A bug fix
comes with a test that fails without it.


## Documentation

User documentation lives in `docs/`. A new page is linked from
[the documentation index](Readme.md#top), and a page with sections gets a table
of contents; `python tools/scripts/updateDocumentToC.py` keeps them current.
A user-visible change gets a line in [the release notes](release-notes.md#top).


## Code of Conduct

Everyone who takes part follows the [Code of Conduct](../CODE_OF_CONDUCT.md).

---

[Home](Readme.md#top)
