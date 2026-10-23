<a id="top"></a>

> **Catch3 compatibility reference:** This page describes the bundled Catch2-compatible API. Names such as `Catch2`, `catch2/`, and `Catch2::Catch2WithMain` remain where they are actual compatibility interfaces. See [Catch3 additions](progmasoft-catch3.md) for Progmasoft APIs.

# Best practices for the Catch3 compatibility API

## Running tests

Your tests should be run in a manner roughly equivalent with:

```
./tests --order=random --warn=no-assertions
```

The [Catch3 runner](progmasoft-runner.md#top) runs them in parallel worker
processes, retries failures once alone to tell deterministic, flaky and
order-dependent failures apart, and fails tests whose leaf path contains no
assertion. Random order shakes out interference between tests that run in
the same worker; when a test fails only after another, the runner names the
other.

Interference, especially destructive interference, usually comes from
errors in the code under test, rather than the tests themselves. Tests that
share state outside the process, such as a file at a fixed path or a
database, need `[!serial]` or `[!resource:NAME]`; the runner gives every
test its own temporary directory.


<a id="parallel-tests"></a>
## Running tests in parallel

The runner already runs the tests of a binary in parallel, one worker per
hardware thread by default (`--jobs`). Across machines, split the run into
[shards](progmasoft-runner.md#choosing-and-ordering-tests):

```text
./tests --shard=0/3
./tests --shard=1/3
./tests --shard=2/3
```

A test goes to the shard a stable hash of its name chooses, so the shards
cover every test exactly once whatever the order or seed of each run. With
`--shard-durations=FILE`, a JSON report or history of an earlier run, the
shards are balanced by duration instead.

If you use CMake and CTest, [`catch_discover_tests`](cmake-integration.md#automatic-test-registration)
registers each `TEST_CASE` as a CTest test, and
[`CatchShardTests.cmake`](cmake-integration.md#catchshardtestscmake)
registers shards.


## Organizing tests into binaries

Both overly large and overly small test binaries can cause issues. Overly
large test binaries have to be recompiled and relinked often, and the
link times are usually also long. Overly small test binaries in turn pay
significant overhead from linking against Catch2 more often per compiled
test case, and also make it hard/impossible to run tests in batches.

Because there is no hard and fast rule for the right size of a test binary,
I recommend having 1:1 correspondence between libraries in project and test
binaries. (At least if it is possible, in some cases it is not.) Having
a test binary for each library in project keeps related tests together,
and makes tests easy to navigate by reflecting the project's organizational
structure.


---

[Home](Readme.md#top)
