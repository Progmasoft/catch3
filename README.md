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
> [the project scope and compatibility notes](https://catch3.uk/docs/why-catch3/).

## What Catch3 provides

- **Familiar test authoring.** Catch2's test cases, assertions, sections,
  matchers, generators, listeners and benchmarks keep working unchanged.
- **A runner of its own.** The [Catch3 runner](https://catch3.uk/docs/progmasoft-runner/)
  runs tests in parallel worker processes, survives crashes and hangs, retries
  failures to tell flaky and order-dependent tests apart, quarantines flaky
  tests, keeps a history that drives time limits, ordering, incremental runs
  and replay, selects and shards tests, and writes console, JUnit, JSON and
  trace reports.
- **Mocks.** [Mocks](https://catch3.uk/docs/progmasoft-mocking/) of virtual
  interfaces and of free functions behind a seam check call counts, arguments
  and order, and
  report through the same assertions as the rest of a test.
- **Death tests.** [Death tests](https://catch3.uk/docs/progmasoft-death-tests/)
  check that a statement ends the process, by exit code, signal or what it
  wrote to standard error; the statement runs in a child process, on Linux,
  macOS and Windows alike.
- **Property testing.** [`ForAll`](https://catch3.uk/docs/progmasoft-properties/)
  draws from any number of composable strategies (`Map`, `Filter`, `FlatMap`,
  containers) and shrinks every failure through the recorded choices, so a
  shrunk input still satisfies the filters that built it. It also runs
  model-based tests of state machines, steers a targeted search with scores,
  keeps an example database that replays past failures first, fails a
  property whose inputs miss a case it must cover, and offers ready-made
  oracles: equivalence, metamorphic relations, round trips and idempotence.
  Every run is reproducible from its seed or replay token.
- **Text snapshots.** Compare generated output against explicitly located
  snapshot files, with opt-in creation or replacement modes.
- **Structured results and XML output.** The runner's result model, with
  every attempt, and its deterministic JUnit XML writer are public.
- **A compiled library, not a header-only package.** Snapshot I/O and result/XML
  serialization are compiled C++17 implementations; templates and assertion
  macros remain in headers where needed.

Catch3 is in an early development stage. Catch2's command line and reporters
were replaced by the Catch3 runner; see
[the command line](https://catch3.uk/docs/command-line/) before moving a
project over, and [current capabilities and limitations](https://catch3.uk/docs/why-catch3/)
before adopting it in a production project.

## Property-test example

```cpp
#include <Progmasoft/Catch3.hpp>

using namespace Progmasoft::Catch3;

TEST_CASE("division undoes multiplication", "[property]") {
    CATCH3_CHECK_FOR_ALL(
        Gen::Integers<int>(-1000, 1000), Gen::Integers<int>(1, 50),
        [](int value, int divisor) {
            Note("product", value * divisor);
            CATCH3_PROPERTY_ASSERT(value * divisor / divisor == value);
        });
}
```

`ForAll` draws one value from each strategy, runs the body over many trials
and, when it fails, reports the smallest input it found with a replay token
that runs exactly that input again. The same engine checks state machines
against a model:

```cpp
const auto result = CheckStateful<Model, Queue>(
    options, [] { return Model{}; },
    [](const Model&) { return Queue{}; },
    {push, pop});
```

A failing command sequence shrinks to fewer steps with simpler arguments. See
the [property-testing guide](https://catch3.uk/docs/progmasoft-properties/)
for strategies, shrinking, targeted search, the example database and
coverage requirements. The older single-generator `CATCH3_CHECK_PROPERTY` API
remains supported; it is described in
[Progmasoft Catch3 additions](https://catch3.uk/docs/progmasoft-catch3/#property-testing).

## Build and test

Bazel is the primary build and validation path. From the repository root:

```sh
bazel test //tests/Progmasoft:property_tests
```

The same suite is also available through the maintained CMake, Meson, Xmake,
and Premake project files. Their setup details and toolchain requirements are
in the [build guide](https://catch3.uk/docs/progmasoft-catch3/#build-quick-start).

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

- [Catch3 additions: properties, snapshots, results, and XML](https://catch3.uk/docs/progmasoft-catch3/)
- [Project scope, compatibility policy, and limitations](https://catch3.uk/docs/why-catch3/)
- [The Catch3 runner](https://catch3.uk/docs/progmasoft-runner/)
- [Mocking](https://catch3.uk/docs/progmasoft-mocking/)
- [Tutorial](https://catch3.uk/docs/tutorial/)
- [Catch2-compatible API reference](https://catch3.uk/docs/)
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
