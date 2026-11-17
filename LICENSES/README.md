# License map

This repository contains two independently licensed source areas:

- Catch2-originated files retain their upstream Boost Software License 1.0
  notices and the root `LICENSE.txt`.
- Progmasoft-authored files are licensed under the GNU Lesser General Public
  License, version 3 or (at your option) any later version, and say so in
  each file with `SPDX-License-Identifier: LGPL-3.0-or-later`. The LGPL is a
  set of additional permissions on the GNU General Public License, version 3;
  both texts are in this directory.
- A few files came from Catch2 and were rewritten by Progmasoft; they carry
  both notices and `SPDX-License-Identifier: LGPL-3.0-or-later AND BSL-1.0`.
  `SECURITY.md` keeps them in `SECURITY.md.license`, the REUSE form for a file
  whose readers should not meet them, so that the policy reads as plain text.
- Vendored third-party libraries under `third_party/` keep their own licenses.
- `PATENTS.md` applies the Progmasoft Patent Grant 1.1, whose text is here, to the
  Progmasoft-authored files; it does not cover Catch2-originated or vendored files.

## Using Catch3 as a whole

Catch3 as a whole is not under the Boost Software License. The library and
its build targets (`Catch3::Catch3`, `Catch3::Catch3WithMain`, the Bazel
module and the other build files) contain Progmasoft-authored files under the
LGPL-3.0-or-later. Whoever uses Catch3 as a whole uses those files, and the
conditions of the LGPL apply to them: the BSL-1.0 of the Catch2-originated
files neither lifts nor replaces them. Only a part made of Catch2-originated
files alone, taken out of Catch3, is under the BSL-1.0 alone.

The conditions take effect when Catch3, or a work that contains it, is
conveyed to someone else: a test binary, a library or a tool with Catch3
linked in, given or sold to others. Building and running tests on your own
machines and in your own CI conveys nothing. In general a conveyed work that
contains Catch3 has to say prominently that it uses Catch3 and that Catch3 is
under the LGPL, come with copies of the GPL-3.0 and LGPL-3.0 texts, and let
the recipient have the source of that Catch3 and use a modified Catch3 with
the work.

What exactly is owed is not the same in every case. It depends on how Catch3
is built into the work and how the work is given:

- **A shared library.** A work that uses Catch3 through a shared library,
  one that runs with a modified Catch3 of the same interface put in its
  place, meets the relinking condition that way (LGPL-3.0, section 4 d 1).
- **Static linking.** A work with Catch3 linked in statically has to give
  the recipient what they need to relink it with a modified Catch3: the
  object files or the source of the rest of the work, with what it takes to
  link them again (section 4 d 0).
- **Code from the headers.** Test macros and templates of Catch3 compile into
  the tests that use them. Object code that takes from Catch3's headers only
  numerical parameters, data structure layouts and accessors, or small
  macros, inline functions and templates of ten lines or fewer, owes the
  notice and the license texts (section 3); larger parts of the headers, as
  Catch3's test macros and templates usually are, bring the conditions of a
  combined work (section 4).
- **Catch3 itself.** Giving Catch3, as it is or modified, is giving the
  library: its source goes with it, and a modified Catch3 stays under the
  LGPL-3.0-or-later (sections 2 and 4 of the LGPL, and the GPL-3.0 it builds
  on).
- **A device.** A work given as part of a consumer product (a "User Product")
  also owes the information needed to install a modified version on the
  device (LGPL-3.0, section 4 e; GPL-3.0, section 6).
- **Whether it is conveying at all.** Handing a work over within one legal
  person, or to someone who works on it only for you under the terms of the
  GPL-3.0's section 2, is not conveying it; the GPL-3.0's definitions decide.

The license texts in this directory hold the full conditions; this summary
does not replace them and is not legal advice.

## Earlier releases

Releases before 0.5.0 keep the license they were published under:
MPL-2.0 with the Progmasoft Linking Exception for the Progmasoft-authored files.

## Keeping the boundary

New Progmasoft files are kept separate from upstream files so the two license
boundaries remain visible. A patch intended for Catch2 upstream must follow the
upstream contribution instructions and must not be copied from a differently
licensed Progmasoft file without resolving that licensing boundary first.
