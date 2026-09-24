# License map

This repository contains two independently licensed source areas:

- Catch2-originated files retain their upstream Boost Software License 1.0
  notices and the root `LICENSE.txt`.
- Progmasoft-authored files identify their terms in each file with
  `SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1`.
  The canonical MPL-2.0 text and Progmasoft linking exception are in this
  directory.

New Progmasoft files are kept separate from upstream files so the two license
boundaries remain visible. A patch intended for Catch2 upstream must follow the
upstream contribution instructions and must not be copied from a differently
licensed Progmasoft file without resolving that licensing boundary first.
