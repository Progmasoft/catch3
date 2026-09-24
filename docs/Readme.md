<a id="top"></a>
# Catch3 documentation

Catch3 combines the upstream Catch2-compatible runner and API with
Progmasoft's C++20 extensions. Start with the [tutorial](tutorial.md#top),
then read [what Catch3 adds](progmasoft-catch3.md) and
[why the two API families remain distinct](why-catch3.md). The compatibility
pages below retain real `catch2/` include paths, `Catch2::` CMake targets,
and upstream version history; renaming those would make examples incorrect.

The Progmasoft extension API lives under `Progmasoft/Catch3/...` and the
`Progmasoft::Catch3` namespace. Its built library is separate from the
upstream-compatible engine. The compatibility pages are about the bundled
engine, not a claim that Catch3 is an official Catch2 release.

## Reference

**Writing tests:**
* [Assertion macros](assertions.md#top)
* [Matchers (asserting complex properties)](matchers.md#top)
* [Comparing floating point numbers](comparing-floating-point-numbers.md#top)
* [Logging macros](logging.md#top)
* [Test cases and sections](test-cases-and-sections.md#top)
* [Test fixtures](test-fixtures.md#top)
* [Explicitly skipping, passing, and failing tests at runtime](skipping-passing-failing.md#top)
* [Reporters (output customization)](reporters.md#top)
* [Event Listeners](event-listeners.md#top)
* [Data Generators (value parameterized tests)](generators.md#top)
* [Other macros](other-macros.md#top)
* [Micro benchmarking](benchmarks.md#top)

**Fine tuning:**
* [Supplying your own main()](own-main.md#top)
* [Compile-time configuration](configuration.md#top)
* [String Conversions](tostring.md#top)

**Running:**
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
* [Why the Catch2 compatibility base?](why-catch.md#top)
* [Migrating from v2 to v3](migrate-v2-to-v3.md#top)
* [Historical Catch2 open-source users](opensource-users.md#top)
* [Historical Catch2 commercial users](commercial-users.md#top)
* [Upstream contribution guide (historical)](contributing.md#top)
* [Upstream release notes (historical)](release-notes.md#top)
* [Upstream deprecations (historical)](deprecations.md#top)
