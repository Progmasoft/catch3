<a id="top"></a>

![Progmasoft Catch3](data/artwork/catch3-logo-full-with-background.svg)

# Progmasoft Catch3

An independently maintained C++ test framework: Catch3 keeps the practical,
expressive test-writing model that developers expect, while adding a separately
owned C++20 library for property checks, snapshots, structured results, and XML
serialization.

> Catch3 is not an official Catch2 release and is not endorsed by Catch2's
> maintainers. Its current compatibility engine is based on Catch2 3.16.1;
> upstream-originated files keep their upstream license. Progmasoft-authored
> additions have their own licensing terms. See [the project scope and
> compatibility notes](docs/why-catch3.md).

## What Catch3 provides

- **Familiar test authoring.** Catch2's test cases, assertions, sections,
  matchers, generators, listeners and benchmarks keep working unchanged.
- **A runner of its own.** The [Catch3 runner](docs/progmasoft-runner.md)
  runs tests in parallel worker processes, survives crashes and hangs, retries
  failures to tell flaky and order-dependent tests apart, quarantines flaky
  tests, keeps a history that drives time limits, ordering, incremental runs
  and replay, selects and shards tests, and writes console, JUnit, JSON and
  trace reports.
- **Deterministic property testing.** Generate values from a reproducible seed,
  check a predicate over multiple trials, and shrink a failing input to a
  smaller counterexample.
- **Text snapshots.** Compare generated output against explicitly located
  snapshot files, with opt-in creation or replacement modes.
- **Structured results and XML output.** The runner's result model, with
  every attempt, and its deterministic JUnit XML writer are public.
- **A compiled library, not a header-only package.** Snapshot I/O and result/XML
  serialization are compiled C++20 implementations; templates and assertion
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
headers. The library is built as C++20; code that includes only the engine's
headers needs C++14. Catch3 and Catch2 can be installed side by side, as
their package, target, header and pkg-config names differ.

## Documentation

- [Catch3 additions: properties, snapshots, results, and XML](docs/progmasoft-catch3.md)
- [Project scope, compatibility policy, and limitations](docs/why-catch3.md)
- [The Catch3 runner](docs/progmasoft-runner.md)
- [Tutorial](docs/tutorial.md)
- [Catch2-compatible API reference](docs/Readme.md)
- [Catch2 upstream project and source history](https://github.com/catchorg/Catch2)

Issues and feature requests for Progmasoft Catch3 belong in the
[Progmasoft/Catch3 issue tracker](https://github.com/Progmasoft/catch3/issues).
