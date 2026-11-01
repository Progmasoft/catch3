# Why Progmasoft Catch3?

Progmasoft Catch3 is an independently maintained continuation built on the
Catch2 v3 codebase. It exists to preserve Catch2's practical testing model while
giving Progmasoft room to add capabilities and APIs under a clear ownership,
namespace, build-target, and licensing boundary.

The name does **not** mean that this is an official Catch2 v3 release, a new
upstream version, or a project endorsed by Catch2's maintainers. The current
compatibility engine is based on Catch2's 3.16.1 development line. The project
is still early-stage. It keeps Catch2's API for writing tests and replaced
Catch2's runner with [its own](progmasoft-runner.md#top).

## What is different today?

| Area | Catch2 compatibility engine | Progmasoft Catch3 additions |
| --- | --- | --- |
| Test execution | Catch2's test registration, `RunContext` and listeners | The Catch3 runner: parallel worker processes, retries, flaky-test detection, quarantine, history, selection, sharding and its own command line and reports; `CATCH3_CHECK_FOR_ALL` and `CATCH3_CHECK_PROPERTY` report property outcomes through Catch2's assertion path |
| Public include path | The engine's headers moved to `Progmasoft/Catch3/...` with Progmasoft names, such as `TestMacros.hpp` | `Progmasoft/Catch3.hpp` and the other `Progmasoft/Catch3/...` headers |
| C++ baseline | C++17; the headers work in C++17 and later | The same |
| Randomness | Catch2's existing behavior is unchanged | A specified SplitMix64 sequence provides deterministic trial seeds and unbiased bounded integer sampling |
| Generated values | No change to existing Catch2 generators | Strategies for numbers, text, collections and structures with `Map`, `Filter` and `FlatMap`; shrinking of recorded choices, so every combination shrinks; replay tokens and an example database; state machines, `Classify`/`Cover` statistics, targeted search, and differential and metamorphic oracles |
| Mocks | None | [`CATCH3_MOCK_METHOD`, `CATCH3_EXPECT_CALL`](progmasoft-mocking.md#top) and `Sequence`, reporting through the assertion path |
| Build identity | One library: the `Catch3` CMake package with `Catch3::Catch3` and `Catch3::Catch3WithMain`, `//:catch3` in Bazel, `catch3` in Meson and pkg-config | The same library |
| Source terms | Catch2-originated source retains its upstream BSL-1.0 terms | Progmasoft-authored additions state their own SPDX license in each file |

Properties take any number of generated values. Shrinking is deterministic and
bounded, but local rather than globally minimal. Parallel trials do not
exist yet; [the roadmap](progmasoft-roadmap.md#top)
orders them with per-test coverage, test selection, mutation testing
and test generation.

## Compatibility policy

Catch2's macros and its `Catch` namespace, with matchers, generators and
listeners, remain the compatibility surface. The runner, its command line,
its reports, the header paths and the build names are Catch3's own; moving
tests from Catch2 means changing their includes and their build files, not
their bodies. Progmasoft features are added in `Progmasoft::Catch3` and must
not require users to rewrite existing Catch2 test bodies. The first compatibility goal is source-level
continuity with the included Catch2 3.16.1 base; this is not a promise that every
future Catch2 upstream change will be adopted unchanged or that all downstream
projects have already been validated.

The engine, the runner and the Progmasoft additions are one compiled C++17
library. Template-based generators and call-site assertion macros remain in
headers; snapshot I/O, result aggregation, and XML serialization are compiled
implementations.
Keep tests for the two areas independently identifiable so additions do not
silently alter the upstream compatibility baseline.

## Why a fork instead of a patch series?

The initial additions need their own design and release cycle. Keeping them in a
separate namespace and file tree lets the project prototype those APIs without
presenting them as upstream Catch2 behavior. If a change is later proposed to
Catch2 upstream, prepare it as a separate contribution under Catch2's current
contribution and licensing requirements; do not assume Progmasoft-licensed
files can be submitted unchanged.

## Build-system support

Bazel is the primary build and validation path. CMake, Meson, Xmake, and Premake
are supported alternate project-generation/build paths over the same local sources.
The CMake project exports `Catch3::Catch3` and `Catch3::Catch3WithMain`; the
other build descriptions compile the same library and the Progmasoft
property-test executable. See the repository build
instructions and each build file for tool-specific commands.

Registry publication is a separate release step. It requires a versioned
immutable source archive and successful validation and maintainer review for
the `catch3` module name; a submission is not a guarantee of acceptance.
