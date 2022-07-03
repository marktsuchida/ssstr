# This file is part of the Ssstr string library.
# Copyright 2022 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

# Check that the version number in ss8str.h is correct.

import os.path
import sys


def check(version, source_path):
    source_name = os.path.split(source_path)[1]
    version_line = f" * {source_name}, version {version}"
    n = 5
    with open(source_path) as infile:
        lines = infile.readlines()[:n]
    if (version_line + "\n") not in lines:
        print(
            f'{source_name}: cannot find in first {n} lines: "{version_line}"',
            file=sys.stderr,
        )
        sys.exit(1)


def usage():
    print("Usage: python checkversion.py version ss8str.h", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) < 2:
        usage()
    version = args.pop(0)
    for arg in args:
        check(version, arg)
