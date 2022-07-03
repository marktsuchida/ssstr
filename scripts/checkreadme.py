# This file is part of the Ssstr string library.
# Copyright 2022 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

# Check that README.md mentions every function, and extract example snippets
# from it to generate test source.

# Syntax for snippets in README.md:
#
# <!--
# %TEST_SNIPPET [args]
# %SNIPPET_PROLOGUE [Extra C code]
# %SNIPPET_PROLOGUE [Extra C code]
# -->
#
# ```c
# [Example C code]
# [Example C code]
# ```
#
# <!--
# %SNIPPET_EPILOGUE [Extra C code]
# %SNIPPET_EPILOGUE [Extra C code]
# -->
#
# Where 'args' (space-separated) can include the following:
# - COMPILE_ONLY -- do not execute the snippet, only check that it compiles
# - FILE_SCOPE -- do not wrap in a function

import os.path
import re
import sys


def parse_snippet_lines(lines):
    it = enumerate(lines)
    i = -1
    try:
        while True:
            i, line = next(it)
            if line.startswith("%TEST_SNIPPET"):
                words = line.split()
                yield i, "snippet", (words[1:] if len(words) > 1 else [])
            elif line.startswith("%SNIPPET_PROLOGUE"):
                yield i, "prologue", line.split(None, 1)[1]
            elif line.startswith("%SNIPPET_EPILOGUE"):
                yield i, "epilogue", line.split(None, 1)[1]
            elif line.rstrip() == "```c":
                start = i
                lines = []
                while True:
                    i, line = next(it)
                    if line.rstrip() == "```":
                        break
                    lines.append(line)
                yield start, "code", lines
    except StopIteration:
        yield i, "eof", ()


def assemble_snippets(parser):
    i, kind, content = next(parser)
    while True:
        if kind == "snippet":
            start = i
            lines = []
            compile_only = "COMPILE_ONLY" in content
            file_scope = "FILE_SCOPE" in content
            for flag in content:
                assert flag in ("COMPILE_ONLY", "FILE_SCOPE")
            while True:
                i, kind, content = next(parser)
                if kind == "prologue":
                    lines.append(content)
                else:
                    break
            assert kind == "code"
            lines.extend(content)
            while True:
                i, kind, content = next(parser)
                if kind == "epilogue":
                    lines.append(content)
                else:
                    break
            yield start, compile_only, file_scope, lines
            continue
        elif kind == "code":  # Code fence that is not a snippet
            i, kind, content = next(parser)
            continue
        elif kind == "eof":
            return
        else:
            lineno = i + 1
            print(
                f"Expected snippet or end of file; found {kind} at line {lineno}",
                file=sys.stderr,
            )
            assert False


def write_snippet_tests(readme_path, test_source_path):
    with open(readme_path) as infile:
        lines = infile.readlines()

    funcs = []
    tests = []
    for i, compile_only, file_scope, lines in assemble_snippets(
        parse_snippet_lines(lines)
    ):
        lineno = i + 1
        func = f"snippet_at_line_{lineno}"
        if file_scope:
            lines.insert(0, f"// Snippet at line {lineno}\n")
        else:
            lines.insert(0, f"void {func}(void) {{\n")
            lines.append("}\n")
        funcs.append("".join(lines))
        if not compile_only:
            tests.append(func)

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
