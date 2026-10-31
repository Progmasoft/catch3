#!/usr/bin/env python3
# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

"""Keeps third_party/ equal to the pinned release archives of its libraries.

    updateThirdParty.py --check    fails when a vendored file differs
    updateThirdParty.py --update   writes the files of the pinned archives

The table below is the only record of the versions. Renovate proposes a new
version by changing "version"; whoever takes it sets "sha256" to the digest
of the new archive, after reading the release, and runs --update.
"""

import argparse
import hashlib
import io
import sys
import tarfile
import urllib.request
from pathlib import Path

from scriptCommon import catchPath

LIBRARIES = {
    "yyjson": {
        # renovate: datasource=github-tags depName=ibireme/yyjson
        "version": "0.13.0",
        "url": "https://github.com/ibireme/yyjson/archive/refs/tags/{version}.tar.gz",
        "sha256": "34e0f62a2bc11ab20d601e8ca1cc2b2079503aa45119a19133d89d19b94a0fae",
        "files": {"src/yyjson.c": "yyjson.c", "src/yyjson.h": "yyjson.h"},
    },
    "pugixml": {
        # renovate: datasource=github-tags depName=zeux/pugixml extractVersion=^v(?<version>.+)$
        "version": "1.16",
        "url": "https://github.com/zeux/pugixml/archive/refs/tags/v{version}.tar.gz",
        "sha256": "357bcab8877dc9943f355d3a72daba1b053238ba955f50fa81586afb65090219",
        "files": {
            "src/pugiconfig.hpp": "pugiconfig.hpp",
            "src/pugixml.cpp": "pugixml.cpp",
            "src/pugixml.hpp": "pugixml.hpp",
        },
    },
}


def archive_files(name: str, library: dict) -> dict:
    """The files of the pinned archive, after its digest is checked."""
    url = library["url"].format(version=library["version"])
    with urllib.request.urlopen(url, timeout=60) as response:
        data = response.read()
    digest = hashlib.sha256(data).hexdigest()
    if digest != library["sha256"]:
        sys.exit(f"{name}: {url} has sha256 {digest}, "
                 f"the table pins {library['sha256']}")
    files = {}
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
        for member in archive.getmembers():
            # The archive has one top directory, named after the release.
            inner = member.name.split("/", 1)[-1]
            if member.isfile() and inner in library["files"]:
                extracted = archive.extractfile(member)
                assert extracted is not None
                files[library["files"][inner]] = extracted.read()
    missing = set(library["files"].values()) - set(files)
    if missing:
        sys.exit(f"{name}: the archive has no {', '.join(sorted(missing))}")
    return files


def normalized(data: bytes) -> bytes:
    # Git may check the files out with CRLF on Windows.
    return data.replace(b"\r\n", b"\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true",
                      help="fail when a vendored file differs from its archive")
    mode.add_argument("--update", action="store_true",
                      help="write the files of the pinned archives")
    arguments = parser.parse_args()

    differences = 0
    for name, library in LIBRARIES.items():
        directory = Path(catchPath) / "third_party" / name
        for file_name, data in archive_files(name, library).items():
            target = directory / file_name
            if arguments.update:
                target.write_bytes(data)
                print(f"{name} {library['version']}: wrote {target.name}")
            elif not target.is_file() or normalized(target.read_bytes()) != normalized(data):
                print(f"{name} {library['version']}: {target} differs from the archive")
                differences += 1
    if differences:
        print("Run tools/scripts/updateThirdParty.py --update.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
