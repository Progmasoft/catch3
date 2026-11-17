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
machines and in your own CI conveys nothing. A conveyed work that contains
Catch3 has, among other things, to:

- say prominently that it uses Catch3 and that Catch3 is under the LGPL, and
  come with copies of the GPL-3.0 and LGPL-3.0 texts;
- give the source of the Catch3 it contains, with any change made to it;
- let the recipient relink the work with a modified Catch3: by linking Catch3
  as a shared library, or by giving the object files of the rest of the work.

The license texts in this directory hold the full conditions; this summary
does not replace them.

## Earlier releases

Releases before 0.5.0 keep the license they were published under:
MPL-2.0 with the Progmasoft Linking Exception for the Progmasoft-authored files.

## Keeping the boundary

New Progmasoft files are kept separate from upstream files so the two license
boundaries remain visible. A patch intended for Catch2 upstream must follow the
upstream contribution instructions and must not be copied from a differently
licensed Progmasoft file without resolving that licensing boundary first.
