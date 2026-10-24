#!/usr/bin/env python3

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

from ConfigureTestsCommon import configure_and_build, run_and_return_output

import json
import os
import re
import sys

"""
Tests the CMake configure option for CATCH_CONFIG_EXPERIMENTAL_REDIRECT

Requires 2 arguments, path folder where the Catch2's main CMakeLists.txt
exists, and path to where the output files should be stored.
"""

if len(sys.argv) != 3:
    print('Wrong number of arguments: {}'.format(len(sys.argv)))
    print('Usage: {} catch2-top-level-dir base-build-output-dir'.format(sys.argv[0]))
    exit(1)

catch2_source_path = os.path.abspath(sys.argv[1])
build_dir_path = os.path.join(os.path.abspath(sys.argv[2]), 'CMakeConfigTests', 'ExperimentalRedirect')

configure_and_build(catch2_source_path,
                    build_dir_path,
                    [("CATCH_CONFIG_EXPERIMENTAL_REDIRECT", "ON")])

report_path = os.path.join(build_dir_path, 'has-printf.json')
run_and_return_output(os.path.join(build_dir_path, 'tests'),
                      'SelfTest',
                      ['--no-history', '--json=' + report_path, 'has printf'])

# The text printf writes must be captured as the test's standard output.
with open(report_path, encoding='utf-8') as report_file:
    report = json.load(report_file)
outputs = [attempt.get('standardOutput', '')
           for test in report['tests']
           for attempt in test['attempts']]
if outputs != ['loose text artifact\n']:
    print('Unexpected captured output: {}'.format(outputs))
    exit(2)
