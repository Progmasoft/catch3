#!/usr/bin/env python3

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

import os
import re
import sys
import subprocess

"""
Test that Catch2 recognizes the three sharding-related environment variables
and responds accordingly (running only the selected shard, creating the
response file, etc).

Requires 2 arguments, path to Catch2 binary to run and the output directory
for the output file.
"""
if len(sys.argv) != 3:
    print("Wrong number of arguments: {}".format(len(sys.argv)))
    print("Usage: {} test-bin-path output-dir".format(sys.argv[0]))
    exit(1)


bin_path = os.path.abspath(sys.argv[1])
output_dir = os.path.abspath(sys.argv[2])
info_file_path = os.path.join(output_dir, '{}.shard-support'.format(os.path.basename(bin_path)))

# Ensure no file exists from previous test runs
if os.path.isfile(info_file_path):
    os.remove(info_file_path)

print('bin path:', bin_path)
print('shard confirmation path:', info_file_path)

import re

# The two shards together run the two tests of the binary, each once.
total = 0
for index in range(2):
    if os.path.isfile(info_file_path):
        os.remove(info_file_path)
    env = os.environ.copy()
    env["BAZEL_TEST"] = "1"
    env["TEST_SHARD_INDEX"] = str(index)
    env["TEST_TOTAL_SHARDS"] = "2"
    env["TEST_SHARD_STATUS_FILE"] = info_file_path

    ret = subprocess.run(
        [bin_path, '--allow-no-tests', '--retries=off'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
        env=env
    )
    if ret.returncode not in (0, 42):
        print('Could not run "{}"'.format(bin_path))
        print("Return code: {}".format(ret.returncode))
        print("stdout: {}".format(ret.stdout))
        print("stderr: {}".format(ret.stderr))
        exit(1)

    summary = re.search(r'(\d+) test\(s\):', ret.stdout)
    if summary is None:
        print("Did not find the summary in stdout.")
        print("stdout:\n{}".format(ret.stdout))
        exit(1)
    total += int(summary.group(1))

    if not os.path.isfile(info_file_path):
        print("The runner did not create the expected file at path '{}'".format(info_file_path))
        exit(2)

if total != 2:
    print("The shards ran {} tests together instead of 2".format(total))
    exit(3)
