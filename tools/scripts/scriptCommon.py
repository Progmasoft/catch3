import os
import sys

catchPath = os.path.dirname(os.path.dirname(os.path.realpath( os.path.dirname(sys.argv[0]))))


def upstreamFiles():
    """The files below src/Progmasoft that come from Catch2, relative to the
    repository with forward slashes; see upstreamFiles.txt."""
    with open(os.path.join(catchPath, 'tools', 'scripts', 'upstreamFiles.txt'), encoding='utf-8') as listing:
        return {line.strip() for line in listing if line.strip() and not line.startswith('#')}
