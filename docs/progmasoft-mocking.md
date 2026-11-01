<a id="top"></a>

# Mocking

`<Progmasoft/Catch3/Mock.hpp>` replaces a dependency of the code under test
with a mock that checks how it is called. Mocks report through Catch's
assertion path: an unexpected call, a call out of order and an unmet
expectation fail the test case that owns the mock, at the line that declared
the expectation, in every report the runner writes.

```cpp
#include <Progmasoft/Catch3/Mock.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>

class Store
{
public:
    virtual ~Store() = default;
    virtual int Get(const std::string &key) const = 0;
    virtual void Put(const std::string &key, int value) = 0;
};

class MockStore final : public Store
{
public:
    CATCH3_MOCK_METHOD(int, Get, (const std::string &), const override);
    CATCH3_MOCK_METHOD(void, Put, (const std::string &, int), override);
};

TEST_CASE("A score goes up by one")
{
    MockStore store;
    CATCH3_EXPECT_CALL(store.GetMock)("ada").Returns(41);
    CATCH3_EXPECT_CALL(store.PutMock)("ada", 42);

    AddPoint(store, "ada");
}
```

A mock verifies itself in its destructor, so nothing has to be called at the
end of the test case. When the test case is already being left by a failure or
a skip, the expectations it did not reach are not reported on top of it.
`Verify()` checks the expectations earlier and forgets them, for a test case
that goes on with new ones.

With the amalgamated files, `catch_amalgamated.hpp` holds all of this.

## Declaring mocks

`CATCH3_MOCK_METHOD(result, name, (types), specifiers)` declares the member
function `name` and the mock `nameMock` beside it. The specifiers are whatever
follows the parameters: `const`, `noexcept`, `override`. Up to 8 parameters are
supported, and a type with a comma in it, such as `std::map<std::string, int>`,
needs an alias.

Overloads need mocks with names of their own:

```cpp
CATCH3_MOCK_OVERLOAD(void, Put, PutOne, (const std::string &, int), override);
CATCH3_MOCK_OVERLOAD(void, Put, PutAll, (const Scores &), override);
```

`CATCH3_MOCK_FUNCTION(name, signature)` declares a `MockFunction` on its own.

## Setting expectations

`CATCH3_EXPECT_CALL(mock)(arguments...)` expects one call. Without arguments it
accepts any arguments; otherwise each parameter takes one of these:

| Written as | Accepts |
| --- | --- |
| a value | what compares equal to it; strings compare by value, also as `const char *`, and integers by value whatever their types, so 256 never accepts an `unsigned char` |
| a Catch matcher, such as `StartsWith("a")` | what the matcher accepts |
| `Any` | anything |
| `Where(predicate, "description")` | what the predicate accepts |

A composition such as `StartsWith("a") && EndsWith("z")` refers to temporaries
that are gone when the mock is called, and Catch's vector matchers, such as
`Equals(vector)`, refer to their vector. Neither compiles here; put them in a
`Where`. A matcher of your own has to hold what it compares with.

The result of `CATCH3_EXPECT_CALL` takes these, in any order:

| Member | Meaning |
| --- | --- |
| `Times(n)`, `Between(a, b)`, `AtLeast(n)`, `AtMost(n)`, `AnyTimes()`, `Never()` | how many calls it wants; one by default |
| `Returns(value)` | the result of each call; a type that cannot be copied moves out on the first one, and a reference result refers to `value` |
| `Does(function)` | runs `function` with the arguments, and returns what it returns; it keeps its state between calls |
| `Throws(exception)` | throws `exception` from each call |
| `InSequence(sequence)` | see below |

A call goes to the first expectation, in the order they were declared, that
accepts its arguments and can take another call. A call none of them accepts
fails the test case and returns a value-initialized result. So does a call they
accept when all of them are used up. When the result cannot be made that way, a
reference for example, the test case ends there; the same happens to an
expectation with no `Returns` or `Does`. A `noexcept` function cannot leave the
test case that way, so such a call ends the process.

A mock with no expectation for a call treats it as a failure. An expectation
with `AnyTimes()` is how to allow calls that do not matter to the test.

## Sequences

Expectations in a `Sequence` have to be met in the order they were declared in,
across mocks:

```cpp
Sequence order;
CATCH3_EXPECT_CALL(file.OpenMock)().InSequence(order);
CATCH3_EXPECT_CALL(file.WriteMock)(Any).AtLeast(1).InSequence(order);
CATCH3_EXPECT_CALL(log.FlushMock)().InSequence(order);
```

A step that is passed takes no more calls. A step reached before the ones ahead
of it have their calls fails the test case and names them.

## Free functions

Code that reaches a free function through a seam, a `std::function` or a
function pointer it calls through, can be pointed at a mock. `ScopedSeam` puts
the seam back at the end of its scope:

```cpp
CATCH3_MOCK_FUNCTION(clock, long());
CATCH3_EXPECT_CALL(clock)().Returns(50L);

const ScopedSeam seam(clockSeam, clock.AsFunction());
CHECK(SecondsSince(20) == 30);
```

The mock has to outlive what `AsFunction` returns. A function pointer can only
take a function without state, so it reaches a mock of static lifetime through
one.

## Threads

Mocks count and order calls correctly when several threads call them. Reports
go through Catch's assertions, which are not safe across threads unless
`CATCH_CONFIG_EXPERIMENTAL_THREAD_SAFE_ASSERTIONS` is set, so a failure in a
thread other than the test's needs that setting.

Outside of a test case, such as in the destructor of a global, there is nothing
to fail: failures go to standard error.
