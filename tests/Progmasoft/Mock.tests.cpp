// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <Progmasoft/Catch3/Matchers/Contains.hpp>
#include <Progmasoft/Catch3/Matchers/String.hpp>
#include <Progmasoft/Catch3/Mock.hpp>
#include <Progmasoft/Catch3/TestMacros.hpp>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "RunnerFixtureClient.hpp"

namespace
{
    using Catch::Matchers::ContainsSubstring;
    using Catch::Matchers::StartsWith;
    using FixtureClient::ExitCode;
    using FixtureClient::RunFixture;
    using FixtureClient::Scratch;
    using Progmasoft::Catch3::Any;
    using Progmasoft::Catch3::MockFunction;
    using Progmasoft::Catch3::ScopedSeam;
    using Progmasoft::Catch3::Sequence;
    using Progmasoft::Catch3::Where;

    using Scores = std::map<std::string, int>;

    class Store
    {
    public:
        virtual ~Store() = default;
        virtual void
        Open() = 0;
        virtual int
        Get(const std::string &key) const = 0;
        virtual void
        Put(const std::string &key, int value) = 0;
        virtual void
        Put(const Scores &scores) = 0;
        virtual int
        Sum(int a, int b, int c, int d, int e, int f, int g, int h) = 0;
        virtual std::string &
        Name() = 0;
        virtual std::unique_ptr<int>
        Take() = 0;
    };

    class MockStore final: public Store
    {
    public:
        CATCH3_MOCK_METHOD(void, Open, (), override);
        CATCH3_MOCK_METHOD(int, Get, (const std::string &), const override);
        CATCH3_MOCK_OVERLOAD(void,
                             Put,
                             PutOne,
                             (const std::string &, int),
                             override);
        CATCH3_MOCK_OVERLOAD(void, Put, PutAll, (const Scores &), override);
        CATCH3_MOCK_METHOD(int,
                           Sum,
                           (int, int, int, int, int, int, int, int),
                           override);
        CATCH3_MOCK_METHOD(std::string &, Name, (), override);
        CATCH3_MOCK_METHOD(std::unique_ptr<int>, Take, (), override);
    };

    /// A mock with no base class, and so nothing after its parameters.
    class MockCounter final
    {
    public:
        CATCH3_MOCK_METHOD(int, Twice, (int));
    };

    /// A seam production code calls a free function through.
    std::function<long()> &
    ClockSeam()
    {
        static auto *seam = new std::function<long()>{ [] {
            return 0L;
        } };
        return *seam;
    }

    long
    SecondsSince(long start)
    {
        return ClockSeam()() - start;
    }

    long (*pointerSeam)() = nullptr;
} // namespace

TEST_CASE("A mock method answers as its expectation says", "[progmasoft][mock]")
{
    MockStore store;
    CATCH3_EXPECT_CALL(store.GetMock)("a").Returns(1);
    CATCH3_EXPECT_CALL(store.GetMock)(StartsWith("b")).Times(2).Returns(2);
    CATCH3_EXPECT_CALL(store.GetMock)(Any).Never();
    CATCH3_EXPECT_CALL(store.OpenMock)();
    CATCH3_EXPECT_CALL(store.PutOne)("a", 1);
    CATCH3_EXPECT_CALL(store.PutAll)();

    const Store &reading = store;
    Store &writing = store;
    writing.Open();
    CHECK(reading.Get("a") == 1);
    CHECK(reading.Get("bb") == 2);
    CHECK(reading.Get("bc") == 2);
    writing.Put("a", 1);
    writing.Put(Scores{ { "a", 1 } });
}

TEST_CASE("A mock method needs nothing after its parameters",
          "[progmasoft][mock]")
{
    MockCounter counter;
    CATCH3_EXPECT_CALL(counter.TwiceMock)(2).Returns(4);
    CHECK(counter.Twice(2) == 4);
}

TEST_CASE("Every parameter reaches an action", "[progmasoft][mock]")
{
    MockStore store;
    CATCH3_EXPECT_CALL(store.SumMock)
    (1,
     Any,
     3,
     Any,
     5,
     Any,
     7,
     Where(
         [](int v) {
             return v > 7;
         },
         "above 7"))
        .Does([](int a, int b, int c, int d, int e, int f, int g, int h) {
            return a + b + c + d + e + f + g + h;
        });
    CHECK(store.Sum(1, 2, 3, 4, 5, 6, 7, 8) == 36);
}

TEST_CASE("Calls are counted against limits", "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(tick, void());
    CATCH3_MOCK_FUNCTION(tock, int(int));
    CATCH3_EXPECT_CALL(tick)().AtLeast(2);
    CATCH3_EXPECT_CALL(tock)(1).AtMost(1).Returns(10);
    CATCH3_EXPECT_CALL(tock)(1).Between(1, 2).Returns(20);
    CATCH3_EXPECT_CALL(tock)(2).AnyTimes();

    tick();
    tick();
    tick();
    CHECK(tock(1) == 10);
    CHECK(tock(1) == 20);
    CHECK(tock(1) == 20);
    CHECK(tock(2) == 0);
}

TEST_CASE("Verify ends the expectations so far", "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(tick, int());
    CATCH3_EXPECT_CALL(tick)().Returns(1);
    CHECK(tick() == 1);
    tick.Verify();
    CATCH3_EXPECT_CALL(tick)().Returns(2);
    CHECK(tick() == 2);
}

TEST_CASE("A sequence orders calls across mocks", "[progmasoft][mock]")
{
    Sequence order;
    MockStore first;
    MockStore second;
    CATCH3_EXPECT_CALL(first.OpenMock)().InSequence(order);
    CATCH3_EXPECT_CALL(second.GetMock)(Any).Times(2).InSequence(order);
    CATCH3_EXPECT_CALL(first.GetMock)(Any).AtLeast(0).InSequence(order);
    CATCH3_EXPECT_CALL(first.PutOne)(Any, Any).InSequence(order);

    first.Open();
    second.Get("a");
    second.Get("b");
    first.Put("a", 1);
}

TEST_CASE("Results of every kind come back", "[progmasoft][mock]")
{
    MockStore store;
    std::string name = "store";
    CATCH3_EXPECT_CALL(store.NameMock)().Returns(name);
    CATCH3_EXPECT_CALL(store.TakeMock)().Returns(std::make_unique<int>(7));
    CATCH3_EXPECT_CALL(store.GetMock)("throws").Throws(
        std::runtime_error("no such key"));
    CATCH3_EXPECT_CALL(store.GetMock)("default");

    CHECK(&store.Name() == &name);
    CHECK(*store.Take() == 7);
    CHECK_THROWS_WITH(store.Get("throws"), "no such key");
    CHECK(store.Get("default") == 0);
}

TEST_CASE("Actions keep their state between calls", "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(next, int());
    int count = 0;
    CATCH3_EXPECT_CALL(next)().Times(3).Does([&count] {
        return ++count;
    });
    next();
    next();
    CHECK(next() == 3);
}

TEST_CASE("An action may call a mock", "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(outer, int());
    CATCH3_MOCK_FUNCTION(inner, int());
    CATCH3_EXPECT_CALL(inner)().Returns(4);
    CATCH3_EXPECT_CALL(outer)().Does([&inner] {
        return inner() + 1;
    });
    CHECK(outer() == 5);
}

TEST_CASE("Strings match by value, whatever their type", "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(log, void(const char *));
    CATCH3_MOCK_FUNCTION(size, void(std::size_t));
    CATCH3_EXPECT_CALL(log)("text");
    CATCH3_EXPECT_CALL(log)(nullptr);
    CATCH3_EXPECT_CALL(size)(3);

    const std::string text = "text";
    log(text.c_str());
    log(nullptr);
    size(3);
}

TEST_CASE("Calls from other threads are counted", "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(tick, int());
    CATCH3_EXPECT_CALL(tick)().Times(400).Returns(1);
    std::vector<std::thread> threads;
    for (int thread = 0; thread < 4; ++thread)
    {
        threads.emplace_back([&tick] {
            for (int call = 0; call < 100; ++call)
            {
                tick();
            }
        });
    }
    for (auto &thread : threads)
    {
        thread.join();
    }
}

TEST_CASE("A mock stands in for a free function behind a seam",
          "[progmasoft][mock]")
{
    CATCH3_MOCK_FUNCTION(clock, long());
    CATCH3_EXPECT_CALL(clock)().Returns(50L);
    {
        const ScopedSeam seam(ClockSeam(), clock.AsFunction());
        CHECK(SecondsSince(20) == 30);
    }
    CHECK(SecondsSince(0) == 0);

    {
        const ScopedSeam seam(
            pointerSeam,
            +[] {
                return 9L;
            });
        CHECK(pointerSeam() == 9);
    }
    CHECK(pointerSeam == nullptr);
}

TEST_CASE("Failures of mocks fail the test case that owns them",
          "[progmasoft][mock]")
{
    const Scratch scratch;
    const auto run = RunFixture(scratch, { "--no-history", "[mock]" });
    INFO(run.output);
    CHECK(ExitCode(run) == 42);
    const auto message = [&run](const std::string &name) {
        const auto &test = run.Test(name);
        CHECK(test["outcome"].AsString() == "failed");
        std::string text;
        for (const auto &assertion :
             test["attempts"].Items().front()["assertions"].Items())
        {
            text += assertion["message"].AsString() + "\n";
        }
        return text;
    };

    const auto unexpected = message("fixture mock unexpected call");
    CHECK_THAT(unexpected,
               ContainsSubstring("unexpected call: Get(\"b\"), expected\n"
                                 "  Get(\"a\")"));
    CHECK_THAT(message("fixture mock with no expectations"),
               ContainsSubstring("nothing was expected of this mock"));
    CHECK_THAT(message("fixture mock unmet expectation"),
               ContainsSubstring("unmet expectation: tick() needs exactly 2 "
                                 "calls, called once"));
    CHECK_THAT(message("fixture mock too many calls"),
               ContainsSubstring("too many calls: tick(), expected exactly 1 "
                                 "call of tick()"));
    CHECK_THAT(message("fixture mock out of order"),
               ContainsSubstring("call out of order: second() came before "
                                 "first()"));
    CHECK_THAT(message("fixture mock past its step"),
               ContainsSubstring("call out of order: first(), its step of the "
                                 "sequence has passed"));
    CHECK_THAT(message("fixture mock with nothing to return"),
               ContainsSubstring("nothing to return from name"));

    const auto leaving = message("fixture mock left by a failure");
    CHECK_THAT(leaving, ContainsSubstring("the test's own failure"));
    CHECK_FALSE(ContainsSubstring("unmet").match(leaving));
}
