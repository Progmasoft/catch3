<a id="top"></a>
# Reports

**Contents**<br>
[The reports](#the-reports)<br>
[Custom output](#custom-output)<br>
[Where Catch2's reporters went](#where-catch2s-reporters-went)<br>

The [Catch3 runner](progmasoft-runner.md#top) writes its own reports. Catch2's
reporters, and `CATCH_REGISTER_REPORTER`, were removed with Catch2's runner:
a reporter sees one process, and a Catch3 run spreads over many.


## The reports

* **Console.** Progress while the run goes on, each failure with its
  expansion, messages, sections and [analysis](progmasoft-runner.md#what-a-failure-report-says),
  then a summary. `--verbose`, `--success`, `--quiet`, `--slowest` and
  `--color` adjust it.
* **JUnit XML**, `--junit=FILE`. One `<testcase>` per test; a failure is
  `<failure>`, a crash or timeout `<error>`, a skipped, blocked or not-run
  test `<skipped>`. Earlier failed attempts are `<rerunFailure>` and, for a
  test that passed in the end, `<flakyFailure>`. Under Bazel it is written
  to `XML_OUTPUT_FILE`.
* **JSON**, `--json=FILE`. Everything the runner knows about the run, in the
  `catch3-run` [format](progmasoft-runner.md#formats-other-tools-read).
* **Trace**, `--trace=FILE`, in the Trace Event Format.

`<Progmasoft/Catch3/Results.hpp>` holds the model the reports are written
from, and `<Progmasoft/Catch3/XmlWriter.hpp>` the JUnit writer, for tools
that build a `RunResult` themselves.


## Custom output

For output of your own:

* Read the JSON report. It is the complete record of a run, and its format
  is versioned.
* Write a [listener](event-listeners.md#top). Listeners receive Catch2's
  events in every worker process; what one prints outside a test reaches
  the runner's standard error.


## Where Catch2's reporters went

| Catch2 | Catch3 |
| --- | --- |
| `console`, `compact` | the console report |
| `junit` | `--junit=FILE` |
| `json`, `xml` | `--json=FILE` |
| `sonarqube`, `tap`, `teamcity`, `automake` | convert the JSON report, or a listener |
| `--reporter NAME::out=FILE` | `--junit=FILE`, `--json=FILE` |
| `CATCH_REGISTER_REPORTER` | a listener, or a tool reading the JSON report |

---

[Home](Readme.md#top)
