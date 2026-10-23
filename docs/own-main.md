<a id="top"></a>
# Supplying main() yourself

**Contents**<br>
[Run the tests from your own main](#run-the-tests-from-your-own-main)<br>
[Options of your own](#options-of-your-own)<br>
[Version detection](#version-detection)<br>

The easiest way to use Catch3 is its own `main`: link against
`Catch2::Catch2WithMain` (the [CMake target](cmake-integration.md#cmake-targets))
or the `catch2-with-main` pkg-config file. To write your own `main`, link
against `Catch2::Catch2` and hand the command line to the
[Catch3 runner](progmasoft-runner.md#top).

The runner starts the same binary again for its worker processes, with the
same `main`. Whatever `main` does before and after `run`, it also does in
every worker. That is usually what global set-up needs; when only the
runner's own process should do something, check whether the environment
variable `CATCH3_WORKER` is set, which it is only in workers.


## Run the tests from your own main

```cpp
#include <catch2/catch_session.hpp>

int main( int argc, char* argv[] ) {
  // your setup ...

  int result = Catch::Session().run( argc, argv );

  // your clean-up...

  return result;
}
```

`run` returns the [exit code](progmasoft-runner.md#exit-codes). A binary that
needs no `Catch::Session` may call `Progmasoft::Catch3::RunTests`, from
`<Progmasoft/Catch3/Runner.hpp>`, with the arguments as a
`std::vector<std::string>`.

_If you only want to run some set up before tests are run, it might be
simpler to use [event listeners](event-listeners.md#top) instead._


## Options of your own

Catch2's `Session::cli()` and `Session::configData()` went with Catch2's
runner. Take your own options out of the command line before passing the
rest on; the runner treats an option it does not know as an error.

```cpp
int main( int argc, char* argv[] ) {
  int height = 0;
  std::vector<char const*> arguments;
  for ( int i = 0; i < argc; ++i ) {
      std::string const argument = argv[i];
      if ( argument.rfind( "--height=", 0 ) == 0 ) {
          height = std::atoi( argument.c_str() + 9 );
      } else {
          arguments.push_back( argv[i] );
      }
  }

  return Catch::Session().run( static_cast<int>( arguments.size() ),
                               arguments.data() );
}
```

Workers are started with the runner's own options. A value the tests need
in every worker belongs in the environment, which workers inherit.


## Version detection

Catch2 provides a triplet of macros providing the header's version,

* `CATCH_VERSION_MAJOR`
* `CATCH_VERSION_MINOR`
* `CATCH_VERSION_PATCH`

these macros expand into a single number, that corresponds to the appropriate
part of the version. As an example, given single header version v2.3.4,
the macros would expand into `2`, `3`, and `4` respectively. The version of
the Catch3 runner is `Progmasoft::Catch3::kVersion` in
`<Progmasoft/Catch3/Version.hpp>`, and `--version` prints it.


---

[Home](Readme.md#top)
