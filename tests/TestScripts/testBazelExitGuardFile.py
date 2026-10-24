#!/usr/bin/env python3

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

"""
Checks the premature-exit guard file: the runner creates it when the run
starts and removes it when the run ends. A test that ends its process is a
crash the runner reports, so the guard is removed then too. The guard is
created only the user can read, and a symbolic link in its place is refused.

Arguments: the test binary and a directory for the guard files.
"""

import os
import subprocess
import sys


def guard_path(directory: str) -> str:
    return os.path.join(directory, 'guard_file.' + os.urandom(8).hex())


def run(cmd, env = None, expected = 0):
    print('Running:', cmd)
    result = subprocess.run(cmd,
                            stdout = subprocess.PIPE,
                            stderr = subprocess.PIPE,
                            universal_newlines = True,
                            env = env)
    if result.returncode != expected:
        print('Return code {} instead of {}'.format(result.returncode, expected))
        print('stdout:', result.stdout)
        print('stderr:', result.stderr)
        exit(1)
    return result


bin_path = os.path.abspath(sys.argv[1])
output_dir = os.path.abspath(sys.argv[2])

# Bazel names the guard in the environment.
path = guard_path(output_dir)
env = os.environ.copy()
env['BAZEL_TEST'] = '1'
env['TEST_PREMATURE_EXIT_FILE'] = path
run([bin_path, 'pass'], env)
assert not os.path.exists(path), 'the guard was left behind after a normal run'

# A test that exits the process is a crash; the run still ends normally.
path = guard_path(output_dir)
run([bin_path, '--retries=off', '--premature-exit-guard-file=' + path], expected = 42)
assert not os.path.exists(path), 'the guard was left behind after a crash'

if os.name != 'nt':
    # A stale guard with loose permissions is tightened while the run lasts.
    path = guard_path(output_dir)
    with open(path, 'w'):
        pass
    os.chmod(path, 0o666)
    run([bin_path, 'pass', '--premature-exit-guard-file=' + path])

    target_path = path + '.target'
    symlink_path = path + '.symlink'
    sentinel = 'do not truncate a symlink target\n'
    with open(target_path, 'w') as target:
        target.write(sentinel)
    os.symlink(target_path, symlink_path)
    try:
        result = subprocess.run(
            [bin_path, 'pass', '--premature-exit-guard-file=' + symlink_path],
            stdout = subprocess.PIPE,
            stderr = subprocess.PIPE,
            universal_newlines = True)
        assert result.returncode != 0, 'a symlink was accepted as the guard file'
        with open(target_path, 'r') as target:
            assert target.read() == sentinel, 'the symlink target was modified'
    finally:
        os.remove(symlink_path)
        os.remove(target_path)
