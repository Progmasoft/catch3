<a id="top"></a>

> **Catch3 compatibility reference:** This page describes the test-writing API Catch3 shares with Catch2: the macros, such as `TEST_CASE` and `REQUIRE`, and the `Catch::` namespace keep Catch2's names. Headers and build names are Catch3's own (`<Progmasoft/Catch3/TestMacros.hpp>`, `find_package(Catch3)`, `Catch3::Catch3WithMain`). See [Catch3 additions](progmasoft-catch3.md) for Progmasoft APIs.

# Event Listeners

An event listener responds to the events of a test run, but it is not
expected to write the run's output; that is the job of the
[reports](reporters.md#top). Instead, an event listener performs actions
within the test process, such as performing global initialization (e.g. of
a C library), or cleaning out in-memory logs if they are not needed (the
test case passed).

Every registered event listener is always active. The
[Catch3 runner](progmasoft-runner.md#top) runs tests in worker processes,
and each worker is a test run of its own for its listeners: they are created
in every worker, and see `testRunStarting` when it starts and `testRunEnded`
when it ends. What a listener prints outside a test reaches the runner's
standard error.

To write your own event listener, you should derive from `Catch::EventListenerBase`,
as it provides empty stubs for all reporter events, allowing you to
only override events you care for. Afterwards you have to register it
with Catch2 using `CATCH_REGISTER_LISTENER` macro, so that Catch2 knows
about it and instantiates it before running tests.

Example event listener:
```cpp
#include <Progmasoft/Catch3/Reporters/EventListener.hpp>
#include <Progmasoft/Catch3/Reporters/Registrars.hpp>

class testRunListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override {
        lib_foo_init();
    }
};

CATCH_REGISTER_LISTENER(testRunListener)
```

_Note that you should not use any assertion macros within a Listener!_

[You can find the list of events that the listeners can react to on its
own page](reporter-events.md#top).


---

[Home](Readme.md#top)
