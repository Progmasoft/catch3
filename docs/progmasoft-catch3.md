# Progmasoft Catch3 additions

Progmasoft Catch3 is a compatibility-preserving extension of Catch2, not a
replacement runner. The upstream `Catch2` API, command-line behavior, include
paths, and BSL-1.0 files remain the compatibility foundation. Progmasoft-owned
headers use the `Progmasoft/Catch3` include tree and `Progmasoft::Catch3`
namespace.

## Property testing

`<Progmasoft/Catch3.hpp>` is the umbrella header. It brings in Catch2's
`catch_all.hpp` as well as the Progmasoft extensions, so it is convenient but
increases compile time. Include individual headers when a translation unit only
needs a narrow part of the API.

```cpp
#include <Progmasoft/Catch3.hpp>

TEST_CASE("integer values stay in their requested domain", "[property]") {
    CATCH3_CHECK_PROPERTY(
        Progmasoft::Catch3::Integer<int>(1, 100),
        [](int value) { return value > 0; },
        Progmasoft::Catch3::PropertyOptions{
            .Trials = 250,
            .Seed = 0xC0FFEE,
            .MaxShrinkSteps = 200});
}
```

The current property API is split into these headers:

- `<Progmasoft/Catch3/Random.hpp>`: portable SplitMix64 sequence, unbiased
  bounded sampling, inclusive integer ranges, and per-trial seed derivation.
- `<Progmasoft/Catch3/Generator.hpp>`: user-defined generation/shrinking pair.
- `<Progmasoft/Catch3/Generators/Integer.hpp>` and `Boolean.hpp`: built-in
  scalar strategies with deterministic shrink candidates.
- `<Progmasoft/Catch3/Generators/Vector.hpp>`, `Tuple.hpp`, `Element.hpp` and
  `Text.hpp`: composite strategies described under
  [Composite generators](#composite-generators).
- `<Progmasoft/Catch3/Property.hpp>`: trial execution and structured results.
- `<Progmasoft/Catch3/Assertions.hpp>`: `CATCH3_CHECK_PROPERTY`, which reports
  the failing trial, replay seed, reason, and minimized counterexample through
  Catch2's normal assertion/reporting path.

`PropertyOptions::Seed` is a stable root seed. Each trial receives a seed derived
from the root and its zero-based trial index, so the reported replay seed can be
used to reproduce that trial independently. The default generator is also stable
across standard-library implementations; it does not rely on the implementation-
defined output sequence of `std::uniform_int_distribution`.

Shrinking is deterministic and greedy: candidates are tested in the order
provided by the generator, and the first still-failing candidate is selected.
`MaxShrinkSteps` bounds predicate evaluations during minimization. The result is
therefore a useful locally minimized counterexample, not a claim of a globally
minimal value. A false predicate or an exception from the predicate fails the
property. Exceptions from generation or shrinking are reported as execution
errors, distinct from an ordinary failing property.

A failing report ends with the line
`replay: set PropertyOptions::ReplayTrialSeed to <seed>`. Setting that option
runs exactly one trial with the given seed, including its shrinking, and
ignores `Seed` and `Trials`; the reported trial number is then 1. This
reproduces a failure from a CI log without rerunning the trials before it.

```cpp
Progmasoft::Catch3::PropertyOptions{ .ReplayTrialSeed = 1234567890123456789ULL }
```

### Composite generators

A property still receives one generated value. Several arguments are passed
as one tuple:

```cpp
using namespace Progmasoft::Catch3;

CATCH3_CHECK_PROPERTY(
    Tuple(Vector(Integer<int>(0, 100), 0, 20), Text("abc", 1, 8)),
    [](const std::tuple<std::vector<int>, std::string> &input) {
        return Accepts(std::get<0>(input), std::get<1>(input));
    },
    PropertyOptions{ .Trials = 500 });
```

- `Tuple(generators...)` draws its components strictly left to right from the
  trial's random source and shrinks one component at a time: every candidate
  of the first component, then every candidate of the second, and so on.
- `Vector(element, minimumSize, maximumSize)` draws a length in the inclusive
  range and then the elements in index order. `Text(alphabet, minimumLength,
  maximumLength)` does the same for byte strings over the given alphabet;
  `PrintableAscii()` is the default alphabet. Both shrink in a fixed order:
  the shortest allowed prefix, the first and second half, the value with one
  element removed, and finally the value with one element simplified. A
  shrunk value is never shorter than its minimum, each step offers at most
  256 candidates, and a size limit above 1,048,576 elements is rejected.
- `Element(values)` picks uniformly from a fixed non-empty list. The list
  order is the simplicity order: a value shrinks to the values listed before
  it. `Text` treats its alphabet the same way, so put the simplest value or
  character first.

Counterexamples of tuples, pairs and vectors are printed element by element,
for example `{ 7, { true, false }, "text" }`. This does not depend on Catch2's
optional tuple or range string makers, so the report is the same whatever
configuration macros a client defines. Scalars still use `Catch::StringMaker`,
including user specializations.

Invalid factory arguments, such as an empty alphabet or a minimum above its
maximum, throw `std::invalid_argument`; a client built without exceptions
gets the message on standard error followed by `std::abort`.

State-machine commands, parallel trials, mapped or filtered generators, and
adaptive shrinking are deliberately not implied by this module; they need
their own deterministic contracts and tests before being added. Shrinking a
composite value is greedy and local like scalar shrinking: it finds a small
counterexample, not a provably smallest one.

## Snapshots

`<Progmasoft/Catch3/Snapshot.hpp>` provides file-backed text snapshots through
`CompareSnapshot(name, actual, options)`. The directory is supplied explicitly;
names are restricted to portable ASCII filename characters and cannot contain
path separators. CRLF and CR are normalized to LF by default, with no trimming
or other content rewriting.

Ordinary comparisons never write files. `SnapshotUpdateMode::kCreateMissing`
creates only absent snapshots, while `kAlways` explicitly permits replacement.
The result reports a match, create/update, mismatch with a one-based first
difference location, invalid input, or I/O failure. `CATCH3_CHECK_SNAPSHOT`
combines this result with Catch2's normal assertion reporting. Keep snapshots
under a directory owned by the test suite and enable replacement only for an
intentional update run.

```cpp
const Progmasoft::Catch3::SnapshotOptions options{
    .Directory = "tests/snapshots",
};
CATCH3_CHECK_SNAPSHOT("rendered-report", RenderReport(), options);
```

## Structured results and XML

`<Progmasoft/Catch3/Results.hpp>` defines a runner-neutral hierarchy of
`TestRunResult`, `TestSuiteResult`, and `TestCaseResult`. `SummarizeTestRun`
validates finite, non-negative durations and computes pass/failure/error/skip
counts. `<Progmasoft/Catch3/XmlWriter.hpp>` serializes that model as deterministic
JUnit-style XML through `JUnitXmlWriter::Write` or `ToString`. It escapes XML
markup, replaces malformed UTF-8 and XML-forbidden control characters, and
omits timestamps so identical results produce identical output. Catch2's own
`--reporter xml` and `--reporter junit` remain the direct reporters for results
produced by its runner; this writer is for Progmasoft's explicit result model.

The snapshot and result/XML APIs have compiled C++ implementations in the
`Progmasoft::Catch3` library target. Generator templates and convenience macros
remain in headers where their types or call-site expansion require it.

## Build and integration

The CMake build exposes `Progmasoft::Catch3`, a compiled C++20 library linked to
`Catch2::Catch2`. The existing Catch2 target retains its C++14 baseline. After
installation, consumers can use:

```cmake
find_package(Catch2 3.16 CONFIG REQUIRED)
find_package(ProgmasoftCatch3 3.16 CONFIG REQUIRED)
target_link_libraries(MyTests PRIVATE Progmasoft::Catch3 Catch2::Catch2WithMain)
```

The Bazel target is `//src/Progmasoft:catch3`; its tests are
`//tests/Progmasoft:property_tests`. The compatibility project continues to
build with Catch2's upstream build and test structure. Bazel is the primary
build path; CMake, Meson, Xmake, and Premake are also maintained as alternate
ways to build the same engine and property tests. Their root files are
`CMakeLists.txt`, `meson.build`, `xmake.lua`, and `premake5.lua`. Xmake and
Premake materialize Catch2's default generated configuration header from the
checked-in upstream template. Meson 1.1.0 or newer is required for the Clang-CL
C++20 test target.

For project history, compatibility intent, license split, and current feature
limits, read [Why Progmasoft Catch3?](why-catch3.md).

## Build quick start

Bazel is the primary build path:

```sh
bazel test //tests/Progmasoft:property_tests
```

The alternate project builds exercise the same property suite:

```sh
# CMake
cmake -S . -B build/cmake -DCATCH_DEVELOPMENT_BUILD=ON -DCATCH_BUILD_TESTING=ON
cmake --build build/cmake --target ProgmasoftSelfTest
ctest --test-dir build/cmake -R Progmasoft --output-on-failure

# Xmake
xmake build ProgmasoftPropertyTests
xmake run ProgmasoftPropertyTests

# Meson (Windows PowerShell; requires the Visual C++ SDK/CRT)
$env:CC = 'clang-cl'; $env:CXX = 'clang-cl'
meson setup build/meson --vsenv -Dtests=true -Dinstall=false
meson compile -C build/meson
meson test -C build/meson --print-errorlogs

# Premake (choose an action supported by the host)
premake5 vs2022
```

Premake generates projects rather than compiling directly; build the generated
solution with the corresponding host toolchain. On macOS, use
`premake5 gmake`, then build the generated makefiles with `make`. Meson users on
macOS can select Apple Clang with `CC=clang CXX=clang++` before configuration.

## Compatibility and licensing boundary

Do not rename or rewrite Catch2 headers, `Catch` symbols, macros, runner options,
or upstream coding style as part of this extension. New functionality belongs
in separately licensed Progmasoft headers and uses Progmasoft's PascalCase C++
types/functions and project naming conventions. Catch2-originated source files
remain under BSL-1.0; Progmasoft-authored source files carry the MPL-2.0 plus
Progmasoft linking-exception SPDX expression shown in their headers. The two
areas are not relicensed as a whole. Any proposed upstream PR must be prepared
under Catch2's contribution rules as a separate contribution, rather than
reusing a source file whose license header says otherwise.
