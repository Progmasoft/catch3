#!/usr/bin/env python3

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

import os
import re
import sys
import xml.etree.ElementTree as ET
import subprocess

"""
Test that the runner recognizes the `XML_OUTPUT_FILE` environment variable
of Bazel and writes a JUnit report to the path it names.

Requires 2 arguments, path to the test binary and the output directory for
the report.
"""

if len(sys.argv) != 3:
    print("Wrong number of arguments: {}".format(len(sys.argv)))
    print("Usage: {} test-bin-path output-dir".format(sys.argv[0]))
    exit(1)


bin_path = os.path.abspath(sys.argv[1])
output_dir = os.path.abspath(sys.argv[2])
xml_out_path = os.path.join(output_dir, '{}.xml'.format(os.path.basename(bin_path)))

# Ensure no file exists from previous test runs
if os.path.isfile(xml_out_path):
    os.remove(xml_out_path)

print('bin path:', bin_path)
print('xml out path:', xml_out_path)

env = os.environ.copy()
env["XML_OUTPUT_FILE"] = xml_out_path

ret = subprocess.run(
    [bin_path, '--retries=off'],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    universal_newlines=True,
    env=env
)
# One of the tests fails on purpose.
if ret.returncode != 42:
    print("Unexpected return code {}".format(ret.returncode))
    print("stdout: {}".format(ret.stdout))
    print("stderr: {}".format(ret.stderr))
    exit(1)

try:
    tree = ET.parse(xml_out_path)
except ET.ParseError as ex:
    print("Invalid XML: '{}'".format(ex))
    raise
except FileNotFoundError as ex:
    print("Could not find '{}'".format(xml_out_path))
    raise

bin_name = os.path.splitext(os.path.basename(bin_path))[0]
if tree.find('.//testsuite[@name="{}"]'.format(bin_name)) is None:
    print("Could not find '{}' testsuite".format(bin_name))
    exit(2)

summary = re.search(r'(\d+) test\(s\): (\d+) passed, (\d+) failed', ret.stdout)
if summary is None:
    print("Could not find test summary in {}".format(ret.stdout))
    exit(2)

total, passed, failed = [int(value) for value in summary.groups()]
if len(tree.findall('.//testcase')) != total:
    print("Unexpected number of test cases!")
    exit(2)

if len(tree.findall('.//failure')) != failed:
    print("Unexpected number of test failures!")
    exit(2)

if (passed + failed) != total:
    print("Something has gone very wrong, ({} + {}) != {}".format(passed, failed, total))
    exit(2)
