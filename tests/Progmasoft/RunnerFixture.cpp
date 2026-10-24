// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

// A test binary whose tests misbehave on purpose. The runner's integration
// tests start it and check what the runner made of each misbehaviour.

#include <Progmasoft/Catch3/Runner.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace
{
    /// The value of the environment variable `name`, if it is set.
    std::optional<std::string>
    Environment(const char *name)
    {
#if defined(_WIN32)
        char *value = nullptr;
        std::size_t length = 0;
        if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
        {
            return std::nullopt;
        }
        std::string result{ value };
        std::free(value);
        return result;
#else
        const char *value = std::getenv(name);
        return value == nullptr ? std::nullopt
                                : std::optional<std::string>{ value };
#endif
    }

    /// A directory the integration test gives every run of the fixture, to
    /// count attempts across worker processes.
    std::filesystem::path
    StateDirectory()
    {
        const auto directory = Environment("CATCH3_FIXTURE_STATE");
        return directory ? std::filesystem::path{ *directory }
                         : std::filesystem::path{};
    }

    /// How often `name` ran before, counted in a file; increments it.
    int
    CountRun(const std::string &name)
    {
        const auto file = StateDirectory() / (name + ".count");
        int count = 0;
        {
            std::ifstream input(file);
            input >> count;
        }
        std::ofstream output(file, std::ios::trunc);
        output << count + 1;
        return count;
    }

    bool polluted = false;
} // namespace

TEST_CASE("fixture passes", "[fixture][pass]")
{
    REQUIRE(1 + 1 == 2);
}

TEST_CASE("fixture prints", "[fixture][pass]")
{
    std::cout << "said through std::cout\n";
    std::cerr << "said through std::cerr\n";
    CHECK(true);
}

TEST_CASE("fixture fails", "[fixture][fail]")
{
    const int expected = 3;
    SECTION("in a section")
    {
        CHECK(1 + 1 == expected);
    }
}

TEST_CASE("fixture throws", "[fixture][fail]")
{
    throw std::runtime_error("broken on purpose");
}

TEST_CASE("fixture crashes", "[fixture][crash]")
{
    SECTION("deep down")
    {
        CHECK(true);
        std::abort();
    }
}

TEST_CASE("fixture hangs", "[fixture][hang][!timeout:1]")
{
    CHECK(true);
    std::this_thread::sleep_for(std::chrono::seconds(60));
}

TEST_CASE("fixture is flaky", "[fixture][flaky]")
{
    // Fails the first time it runs, passes afterwards.
    REQUIRE(CountRun("flaky") > 0);
}

TEST_CASE("fixture alternates", "[fixture][alternate]")
{
    // Fails on every even run: flaky on every run of the runner.
    REQUIRE(CountRun("alternates") % 2 == 1);
}

TEST_CASE("fixture innocent one", "[fixture][order]")
{
    CHECK(true);
}

TEST_CASE("fixture polluter", "[fixture][order]")
{
    polluted = true;
    CHECK(true);
}

TEST_CASE("fixture innocent two", "[fixture][order]")
{
    CHECK(true);
}

TEST_CASE("fixture victim", "[fixture][order]")
{
    REQUIRE_FALSE(polluted);
}

TEST_CASE("fixture base", "[fixture][deps]")
{
    FAIL("the base is broken");
}

TEST_CASE("fixture dependent", "[fixture][deps][!depends:fixture base]")
{
    CHECK(true);
}

TEST_CASE("fixture temporary directory", "[fixture][temp]")
{
    const auto directory = Progmasoft::Catch3::TestTemporaryDirectory();
    REQUIRE(std::filesystem::exists(directory));
    REQUIRE(std::filesystem::equivalent(std::filesystem::temp_directory_path(),
                                        directory));
    std::ofstream(directory / "scratch.txt") << "kept only on failure";
    CHECK_FALSE(Environment("CATCH3_FIXTURE_FAIL_TEMP").has_value());
}

TEST_CASE("fixture slow one", "[fixture][slow]")
{
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(true);
}

TEST_CASE("fixture slow two", "[fixture][slow]")
{
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(true);
}

TEST_CASE("fixture slow three", "[fixture][slow]")
{
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(true);
}

TEST_CASE("fixture slow four", "[fixture][slow]")
{
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(true);
}

TEST_CASE("fixture skips", "[fixture][skip]")
{
    SKIP("not on this machine");
}
