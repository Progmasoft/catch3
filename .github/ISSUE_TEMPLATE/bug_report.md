---
name: Bug report
about: Report a reproducible problem in Progmasoft Catch3
title: "[Bug] "
---

<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1 -->

Before filing, check the latest code and [current limitations](https://github.com/Progmasoft/catch3/blob/devel/docs/why-catch3.md). Catch3 is an independent project, not an official Catch2 release. Please do not post credentials or private test data. Send security vulnerabilities privately to support@progmasoft.com.

### Affected surface

- [ ] Catch3 addition (property checks, snapshots, results, or XML)
- [ ] Catch2-compatible runner, assertion, or reporter
- [ ] Build, installation, or packaging
- [ ] Documentation

### What happened?

Describe the observed behavior and include the exact failure message. If the issue concerns the compatibility engine, say whether it also reproduces with upstream Catch2.

### What did you expect?

Describe the expected behavior and the contract or documentation that supports it.

### Minimal reproduction

Provide a small C++20 test, the command used to build and run it, and the test-runner arguments. For a property failure, include the root seed, replay seed, trial number, and minimized counterexample when available. For a snapshot failure, describe the snapshot mode and file location without pasting private content.

### Environment

- Catch3 commit or release:
- OS and architecture:
- Compiler and version:
- Build system and version (Bazel, CMake, Meson, Xmake, or Premake):
- Relevant flags or configuration:

### Additional context

Attach sanitized logs or screenshots if they help. State whether the problem reproduces on a clean checkout.
