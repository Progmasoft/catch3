#!/usr/bin/env python3
#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)
# SPDX-License-Identifier: BSL-1.0

import os
import re
import datetime

from scriptCommon import catchPath, upstreamFiles
from releaseCommon import Version

root_path = os.path.join(catchPath, 'src')
starting_header = os.path.join(root_path, 'Progmasoft', 'Catch3', 'All.hpp')
output_header = os.path.join(catchPath, 'extras', 'catch_amalgamated.hpp')
output_cpp = os.path.join(catchPath, 'extras', 'catch_amalgamated.cpp')
output_third_party = os.path.join(catchPath, 'extras', 'catch_amalgamated_third_party.hpp')
third_party_path = os.path.join(catchPath, 'third_party')

# REUSE-IgnoreStart

# These are the copyright comments in each file, we want to ignore them
copyright_lines = [
'//              Copyright Catch2 Authors\n',
'// Distributed under the Boost Software License, Version 1.0.\n',
'//   (See accompanying file LICENSE.txt or copy at\n',
'//        https://www.boost.org/LICENSE_1_0.txt)\n',
'// SPDX-License-Identifier: BSL-1.0\n',
]

# The header of the amalgamated file: copyright information + explanation
# what this file is.
file_header = '''\

//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

//  Catch3 v{catch3_version}, on the Catch2 v{version_string} engine
//  Generated: {generation_time}
//  ----------------------------------------------------------
//  This file is an amalgamation of multiple different files.
//  You probably shouldn't edit it directly.
//
//  The parts that come from src/Progmasoft, the Catch3 runner, keep
//  their own copyright and license lines (MPL-2.0 WITH
//  AdditionRef-Progmasoft-Exception-2.0). The runner needs C++17.
//  ----------------------------------------------------------
'''

# The header of the file with yyjson and pugixml.
third_party_header = '''\
// Catch3 v{catch3_version}: yyjson and pugixml for catch_amalgamated.cpp
// Generated: {generation_time}
// ----------------------------------------------------------
// This file is an amalgamation of src/Progmasoft/Runner/Yyjson.inl,
// src/Progmasoft/PugiXml.inl and the libraries they include from
// third_party/. You probably shouldn't edit it directly.
//
// yyjson and pugixml keep their MIT licenses, stated in their text below;
// the rest is MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0. Only
// catch_amalgamated.cpp includes this file.
// ----------------------------------------------------------
'''

# REUSE-IgnoreEnd

# Returns file header with proper version string and generation time
def catch3_version():
    version_file = os.path.join(root_path, 'Progmasoft', 'Catch3', 'Version.hpp')
    with open(version_file, mode='r', encoding='utf-8') as input:
        match = re.search(r'kVersion = "([^"]+)"', input.read())
    if not match:
        raise RuntimeError('no kVersion in ' + version_file)
    return match.group(1)

def formatted_file_header(version):
    return file_header.format(version_string=version.getVersionString(),
                              catch3_version=catch3_version(),
                              generation_time=datetime.datetime.now())

# Which headers were already concatenated (and thus should not be
# processed again)
concatenated_headers = set()

# The engine's files are the ones that come from Catch2.
upstream_files = upstreamFiles()
include_parser = re.compile(r'\s*#include <(Progmasoft/Catch3/[^>]+)>.*')


def engine_include(line):
    """The engine header a line includes, or None."""
    m = include_parser.match(line)
    # The configuration header is generated from UserConfig.hpp.in.
    if m and ('src/' + m.group(1) in upstream_files
              or m.group(1) == 'Progmasoft/Catch3/UserConfig.hpp'):
        return m.group(1)
    return None

def concatenate_file(out, filename: str, expand_headers: bool) -> int:
    # Gathers statistics on how many headers were expanded
    concatenated = 1
    with open(filename, mode='r', encoding='utf-8') as input:
        for line in input:
            if line in copyright_lines or line.strip() == '#pragma once':
                continue
            m = engine_include(line)
            # The runner's headers are expanded at the start of the .cpp file.
            if not m and not expand_headers and progmasoft_include_parser.match(line):
                continue
            # anything that isn't a Catch2 header can just be copied to
            # the resulting file
            if not m:
                out.write(line)
                continue

            # TBD: We can also strip out include guards from our own
            # headers, but it wasn't worth the time at the time of writing
            # this script.

            # We do not want to expand headers for the cpp file
            # amalgamation but neither do we want to copy them to output
            if not expand_headers:
                continue

            next_header = m
            # We have to avoid re-expanding the same header over and
            # over again, or the header will end up with couple
            # hundred thousands lines (~300k as of preview3 :-) )
            if next_header in concatenated_headers:
                continue

            # Skip including the auto-generated user config file,
            # because it has not been generated yet at this point.
            # The code around it should be written so that just not including
            # it is equivalent with all-default user configuration.
            if next_header == 'Progmasoft/Catch3/UserConfig.hpp':
                concatenated_headers.add(next_header)
                continue

            concatenated_headers.add(next_header)
            concatenated += concatenate_file(out, os.path.join(root_path, next_header), expand_headers)

    return concatenated


def generate_header():
    with open(output_header, mode='w', encoding='utf-8') as header:
        header.write(formatted_file_header(Version()))
        header.write('#ifndef CATCH_AMALGAMATED_HPP_INCLUDED\n')
        header.write('#define CATCH_AMALGAMATED_HPP_INCLUDED\n')
        print('Concatenated {} headers'.format(concatenate_file(header, starting_header, True)))
        # Mocks are Progmasoft's own, and tests use them through this header.
        concatenate_file(header, os.path.join(root_path, 'Progmasoft', 'Catch3', 'Mock.hpp'), True)
        header.write('#endif // CATCH_AMALGAMATED_HPP_INCLUDED\n')

# The runner's files include each other as <Progmasoft/...> or, inside one
# directory, as "Name.hpp".
progmasoft_include_parser = re.compile(r'\s*#include (?:<(Progmasoft/[^>]+)>|"([A-Za-z/]+\.hpp)").*')
concatenated_runner_headers = set()

# The runner compiles yyjson and pugixml into its own code through these
# files; in the amalgamation they are one header, included where the first
# of them was.
third_party_wrappers = ['Runner/Yyjson.inl', 'PugiXml.inl']
wrapper_include_parser = re.compile(r'\s*#include "([A-Za-z./]+\.inl)".*')
third_party_files = {
    'yyjson.c': 'yyjson/yyjson.c',
    'yyjson.h': 'yyjson/yyjson.h',
    'pugixml.cpp': 'pugixml/pugixml.cpp',
    'pugixml.hpp': 'pugixml/pugixml.hpp',
    'pugiconfig.hpp': 'pugixml/pugiconfig.hpp',
}
third_party_include_parser = re.compile(r'\s*#\s*include\s+[<"]([A-Za-z.]+)[>"].*')
third_party_included = False


def concatenate_runner_file(out, filename: str, expand_headers: bool):
    """Copies a file of the runner, expanding the runner's headers it
    includes once, or dropping those includes from the .cpp files."""
    directory = os.path.dirname(filename)
    with open(filename, mode='r', encoding='utf-8') as input:
        for line in input:
            if line.strip() == '#pragma once':
                continue
            # The engine's headers are all in the amalgamated header.
            if engine_include(line):
                continue
            if wrapper_include_parser.match(line):
                global third_party_included
                if not third_party_included:
                    out.write('#include "catch_amalgamated_third_party.hpp"\n')
                    third_party_included = True
                continue
            m = progmasoft_include_parser.match(line)
            if not m:
                out.write(line)
                continue
            if m.group(1):
                header = os.path.normpath(os.path.join(root_path, m.group(1)))
            else:
                header = os.path.normpath(os.path.join(directory, m.group(2)))
            if not expand_headers or header in concatenated_runner_headers:
                continue
            concatenated_runner_headers.add(header)
            concatenate_runner_file(out, header, True)


def concatenate_third_party_file(out, filename: str, done: set):
    """Copies a file of yyjson or pugixml, with the files of the two
    libraries it includes expanded once."""
    with open(filename, mode='r', encoding='utf-8') as input:
        for line in input:
            m = third_party_include_parser.match(line)
            name = m.group(1) if m else None
            # pugixml includes its source through a macro in header-only mode.
            if line.strip() == '#\tinclude PUGIXML_SOURCE':
                name = 'pugixml.cpp'
            if name not in third_party_files:
                out.write(line)
                continue
            if name not in done:
                done.add(name)
                concatenate_third_party_file(
                    out, os.path.join(third_party_path, third_party_files[name]), done)


def generate_third_party():
    with open(output_third_party, mode='w', encoding='utf-8', newline='\n') as header:
        header.write(third_party_header.format(catch3_version=catch3_version(),
                                               generation_time=datetime.datetime.now()))
        header.write('#pragma once\n')
        done = set()
        for wrapper in third_party_wrappers:
            path = os.path.join(root_path, 'Progmasoft', wrapper)
            with open(path, mode='r', encoding='utf-8') as input:
                for line in input:
                    if line.startswith(('// Copyright', '// SPDX-License-Identifier')):
                        continue
                    m = third_party_include_parser.match(line)
                    if m and m.group(1) in third_party_files:
                        name = m.group(1)
                        if name not in done:
                            done.add(name)
                            concatenate_third_party_file(
                                header, os.path.join(third_party_path, third_party_files[name]), done)
                        continue
                    header.write(line)
    print('Wrote {}'.format(os.path.relpath(output_third_party, catchPath)))


def generate_cpp():
    from glob import glob
    cpp_files = sorted(os.path.join(catchPath, path) for path in upstream_files
                       if path.endswith('.cpp'))
    runner_headers = sorted(glob(os.path.join(root_path, 'Progmasoft', 'Runner', '*.hpp'))) + [
        os.path.join(root_path, 'Progmasoft', 'Catch3', name)
        for name in ('Results.hpp', 'Runner.hpp', 'Version.hpp', 'XmlWriter.hpp')]
    runner_files = [os.path.join(root_path, 'Progmasoft', 'Mock.cpp'),
                    os.path.join(root_path, 'Progmasoft', 'Results.cpp'),
                    os.path.join(root_path, 'Progmasoft', 'XmlWriter.cpp')] + \
        sorted(glob(os.path.join(root_path, 'Progmasoft', 'Runner', '*.cpp')))
    with open(output_cpp, mode='w', encoding='utf-8') as cpp:
        cpp.write(formatted_file_header(Version()))
        cpp.write('\n#include "catch_amalgamated.hpp"\n')
        concatenate_file(cpp, os.path.join(root_path, 'Progmasoft/Catch3/Internal/WindowsHProxy.hpp'), False)
        # The runner's headers come first: Catch::Session reaches the runner.
        for header in runner_headers:
            header = os.path.normpath(header)
            if header not in concatenated_runner_headers:
                concatenated_runner_headers.add(header)
                concatenate_runner_file(cpp, header, True)
        for file in cpp_files:
            concatenate_file(cpp, file, False)
        for file in runner_files:
            concatenate_runner_file(cpp, file, False)
    print('Concatenated {} cpp files'.format(len(cpp_files) + len(runner_files)))

if __name__ == "__main__":
    generate_header()
    generate_cpp()
    generate_third_party()


# Notes:
# * For .cpp files, internal includes have to be stripped and rewritten
# * for .hpp files, internal includes have to be resolved and included
# * The .cpp file needs to start with `#include "catch_amalgamated.hpp"
# * include guards can be left/stripped, doesn't matter
# * *.cpp files should be included sorted, to minimize diffs between versions
# * *.hpp files should also be somehow sorted -> use Progmasoft/Catch3/All.hpp as the
# *       entrypoint
# * allow disabling main in the .cpp amalgamation
