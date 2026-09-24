<a id="top"></a>
![Catch2 logo](data/artwork/catch2-logo-full-with-background.svg)

> **Progmasoft Catch3 is an independently maintained Catch2-derived project.**
> It currently keeps Catch2 3.16.0's test runner and compatibility API as its
> engine, and adds a separate C++20 property-testing layer. Catch2-originated
> files retain their upstream license; new Progmasoft APIs use the terms stated
> in each file. Catch3 is not an official Catch2 release or endorsed by Catch2's
> maintainers. [Why Catch3 exists and how it differs](docs/why-catch3.md).


## What is Catch2?

Catch2 is mainly a unit testing framework for C++, but it also
provides basic micro-benchmarking features, and simple BDD macros.

Catch2's main advantage is that using it is both simple and natural.
Test names do not have to be valid identifiers, assertions look like
normal C++ boolean expressions, and sections provide a nice and local way
to share set-up and tear-down code in tests.

**Example unit test**
```cpp
#include <catch2/catch_test_macros.hpp>

#include <cstdint>

uint32_t factorial( uint32_t number ) {
    return number <= 1 ? number : factorial(number-1) * number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( factorial( 1) == 1 );
    REQUIRE( factorial( 2) == 2 );
    REQUIRE( factorial( 3) == 6 );
    REQUIRE( factorial(10) == 3'628'800 );
}
```

**Example microbenchmark**
```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cstdint>

uint64_t fibonacci(uint64_t number) {
    return number < 2 ? number : fibonacci(number - 1) + fibonacci(number - 2);
}

TEST_CASE("Benchmark Fibonacci", "[!benchmark]") {
    REQUIRE(fibonacci(5) == 5);

    REQUIRE(fibonacci(20) == 6'765);
    BENCHMARK("fibonacci 20") {
        return fibonacci(20);
    };

    REQUIRE(fibonacci(25) == 75'025);
    BENCHMARK("fibonacci 25") {
        return fibonacci(25);
    };
}
```

_Note that benchmarks are not run by default, so you need to run it explicitly
with the `[!benchmark]` tag._


## Project status

This repository is based on Catch2's v3 development line and currently identifies
its compatibility engine as version 3.16.0. It is an early-stage fork: existing
Catch2 functionality remains the foundation, while Progmasoft additions are
being introduced in their own namespace and include tree. See the
[compatibility and difference notes](docs/why-catch3.md) before treating this as
a drop-in replacement in production.


## How to use it
This documentation comprises these three parts:

* [Why do we need yet another C++ Test Framework?](docs/why-catch.md#top)
* [Tutorial](docs/tutorial.md#top) - getting started
* [Reference section](docs/Readme.md#top) - all the details


## More
* Catch3 differences and current limitations: [docs/why-catch3.md](docs/why-catch3.md)
* Progmasoft property-testing API and integration: [docs/progmasoft-catch3.md](docs/progmasoft-catch3.md)
* Catch2 upstream history and migration reference: [Catch2's repository](https://github.com/catchorg/Catch2)
* Issues and bugs for this fork can be raised on the [Progmasoft/Catch3 issue tracker](https://github.com/Progmasoft/catch3/issues)
* Catch2's upstream documentation on [open-source users](docs/opensource-users.md#top)
or [commercially](docs/commercial-users.md#top).
