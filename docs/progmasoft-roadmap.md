<a id="top"></a>
# Catch3 roadmap

Catch3 grows from a Catch2-compatible test framework into a testing toolkit:
unit testing, property testing, mocking, coverage, mutation testing, test
generation and test selection, driven by a runner of its own. Two rules hold
for every step:

- **Catch2's test-writing API stays compatible.** The macros and the `Catch`
  namespace, with its matchers, generators and listeners, keep working
  unchanged. Everything else is Catch3's own: the runner replaced Catch2's
  session, command line and reporters, the headers are under
  `Progmasoft/Catch3/` and the build names are `Catch3`. New capabilities
  live in the `Progmasoft::Catch3` namespace.
- **Nothing is claimed before it is tested.** A feature is listed as done when
  its tests run in CI on Windows, Linux and macOS.

The steps are ordered by what each one builds on.

| Step | Capability | State |
| --- | --- | --- |
| 1 | Native property testing: choice-sequence shrinking, combinators, state machines, statistics, targeted search, example database, differential and metamorphic oracles | done; see [Native property testing](progmasoft-properties.md#top) |
| 2 | The Catch3 runner | done; see [The Catch3 runner](progmasoft-runner.md#top) |
| 3 | Mocking | planned |
| 4 | Per-test code coverage | planned |
| 5 | Change-impact test selection and test prioritization | planned |
| 6 | Mutation testing | planned |
| 7 | Coverage-, mutation- and history-guided test generation | planned |

## 2. The Catch3 runner

Catch2's runner executed the tests of one binary in one process. The
features that follow need more: they run each test in isolation, under
coverage or mutation, and decide which tests to run and in which order. The
Catch3 runner replaced Catch2's in every test binary:

- tests run in parallel worker processes that take the next test when they
  are free; a crash or hang ends one test, not the run;
- failures are retried alone, which tells deterministic, flaky and
  order-dependent failures apart, and the test that leaves state behind is
  found by bisection;
- a history of verdicts and durations drives quarantine, adaptive time
  limits, predictive ordering, incremental runs, replay and analytics;
- tests are selected by change, by past failure or by prediction, and
  sharded by a stable hash or by duration;
- tags declare dependencies, shared resources, serial tests, time limits and
  retries;
- every failure gets a category, a signature and an analysis;
- reports are console, JUnit XML with flaky and rerun elements, JSON and a
  trace; the discovery listing has a fixed, versioned format;
- reports of other frameworks are imported, so their tests get the same
  history, flaky detection, quarantine and analytics;
- every test gets a temporary directory of its own;
- its command line is parsed by a hand-written, typed parser, like the other
  Progmasoft tools.

## 3. Mocking

Mocks that report through Catch2's assertion path: expectations with call
counts, argument matchers built on Catch2's matchers, sequences, and actions,
for virtual interfaces and for free functions behind a seam. Unexpected calls
and unmet expectations fail the test case that owns the mock.

## 4. Per-test code coverage

The runner runs each test case of an instrumented binary on its own and keeps
one coverage profile per test: LLVM source-based coverage with Clang and GCC's
gcov format with GCC. The result is a map from each test to the lines and
branches it reaches, plus the usual combined report.

## 5. Change-impact selection and prioritization

With the per-test map, the runner selects the tests whose covered lines a
change touches, taking the change from Git, and runs those first. Ordering
uses failure history, duration and coverage of the change, so that a failing
run fails early. A full run remains one option away, and the selection is
reported, never silent.

## 6. Mutation testing

The runner makes small changes to the code under test, such as a flipped
comparison or a removed call, rebuilds, and runs the tests that cover the
changed line. A mutation that no test detects points at a missing assertion.
The per-test coverage map keeps this affordable: only covering tests run for
each mutant.

## 7. Test generation

The property engine already records choice sequences. Test generation feeds
those sequences from three sources:

- **coverage-guided:** choice sequences that reach new coverage are kept and
  mutated further, as a fuzzer keeps inputs, so property tests explore code
  that random inputs miss;
- **mutation-guided:** inputs are searched for that tell a surviving mutant
  from the original program, and become new regression examples;
- **incremental:** examples found in earlier runs, kept in the example
  database, seed the next run, so the search continues where it stopped.

Generated examples are written as ordinary Catch2 test cases or stored replay
tokens, so they run without the generator.
