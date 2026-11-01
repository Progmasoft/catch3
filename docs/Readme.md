<a id="top"></a>
# Catch3 documentation

Catch3 keeps Catch2's API for writing tests, runs them with
[its own runner](progmasoft-runner.md#top) and adds Progmasoft's C++17
extensions. Start with the [tutorial](tutorial.md#top), then read
[the runner](progmasoft-runner.md#top), [what Catch3 adds](progmasoft-catch3.md),
[native property testing](progmasoft-properties.md), [the roadmap](progmasoft-roadmap.md) and
[why Catch3 exists](why-catch3.md).

Every header is under `Progmasoft/Catch3/`, and the library is
`Catch3::Catch3` (`find_package(Catch3)`, `//:catch3` in Bazel). The pages
marked as compatibility references describe the test-writing API Catch3
shares with Catch2: the macros and the `Catch::` namespace. Progmasoft's own
API lives in the `Progmasoft::Catch3` namespace. Catch3 is not an official
Catch2 release.

## Reference

**Writing tests:**
* [Assertion macros](assertions.md#top)
* [Matchers (asserting complex properties)](matchers.md#top)
* [Comparing floating point numbers](comparing-floating-point-numbers.md#top)
* [Logging macros](logging.md#top)
* [Test cases and sections](test-cases-and-sections.md#top)
* [Test fixtures](test-fixtures.md#top)
* [Explicitly skipping, passing, and failing tests at runtime](skipping-passing-failing.md#top)
* [Reports](reporters.md#top)
* [Event Listeners](event-listeners.md#top)
* [Data Generators (value parameterized tests)](generators.md#top)
* [Other macros](other-macros.md#top)
* [Micro benchmarking](benchmarks.md#top)

**Fine tuning:**
* [Supplying your own main()](own-main.md#top)
* [Compile-time configuration](configuration.md#top)
* [String Conversions](tostring.md#top)

**Running:**
* [The Catch3 runner](progmasoft-runner.md#top)
* [Mocking](progmasoft-mocking.md#top)
* [Command line reference](command-line.md#top)
* [Running specific section/generator](filtering-execution-path.md#top)

**Odds and ends:**
* [Frequently Asked Questions (FAQ)](faq.md#top)
* [Best practices and other tips](usage-tips.md#top)
* [CMake integration](cmake-integration.md#top)
* [Tooling integration (CI, test runners, other)](ci-and-misc.md#top)
* [Known limitations](limitations.md#top)
* [Thread safety of the compatibility engine](thread-safety.md#top)

**Other:**
* [Release notes](release-notes.md#top)
* [Contributing](contributing.md#top)
* [Migrating from Catch2 v2 to v3](migrate-v2-to-v3.md#top)
