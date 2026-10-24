#!/usr/bin/env python3

"""
Checks that all of the "All.hpp" headers of the engine include all subheaders.

The logic is simple: given a folder, e.g. `Progmasoft/Catch3/Matchers`, the
corresponding header is `Progmasoft/Catch3/Matchers/All.hpp` and contains
* all engine headers in `Progmasoft/Catch3/Matchers`,
* all engine headers in `Progmasoft/Catch3/Matchers/{Internal, Detail}`,
* the `All.hpp` headers of its other subfolders that hold engine headers.

The top level header is `Progmasoft/Catch3/All.hpp`. Engine headers are the
ones listed in upstreamFiles.txt; Progmasoft's own headers in the same
folders are not part of the convenience headers.
"""

internal_dirs = ['Detail', 'Internal']

from scriptCommon import catchPath, upstreamFiles
import os
import re
import sys

source_root = 'src/Progmasoft/Catch3'
include_parser = re.compile(r'#include <(Progmasoft/Catch3/.+\.hpp)>')
engine_headers = {path[len('src/'):] for path in upstreamFiles() if path.endswith('.hpp')}

errors_found = False


def engine_headers_in(folder):
    """Engine headers directly in folder, as include paths."""
    prefix = folder + '/'
    return sorted(h for h in engine_headers
                  if h.startswith(prefix) and '/' not in h[len(prefix):])


def subfolders_with_engine_headers(folder):
    prefix = folder + '/'
    names = set()
    for header in engine_headers:
        if header.startswith(prefix):
            rest = header[len(prefix):]
            if '/' in rest:
                names.add(rest.split('/')[0])
    return sorted(names)


def collated_includes(folder):
    base = engine_headers_in(folder)
    for name in subfolders_with_engine_headers(folder):
        subfolder = folder + '/' + name
        if name in internal_dirs:
            base.extend(engine_headers_in(subfolder))
        else:
            base.append(subfolder + '/All.hpp')
    return sorted(base)


def includes_from_file(header):
    includes = []
    with open(os.path.join(catchPath, 'src', header), 'r', encoding='utf-8') as file:
        for line in file:
            if not line.startswith('#include'):
                continue
            match = include_parser.match(line)
            if match:
                includes.append(match.group(1))
    return includes


def get_duplicates(xs):
    seen = set()
    duplicated = []
    for x in xs:
        if x in seen:
            duplicated.append(x)
        seen.add(x)
    return duplicated


def verify_convenience_header(folder):
    """
    Checks the convenience header of one folder:
    1) The header exists
    2) Its includes are sorted
    3) It has no duplicated includes
    4) It includes every header it should
    5) It includes nothing else
    """
    global errors_found
    header_name = folder + '/All.hpp'

    # 1) Does it exist?
    if header_name not in engine_headers:
        errors_found = True
        print('Missing convenience header: {}'.format(header_name))
        return
    file_incs = includes_from_file(header_name)
    # 2) Are the includes sorted?
    if sorted(file_incs) != file_incs:
        errors_found = True
        print("'{}': Includes are not in sorted order!".format(header_name))

    # 3) Are there no duplicates?
    for duplicate in get_duplicates(file_incs):
        errors_found = True
        print("'{}': Duplicated include: '{}'".format(header_name, duplicate))

    # The convenience header does not include itself.
    target_includes = [x for x in collated_includes(folder) if x != header_name]
    # 4) Are all required headers present?
    file_incs_set = set(file_incs)
    for include in target_includes:
        if (include not in file_incs_set and
                include != 'Progmasoft/Catch3/Internal/WindowsHProxy.hpp'):
            errors_found = True
            print("'{}': missing include '{}'".format(header_name, include))

    # 5) Are there any superfluous headers?
    desired_set = set(target_includes)
    for include in file_incs:
        if include not in desired_set:
            errors_found = True
            print("'{}': superfluous include '{}'".format(header_name, include))


def walk_source_folders(folder):
    verify_convenience_header(folder)
    for name in subfolders_with_engine_headers(folder):
        if name not in internal_dirs:
            walk_source_folders(folder + '/' + name)


walk_source_folders('Progmasoft/Catch3')

# Propagate error "code" upwards
if not errors_found:
    print('Everything ok')
else:
    sys.exit(1)
