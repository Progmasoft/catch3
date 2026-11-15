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
- Vendored third-party libraries under `third_party/` keep their own licenses.

Releases before 0.5.0 keep the license they were published under:
MPL-2.0 with the Progmasoft Linking Exception for the Progmasoft-authored files.

New Progmasoft files are kept separate from upstream files so the two license
boundaries remain visible. A patch intended for Catch2 upstream must follow the
upstream contribution instructions and must not be copied from a differently
licensed Progmasoft file without resolving that licensing boundary first.
