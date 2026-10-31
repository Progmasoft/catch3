<a id="top"></a>
# Native property testing

`<Progmasoft/Catch3/ForAll.hpp>` is Catch3's own property-testing engine. It
needs no third-party library. Catch2's support for RapidCheck through
RapidCheck's own Catch integration keeps working as before, but the Progmasoft
additions do not build on RapidCheck. The engine goes beyond it where property
tests usually stop being practical:

- **Shrinking of the recorded choices.** Every combination of strategies
  shrinks, including `Map`, `Filter`, `FlatMap` and state machines, and a
  shrunk input always satisfies the filters and dependencies that built it.
- **An example database.** Failures found once are replayed first on the next
  run, on any machine that shares the directory, until they are fixed.
- **Targeted search.** `Target` scores guide a search towards inputs that
  random sampling rarely reaches.
- **Coverage requirements.** `Cover` fails a property whose inputs reach an
  interesting case too rarely.
- **Ready-made oracles.** `Equivalent`, `Metamorphic`, `RoundTrip` and
  `Idempotent` check properties that need no expected result.
- **No exceptions required.** `CATCH3_PROPERTY_ASSERT` returns from the body;
  the engine never throws.

The older `CheckProperty` and `Generator` API of
[Progmasoft Catch3 additions](progmasoft-catch3.md#top) remains supported
and unchanged.

## A first property

```cpp
#include <Progmasoft/Catch3.hpp>

using namespace Progmasoft::Catch3;

TEST_CASE( "reversing twice gives the original", "[property]" ) {
    CATCH3_CHECK_FOR_ALL(
        Gen::Vectors( Gen::Integers<int>() ),
        []( const std::vector<int>& values ) {
            auto copy = values;
            std::reverse( copy.begin(), copy.end() );
            std::reverse( copy.begin(), copy.end() );
            return copy == values;
        } );
}
```

`ForAll([options,] strategy..., body)` draws one value from each strategy and
passes them to the body. The body returns `bool`, or returns `void` and uses
the property macros:

```cpp
CATCH3_CHECK_FOR_ALL(
    Gen::Integers<int>( -1000, 1000 ), Gen::Integers<int>( 1, 50 ),
    []( int value, int divisor ) {
        CATCH3_PROPERTY_ASSUME( value % divisor == 0 ); // otherwise discard
        Note( "quotient", value / divisor );           // part of the report
        CATCH3_PROPERTY_ASSERT( value / divisor * divisor == value );
    } );
```

`CATCH3_CHECK_FOR_ALL` and `CATCH3_REQUIRE_FOR_ALL` report through Catch2's
normal assertion path. `ForAll` itself returns a `PropertyResult` for code
that wants to inspect it. Catch2's own `CHECK` and `REQUIRE` should not be
used inside a property body: the engine runs the body many times while it
shrinks, and every run would be reported.

## Strategies

A `Strategy<T>` builds values of `T`. All strategies live in
`Progmasoft::Catch3::Gen`:

| Strategy | Values | Shrinks towards |
| --- | --- | --- |
| `Integers<T>(min, max)` | integers of the range | the value closest to zero |
| `Booleans()` | `true`, `false` | `false` |
| `Floats<T>(FloatOptions)` | finite values of the range; NaN and infinities on request | zero or the bound closest to it, then integral values |
| `Characters(alphabet)`, `PrintableCharacters()` | characters | the first character of the alphabet |
| `Strings(character, range)`, `Utf8Strings(range, min, max)` | byte strings; UTF-8 text without surrogates | shorter text, simpler characters |
| `Vectors(element, range)`, `UniqueVectors`, `Sets`, `Maps` | collections | fewer and simpler elements |
| `Optionals(value)` | `std::nullopt` or a value | `std::nullopt` |
| `Tuples(a, b, ...)`, `Construct<T>(a, b, ...)` | tuples, and objects built with braces | each component |
| `Permutations(values)` | orderings of a list | the original order |
| `Just(value)`, `SampledFrom(values)` | fixed values | the first value |
| `OneOf(a, b, ...)`, `Frequency({ {weight, strategy}, ... })` | one of several strategies | the first strategy |
| `Lazy(factory)`, `Recursive(base, extend, depth)` | self-referential and recursive structures | a leaf |

Every strategy has `Map(function)`, `Filter(predicate)` and
`FlatMap(function)`. `FlatMap` draws a value and then a second strategy that
depends on it, for example a size and then a vector of exactly that size.

A `SizeRange{ minimum, maximum }` bounds a collection. Without a maximum the
length grows with the size of the trial, which rises from 0 to 100 over the
first half of the trials, so the first trials try small inputs.

## How shrinking works

Each strategy builds its value from a sequence of choices, small integers that
a `ChoiceSource` hands out. During a trial the engine records them. When a
trial fails, the engine does not shrink the value; it shrinks the recorded
choices and replays them through the same strategies. Choice 0 is always the
simplest decision, and a shorter sequence is simpler than a longer one. Five
passes repeat until none helps or `MaxShrinkSteps` replays are used:

1. delete blocks of 16, 8, 4, 2 and 1 choices, which drops collection
   elements and command steps;
2. set blocks to zero;
3. lower each choice by binary search, then by single steps for conditions
   that are not monotone;
4. swap neighbours that are out of order;
5. move a unit from a choice to a later one, for sums such as `x + y > 10`.

Because the strategies are replayed, the shrunk input is always one the
strategies can produce: a filter is still satisfied, a dependent size still
matches its vector, a state machine still respects its preconditions. That is
the difference from shrinking values directly.

The report shows the minimized input, the reason, the notes, and a replay
token:

```text
property failed after 37 trials; failing trial 37, root seed 3359259219, shrink checks 112
counterexample: { 0, 100 }
reason: property returned false
replay: set PropertyOptions::Replay to "c3.3c.0-0-2s"
```

Setting `PropertyOptions::Replay` to the token runs exactly that input once,
without trials or shrinking.

## Options

| Field | Default | Meaning |
| --- | --- | --- |
| `Trials` | 100 | valid trials to run |
| `Seed` | fixed | root seed; every trial derives its own seed from it |
| `MaxShrinkSteps` | 1000 | replays the shrinker may use |
| `Replay` | none | run only the input of a replay token |
| `MaxDiscardRatio` | 10 | discarded trials allowed per requested trial before the property gives up |
| `TargetSteps` | 500 | steps of targeted search when the body calls `Target` |
| `DatabasePath` | none | directory of the example database; otherwise `CATCH3_PROPERTY_DATABASE` |
| `Name` | test name and line | names the property in the database |
| `ShowStatistics` | false | adds the `Classify` and `Cover` counts to a passing report |

## Statistics and coverage requirements

`Classify(condition, label)` counts how often a condition held. `Cover(percent,
condition, label)` also requires a percentage: a property whose inputs rarely
reach an interesting case fails with `kInsufficientCoverage` instead of
passing on easy inputs.

```cpp
Cover( 10.0, values.empty(), "empty" );
Cover( 30.0, values.size() > 10, "long" );
```

## Targeted search

`Target(score, label)` reports a score. After the regular trials the engine
takes the highest-scoring input of each label and mutates its choices one at a
time, keeping a mutation when the score rises. Inputs that random sampling
almost never reaches, such as a long chain of operations or a value near an
overflow, become reachable when a score measures the distance to them.

## The example database

With `DatabasePath`, or the `CATCH3_PROPERTY_DATABASE` environment variable,
the engine stores the replay token of every minimized failure in a file named
after the property. The next run replays those tokens before any new trial, so
a failure found once, on any machine that shares the directory, is checked
first until it is fixed; a stored example that passes is removed. The engine
never fails a test because the directory is missing or unwritable.

## Model-based testing

`<Progmasoft/Catch3/Stateful.hpp>` runs random sequences of commands against a
model and the system under test side by side:

```cpp
using Model = std::deque<int>;

Command<Model, Queue> push{
    "push",
    []( const Model& ) {
        return Gen::Integers<int>( 0, 100 ).Map( []( int value ) {
            return Action<Model, Queue>{
                "push(" + std::to_string( value ) + ")",
                [value]( Model& model, Queue& queue ) {
                    model.push_back( value );
                    queue.Push( value );
                    CATCH3_PROPERTY_ASSERT( queue.Size() == model.size() );
                } };
        } );
    } };

const auto result = CheckStateful<Model, Queue>(
    options, [] { return Model{}; },
    []( const Model& ) { return Queue{}; },
    { push, pop } );
```

A command builds its action from the current model, so arguments can depend on
the state, and its precondition decides whether it may run. A failing sequence
shrinks to fewer steps with simpler arguments; the report lists the steps, for
example `{ push(0), push(0), push(0), push(0) }`.

## Ready-made oracles

`<Progmasoft/Catch3/Relations.hpp>` provides bodies for properties that need
no specification of the expected result:

| Body | Checks |
| --- | --- |
| `Equivalent(reference, candidate)` | differential testing: both return the same result |
| `Metamorphic(function, transform, relation)` | `relation(f(x), f(transform(x)))` |
| `RoundTrip(encode, decode)` | `decode(encode(x)) == x` |
| `Idempotent(function)` | `f(f(x)) == f(x)` |

```cpp
CATCH3_CHECK_FOR_ALL( Gen::Vectors( Gen::Integers<int>() ),
                      Equivalent( ReferenceSort, FastSort ) );
```

A mismatch reports both results as notes.

## Compatibility

The engine is C++17 and part of the `Catch3::Catch3` library. It does not
change the `Catch` namespace, and it needs no exceptions: a client built without them gets the
same results, except that an exception thrown by a body cannot be caught.
