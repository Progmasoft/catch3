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
bazelisk run //tools:check
```

It checks licenses, convenience headers, header guards, file names, that every
place states the same version, and that the single-file distribution is up to
date. A change to anything under `src/` regenerates that distribution:

```
bazelisk run //tools:amalgamate
```

The tools in `tools/` are C++17 and run through Bazel, with arguments after
`--`:

| Tool | Does |
| --- | --- |
| `//tools:amalgamate` | writes the single-file distribution; `-- --check` only compares it |
| `//tools:check` | runs the repository checks |
| `//tools:third_party` | `-- --check` compares `third_party/` with the archives `MODULE.bazel` pins, `-- --update` writes it |
| `//tools:version` | prints the version; `-- --next` or `-- <version>` moves every place that states it, and the distribution |
| `//tools:release_notes` | prints a section of the release notes, of `-- <version>` or the current one |
| `//tools:repo_info` | reports the checkout and the tools on `PATH`; `-- --json` writes it as JSON |


## Two parts, two sets of rules

`src/catch2` is the compatibility engine taken from Catch2. Its public API is
what Catch3 promises to keep: code that compiles against Catch2's headers
must keep compiling. It stays under the Boost Software License 1.0, keeps the
`Catch2 Authors` header and follows the style of the surrounding Catch2 code.
Code that includes its headers needs C++17 or later.

`src/Progmasoft` holds what Progmasoft adds: the runner, the results model,
property testing and snapshots. It is licensed `MPL-2.0 WITH
AdditionRef-Progmasoft-Exception-2.0`, follows the
[Progmasoft C++ Style](https://github.com/Progmasoft/progmasoft-cpp), is
C++17, reports recoverable errors with `Expected` rather than exceptions, and
is formatted with the `.clang-format` beside it.

`//tools:check` checks that every source file carries the header of its part.


## Tests

`tests/SelfTest` tests the compatibility engine; `tests/ExtraTests` tests
compile-time configurations and the runner's behaviour around them;
`tests/Progmasoft` tests the Progmasoft code, and the runner's integration
tests there start a fixture binary whose tests misbehave on purpose. A bug fix
comes with a test that fails without it.


## Documentation

User documentation lives in `docs/`. A new page is linked from
[the documentation index](Readme.md#top), and a page with sections gets a table
of contents.
A user-visible change gets a line in [the release notes](release-notes.md#top).


## Code of Conduct

Everyone who takes part follows the [Code of Conduct](../CODE_OF_CONDUCT.md).

---

[Home](Readme.md#top)
