---
name: Feature request
about: Propose an improvement to Progmasoft Catch3
title: "[Feature] "
---

<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1 -->

Read [Catch3's scope and compatibility policy](https://github.com/Progmasoft/catch3/blob/devel/docs/why-catch3.md) before proposing a change. Catch3 additions and changes to the Catch2-compatible engine have different ownership and compatibility constraints.

### Problem and use case

What testing task is difficult today? Include a concrete example from a real test suite.

### Proposed behavior

Describe the user-facing API or runner behavior, with a short C++20 example if useful. Explain how failures should be reported and reproduced.

### Ownership and compatibility

- Does this belong in `Progmasoft::Catch3`, the compatibility engine, build tooling, or documentation?
- Would it change existing Catch2 headers, macros, command-line behavior, or reporters?
- Would it need compiled code, a new dependency, or a new minimum compiler version?

### Alternatives and validation

What workaround exists today? What focused test would show that the proposed behavior works?
