<a id="top"></a>

![Progmasoft Catch3](data/artwork/catch3-logo-full-with-background.svg)

# Progmasoft Catch3

An independently maintained C++ test framework: Catch3 keeps the practical,
expressive test-writing model that developers expect, while adding a separately
owned C++17 library for property checks, snapshots, structured results, and XML
serialization.

The site of Catch3 is [catch3.uk](https://catch3.uk).

> Catch3 is not an official Catch2 release and is not endorsed by Catch2's
> maintainers. Its current compatibility engine is based on Catch2 3.16.1;
> upstream-originated files keep their upstream license, the Boost Software
> License 1.0. Progmasoft-authored additions are under LGPL-3.0-or-later. See
> [the project scope and compatibility notes](docs/why-catch3.md).

## What Catch3 provides

- **Familiar test authoring.** Catch2's test cases, assertions, sections,
  matchers, generators, listeners and benchmarks keep working unchanged.
- **A runner of its own.** The [Catch3 runner](docs/progmasoft-runner.md)
  runs tests in parallel worker processes, survives crashes and hangs, retries
  failures to tell flaky and order-dependent tests apart, quarantines flaky
  tests, keeps a history that drives time limits, ordering, incremental runs
  and replay, selects and shards tests, and writes console, JUnit, JSON and
  trace reports.
- **Mocks.** [Mocks](docs/progmasoft-mocking.md) of virtual interfaces and of
  free functions behind a seam check call counts, arguments and order, and
  report through the same assertions as the rest of a test.
- **Deterministic property testing.** Generate values from a reproducible seed,
  check a predicate over multiple trials, and shrink a failing input to a
  smaller counterexample.
- **Text snapshots.** Compare generated output against explicitly located
  snapshot files, with opt-in creation or replacement modes.
- **Structured results and XML output.** The runner's result model, with
  every attempt, and its deterministic JUnit XML writer are public.
- **A compiled library, not a header-only package.** Snapshot I/O and result/XML
  serialization are compiled C++17 implementations; templates and assertion
  macros remain in headers where needed.

Catch3 is in an early development stage. Catch2's command line and reporters
were replaced by the Catch3 runner; see
[the command line](docs/command-line.md) before moving a project over. The
property API currently checks one generated value per property and uses bounded,
deterministic greedy shrinking. See [current capabilities and limitations](docs/why-catch3.md)
before adopting it in a production project.

## Property-test example

```cpp
#include <Progmasoft/Catch3.hpp>

TEST_CASE("generated integers stay within their requested domain", "[property]") {
    CATCH3_CHECK_PROPERTY(
        Progmasoft::Catch3::Integer<int>(1, 100),
        [](int value) { return value > 0; },
        Progmasoft::Catch3::PropertyOptions{
            .Trials = 250,
            .Seed = 0xC0FFEE,
            .MaxShrinkSteps = 200});
}
```

The root seed and trial-index derivation are stable, so a reported failing trial
can be replayed. Generators can provide their own generation and shrinking
behavior. See the [property-testing guide](docs/progmasoft-catch3.md#property-testing)
for the API, seed guarantees, and shrinking contract.

## Build and test

Bazel is the primary build and validation path. From the repository root:

```sh
bazel test //tests/Progmasoft:property_tests
```

The same suite is also available through the maintained CMake, Meson, Xmake,
and Premake project files. Their setup details and toolchain requirements are
in the [build guide](docs/progmasoft-catch3.md#build-quick-start).

For a CMake consumer of the installed library:

```cmake
find_package(Catch3 0.3 CONFIG REQUIRED)

target_link_libraries(MyTests PRIVATE Catch3::Catch3WithMain)
```

`Catch3::Catch3` is the library: the engine, the runner, property testing and
snapshots; `Catch3::Catch3WithMain` adds the default `main`. Tests include
`<Progmasoft/Catch3/TestMacros.hpp>` and the other `Progmasoft/Catch3/`
headers. The library is C++17, and its headers work in C++17
and later. Catch3 and Catch2 can be installed side by side, as
their package, target, header and pkg-config names differ.

## Documentation

- [Catch3 additions: properties, snapshots, results, and XML](docs/progmasoft-catch3.md)
- [Project scope, compatibility policy, and limitations](docs/why-catch3.md)
- [The Catch3 runner](docs/progmasoft-runner.md)
- [Mocking](docs/progmasoft-mocking.md)
- [Tutorial](docs/tutorial.md)
- [Catch2-compatible API reference](docs/Readme.md)
- [Catch2 upstream project and source history](https://github.com/catchorg/Catch2)

Issues and feature requests for Progmasoft Catch3 belong in the
[Progmasoft/Catch3 issue tracker](https://github.com/Progmasoft/catch3/issues).

## License

Catch2-originated files keep the Boost Software License 1.0
([LICENSE.txt](LICENSE.txt)). Progmasoft-authored files are under the GNU Lesser
General Public License, version 3 or later
([LICENSES/LGPL-3.0-or-later.txt](LICENSES/LGPL-3.0-or-later.txt), with the
GPL-3.0 text it builds on beside it). Vendored libraries in `third_party/` keep
their own licenses. [LICENSES/README.md](LICENSES/README.md) maps which file is
under which license; releases before 0.5.0 keep the license they were published
under. Progmasoft-authored code is also covered by the [Progmasoft Patent Grant
1.1](PATENTS.md); Catch2-originated files are not.

Catch3 as a whole is therefore not under the Boost Software License: whoever
uses the library uses its LGPL files, and the BSL-1.0 of the Catch2-originated
files does not lift the LGPL's conditions. They take effect when Catch3, or a
work that contains it such as a test binary, is given to others; running your
own tests conveys nothing. What is owed then is not the same in every case: it
depends on whether Catch3 is linked as a shared library or statically, how
much of its headers a work compiles in, whether Catch3 itself is given, and
whether the work is part of a device.
[Using Catch3 as a whole](LICENSES/README.md#using-catch3-as-a-whole)
goes through each case.
