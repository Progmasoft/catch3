# Why Progmasoft Catch3?

Progmasoft Catch3 is an independently maintained continuation built on the
Catch2 v3 codebase. It exists to preserve Catch2's practical testing model while
giving Progmasoft room to add capabilities and APIs under a clear ownership,
namespace, build-target, and licensing boundary.

The name does **not** mean that this is an official Catch2 v3 release, a new
upstream version, or a project endorsed by Catch2's maintainers. The current
compatibility engine is based on Catch2's 3.16.0 development line. The project
is still early-stage and does not yet replace or independently reimplement the
Catch2 runner.

## What is different today?

| Area | Catch2 compatibility engine | Progmasoft Catch3 additions |
| --- | --- | --- |
| Test execution | Catch2's existing runner, reporters, command-line interface, and test registration | `CATCH3_CHECK_PROPERTY` reports property outcomes through Catch2's assertion path |
| Public include path | Existing `catch2/...` headers | `Progmasoft/Catch3.hpp` and focused `Progmasoft/Catch3/...` headers |
| C++ baseline | Existing Catch2 target remains at C++14 | The added property API requires C++20 |
| Randomness | Catch2's existing behavior is unchanged | A specified SplitMix64 sequence provides deterministic trial seeds and unbiased bounded integer sampling |
| Generated values | No change to existing Catch2 generators | Initial integer/boolean strategies, custom generator callbacks, and bounded greedy shrinking |
| Build identity | Existing `Catch2` target and APIs remain available | Separate `Progmasoft::Catch3` CMake target and `//src/Progmasoft:catch3` Bazel target |
| Source terms | Catch2-originated source retains its upstream BSL-1.0 terms | Progmasoft-authored additions state their own SPDX license in each file |

The current property runner handles one generated value per property. Shrinking
is deterministic and bounded, but greedy rather than globally minimal. There is
not yet built-in support for state-machine properties, generated tuples,
discard/classification policies, parallel execution, or a standalone Catch3
runner. Those features are not implied by the initial API.

## Compatibility policy

Catch2's `Catch` namespace, established headers, macros, command-line behavior,
reporters, and coding style remain the compatibility surface. Progmasoft
features should be added under `Progmasoft::Catch3` and must not require users
to rename existing Catch2 tests. The first compatibility goal is source-level
continuity with the included Catch2 3.16.0 base; this is not a promise that every
future Catch2 upstream change will be adopted unchanged or that all downstream
projects have already been validated.

The split also applies to implementation and packaging: Catch2 remains the
engine dependency, while Progmasoft's compiled C++20 library is a separate
target. Template-based generators and call-site assertion macros remain in
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
The CMake project preserves the upstream `Catch2` targets and adds
`Progmasoft::Catch3`; the other build descriptions compile the same Catch2
engine and the Progmasoft property-test executable. See the repository build
instructions and each build file for tool-specific commands.

Registry publication is a separate release step. It requires a versioned
immutable source archive and successful validation and maintainer review for
the `catch3` module name; a submission is not a guarantee of acceptance.
