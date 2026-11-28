<a id="top"></a>

# Death tests

`<Progmasoft/Catch3/Death.hpp>` checks that a statement ends the process: an
`abort()` on a broken invariant, an `exit(3)` on bad input, a crash on a null
pointer. The statement runs in a child process, so the test that checks it
goes on, and every other test of the binary is untouched.

```cpp
#include <Progmasoft/Catch3/Death.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>

using namespace Progmasoft::Catch3;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("A broken invariant stops the program")
{
    Buffer buffer(4);
    CATCH3_CHECK_DEATH(buffer.At(9));
    CATCH3_CHECK_DEATH_THAT(buffer.At(9), ContainsSubstring("out of range"));
    CATCH3_CHECK_EXIT(Parse("--size=-1"), ExitedWith{2});
    CATCH3_CHECK_EXIT(buffer.Release(nullptr), KilledBySignal{SIGSEGV});
}
```

## The macros

| Macro | Passes when |
| --- | --- |
| `CATCH3_CHECK_DEATH(statement)` | the statement ends the process: it exits with a status other than 0, or a signal or an exception ends it |
| `CATCH3_CHECK_DEATH_THAT(statement, matcher)` | as above, and what the statement wrote to standard error matches `matcher` |
| `CATCH3_CHECK_EXIT(statement, ending)` | the statement ends the process as `ending` says: `ExitedWith{code}`, `KilledBySignal{signal}` (POSIX), or any callable that takes a `DeathResult` |

Each has a `REQUIRE` form that ends the test case on failure. A statement
that returns or throws fails the check, and the report says which. Only an
exit with status 0 needs `CATCH3_CHECK_EXIT(statement, ExitedWith{0})`;
`CATCH3_CHECK_DEATH` does not count it as dying.

`Detail::RunDeathStatement` returns the `DeathResult` itself, for code that
wants to inspect the ending, the code and the text.

## How it works

The test binary starts itself again with `--in-process` and the name of the
current test, and a variable that names the statement by its file, its line
and how many times the test has reached that line. The child runs the test up
to that statement, gives the fatal signals their default actions, runs the
statement and ends. Another death statement of the same test is skipped in
the child. The parent reads how the child ended and what the statement wrote
to standard error after it began.

This works the same on Linux, macOS and Windows: no `fork`, so a test that
started threads before the statement is safe. It also means:

- The test must reach the statement the same way every time. A statement
  inside a condition that depends on the time or on randomness is reported as
  not reached.
- What the test does before the statement runs again in the child. Its side
  effects, such as files it writes, happen twice; keep them before the
  statement idempotent, or put the statement first.
- The child reads no project file, keeps no history, writes no report, and
  does not see the filter or shard Bazel gives the parent.
- A statement that never ends keeps the test waiting until the runner's time
  limit ends it.
- On Windows `abort()` ends with exit code 3; `KilledBySignal` is for POSIX
  systems, where a signal ends the process.

The amalgamated `catch_amalgamated.hpp` and `.cpp` include death tests.
