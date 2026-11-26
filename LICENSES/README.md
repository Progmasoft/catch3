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

- **A shared library.** A work that links Catch3 as a shared library, which
  at run time uses a copy of Catch3 already present on the user's system and
  works with a modified Catch3 that is interface-compatible with the linked
  one, meets the relinking condition that way (LGPL-3.0, section 4 d 1).
- **Static linking.** A work with Catch3 linked in statically has to give
  the recipient what they need to relink it with a modified Catch3: the
  object files or the source of the rest of the work, in a form that lets
  them be linked again (section 4 d 0).
- **Code from the headers.** Section 3 covers object code that takes
  material from Catch3's headers, as tests take its test macros and
  templates. If that material is limited to numerical parameters, data
  structure layouts and accessors, or small macros, inline functions and
  templates of ten or fewer lines, the object code may be conveyed on terms
  of your choice, with no condition from section 3. If it takes more, as
  Catch3's test macros and templates usually do, each copy of the object
  code needs a prominent notice that Catch3 is used in it and is covered by
  the LGPL, and copies of the GPL-3.0 and the LGPL-3.0; section 3 asks
  nothing beyond that. A work that is also linked with the compiled library
  of Catch3, as a test binary is, is a combined work as well, and section 4
  applies to it.
- **Catch3 itself.** Giving Catch3, as it is or modified, is giving the
  library, on the conditions of the GPL-3.0 for conveying it, its source
  among them. A modified Catch3 may be conveyed under the LGPL-3.0-or-later,
  or under the GNU GPL without the LGPL's additional permissions: the
  GPL-3.0 lets any conveyed copy drop additional permissions (section 7),
  and the LGPL-3.0 names that choice for a modification whose facility
  relies on an application to supply a function or data (section 2 b; the
  LGPL way then needs a good faith effort that the facility still works
  without it, section 2 a).
- **A device.** A work given as part of a consumer product (a "User Product")
  may also owe the information needed to install a modified version on the
  device (LGPL-3.0, section 4 e; GPL-3.0, section 6).
- **Within an organisation, and to contractors.** Making and running copies
  that you do not convey needs no permission beyond the license (GPL-3.0,
  section 2), and in the FSF's reading copies made and used within one
  organisation for itself are not distribution. Giving a copy to anyone
  outside it, a contractor included, is conveying. The GPL-3.0's section 2
  allows conveying a work to someone who modifies or runs it exclusively for
  you, under your direction and control and on terms that forbid copies of
  your material outside that relationship, provided that you comply with the
  license for the material whose copyright you do not control: for Catch3,
  that is its LGPL.

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
