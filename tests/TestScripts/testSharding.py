#!/usr/bin/env python3

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

"""
This test script verifies that sharding splits the tests without losing or
repeating any. It lists the tests once without sharding and once per shard,
runs every shard, and checks that the shards together are exactly all tests
and that each shard runs what it lists.
"""

import json
import os
import subprocess
import sys
import tempfile
from typing import List

number_of_shards = 5
spec = "[generators]~[benchmarks]"


def list_tests(self_test_exe: str, extra_args: List[str] = None) -> List[str]:
    cmd = [self_test_exe, '--list=names', spec] + (extra_args or [])
    ret = subprocess.run(cmd,
                         stdout = subprocess.PIPE,
                         stderr = subprocess.PIPE,
                         timeout = 60,
                         check = True,
                         universal_newlines = True)
    return [line for line in ret.stdout.splitlines() if line]


def execute_tests(self_test_exe: str, extra_args: List[str]) -> List[str]:
    with tempfile.TemporaryDirectory() as directory:
        report = os.path.join(directory, 'report.json')
        cmd = [self_test_exe, '--no-history', '--allow-no-tests',
               '--json=' + report, spec] + extra_args
        subprocess.run(cmd,
                       stdout = subprocess.PIPE,
                       stderr = subprocess.PIPE,
                       timeout = 120,
                       check = True,
                       universal_newlines = True)
        with open(report, encoding = 'utf-8') as file:
            run = json.load(file)
    return [test['name'] for test in run['tests']]


def main():
    self_test_exe, = sys.argv[1:]
    all_tests = list_tests(self_test_exe)
    if len(all_tests) < 2:
        raise RuntimeError("Unexpectedly few tests listed (got {})".format(len(all_tests)))
    assert all_tests == list_tests(self_test_exe, ['--shard=0/1'])

    combined = []
    for index in range(number_of_shards):
        shard = '--shard={}/{}'.format(index, number_of_shards)
        listed = list_tests(self_test_exe, [shard])
        executed = execute_tests(self_test_exe, [shard])
        assert sorted(listed) == sorted(executed), (
            "Shard {} lists\n{}\nbut runs\n{}".format(index, listed, executed))
        combined += listed

    assert sorted(all_tests) == sorted(combined), (
        "All tests and combined shards disagree.\nNo shard:\n{}\n\nCombined:\n{}\n".format(
            '\n'.join(all_tests), '\n'.join(combined)))


if __name__ == '__main__':
    sys.exit(main())
