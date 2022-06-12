# This file is part of the Ssstr string library.
# Copyright 2022, Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

# Check that README.md mentions every function, and extract example snippets
# from it to generate test source.

# Syntax for snippets in README.md:
#
# <!-- TEST_SNIPPET [args] -->
# <!-- SNIPPET_PROLOGUE [Extra C code] -->
# <!-- SNIPPET_PROLOGUE [Extra C code] -->
# ```c
# [Example C code]
# [Example C code]
# ```
# <!-- SNIPPET_EPILOGUE [Extra C code] -->
# <!-- SNIPPET_EPILOGUE [Extra C code] -->
#
# Where args (space-separated) can include the following:
# - COMPILE_ONLY -- do not execute the snippet, only check that it compiles
# - FILE_SCOPE -- do not wrap in a function

import os.path
import re
import sys


def write_snippet_tests(readme_path, test_source_path):
    with open(readme_path) as infile:
        lines = infile.readlines()

    funcs = []
    tests = []
    for i, line in enumerate(lines):
        if line.startswith("<!-- TEST_SNIPPET "):
            lineno = i + 1
            words = line.split()
            assert words[-1] == "-->"
            args = words[2:-1]
            prologue = []
            j = i + 1
            while lines[j].startswith("<!-- SNIPPET_PROLOGUE "):
                prologue.append(" ".join(lines[j].split()[2:-1]) + "\n")
                j += 1
            assert lines[j].rstrip() == "```c"
            snippet = []
            k = j + 1
            while lines[k].rstrip() != "```":
                snippet.append(lines[k])
                k += 1
            epilogue = []
            l = k + 1
            while lines[l].startswith("<!-- SNIPPET_EPILOGUE "):
                epilogue.append(" ".join(lines[l].split()[2:-1]) + "\n")
                l += 1
            snippet = prologue + snippet + epilogue
            func = f"snippet_at_line_{lineno}"
            if "FILE_SCOPE" not in args:
                snippet.insert(0, f"void {func}(void) {{\n")
                snippet.append("}\n")
            else:
                snippet.insert(0, f"// Snippet at line {lineno}")
            if "COMPILE_ONLY" not in args:
                tests.append(func)
            funcs.append("".join(snippet))

    with open(test_source_path, "w") as outfile:
        outfile.write(
            """// Generated file, do not edit
#include "ss8str.h"
#include <unity.h>
#include <time.h>

void setUp(void) {}
void tearDown(void) {}
"""
        )
        for func in funcs:
            outfile.write(f"\n{func}\n")
        outfile.write("\nint main() { UNITY_BEGIN();\n")
        for test in tests:
            outfile.write(f"RUN_TEST({test});\n")
        outfile.write("return UNITY_END(); }\n")


def get_all_functions(manpage_paths):
    funcs = []
    for path in manpage_paths:
        dir, name = os.path.split(path)
        name, section = name.split(".")
        if section != "3":
            continue
        funcs.append(name)
    return funcs


def check_functions_mentioned(functions, readme_path):
    # First find all words that look like function calls.
    with open(readme_path) as infile:
        lines = infile.readlines()
    mentions = set()
    pattern = re.compile(r"(ss8_[_a-z0-9]+)\(")
    for line in lines:
        for match in re.finditer(pattern, line):
            mentions.add(match.group(1))

    unmentioned = []
    for func in functions:
        if func not in mentions:
            unmentioned.append(func)
    unmentioned.sort()
    if unmentioned:
        print(f"Functions not mentioned in {readme_path}:", file=sys.stderr)
        for f in unmentioned:
            print(f"{f}()", file=sys.stderr)
        sys.exit(1)


def check(readme_path, test_source_path, manpage_paths):
    write_snippet_tests(readme_path, test_source_path)
    functions = get_all_functions(manpage_paths)
    check_functions_mentioned(functions, readme_path)


def usage():
    print(
        "Usage: python checkreadme.py test_readme_snippets.c README.md man_pages ...",
        file=sys.stderr,
    )
    sys.exit(1)


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) < 2:
        usage()
    test_source_path = args.pop(0)
    readme_path = args.pop(0)
    manpage_paths = args
    check(readme_path, test_source_path, manpage_paths)
