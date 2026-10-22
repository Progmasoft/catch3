<a id="top"></a>
# Catch3 roadmap

Catch3 grows from a Catch2-compatible test framework into a testing toolkit:
unit testing, property testing, mocking, coverage, mutation testing, test
generation and test selection, driven by a runner of its own. Two rules hold
for every step:

- **Catch2 stays compatible.** The `Catch` namespace, the `catch2/` headers,
  the macros, the command line and the reporters keep working unchanged. New
  capabilities live under `Progmasoft::Catch3` and `Progmasoft/Catch3/`.
- **Nothing is claimed before it is tested.** A feature is listed as done when
  its tests run in CI on Windows, Linux and macOS.

The steps are ordered by what each one builds on.

| Step | Capability | State |
| --- | --- | --- |
| 1 | Native property testing: choice-sequence shrinking, combinators, state machines, statistics, targeted search, example database, differential and metamorphic oracles | done; see [Native property testing](progmasoft-properties.md#top) |
| 2 | The `catch3` runner | planned |
| 3 | Mocking | planned |
| 4 | Per-test code coverage | planned |
| 5 | Change-impact test selection and test prioritization | planned |
| 6 | Mutation testing | planned |
| 7 | Coverage-, mutation- and history-guided test generation | planned |

## 2. The `catch3` runner

Catch2's runner executes the tests of one binary in one process. The features
that follow need more: they run many binaries, each test in isolation, under
coverage or mutation, and decide which tests to run and in which order. The
`catch3` runner is a separate program that drives Catch2 test binaries from
the outside, so existing binaries need no change:

- it discovers test cases with `--list-tests --reporter json` and runs them
  with `--reporter json`, both stable parts of Catch2's command line;
- it runs tests in parallel processes, shards them across machines, enforces
  per-test timeouts, isolates crashes, and retries flaky tests with a record
  of every attempt;
- it keeps a history of results and durations, the input for prioritization;
- it writes JUnit XML, JSON and a terminal report, and keeps Catch2's exit
  codes;
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
