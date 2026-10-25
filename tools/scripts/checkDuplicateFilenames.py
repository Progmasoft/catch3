#!/usr/bin/env python3

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

"""Checks that no two source files of the library share a file name.

Build tools that keep object files in one directory, such as Visual Studio
projects, would otherwise compile one over the other. Headers may share
names: they are always included with their directory.
"""

import os
import sys

files_set = set()

for root, dir, files in os.walk("src/Progmasoft"):
    for file in files:
        if not file.endswith('.cpp'):
            continue
        if file not in files_set:
            files_set.add(file)
        else:
            print("File %s is duplicate" % file)
            sys.exit(1)
