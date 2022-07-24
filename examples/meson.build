# This file is part of the Ssstr string library.
# Copyright 2022 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

example_c_args = [
    '-UNDEBUG',
    '-DSSSTR_EXTRA_DEBUG',
    '-D_CRT_SECURE_NO_WARNINGS',
]

example_fgets_test = executable(
    'test_example_fgets',
    'test_example_fgets.c',
    include_directories: [
        public_inc,
    ],
    c_args: example_c_args,
    dependencies: [
        unity_dep,
    ],
)

example_fread_test = executable(
    'test_example_fread',
    'test_example_fread.c',
    include_directories: [
        public_inc,
    ],
    c_args: example_c_args,
    dependencies: [
        unity_dep,
    ],
)

example_strftime_test = executable(
    'test_example_strftime',
    'test_example_strftime.c',
    include_directories: [
        public_inc,
    ],
    c_args: example_c_args,
    dependencies: [
        unity_dep,
    ],
)

test(
    'example-fgets-test',
    example_fgets_test,
    timeout: 10,
)
test(
    'example-fread-test',
    example_fread_test,
    timeout: 10,
)
test('example-strftime-test', example_strftime_test)

example_test_sources = files(
    'test_example_fgets.c',
    'test_example_fread.c',
    'test_example_strftime.c',
)
