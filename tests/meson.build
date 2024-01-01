# This file is part of the Ssstr string library.
# Copyright 2022-2023 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

test_sources = files(
    'test_ss8str.c',
)

ssstr_test = executable(
    'test_ssstr',
    test_sources,
    c_args: [
        extra_args,
        '-UNDEBUG',
    ],
    dependencies: [
        ssstr_dep,
        unity_dep,
    ],
)
test('ssstr-test', ssstr_test)

ssstr_test_extra_asserts = executable(
    'test_ssstr_extra_asserts',
    test_sources,
    c_args: [
        extra_args,
        '-UNDEBUG',
        '-DSSSTR_EXTRA_DEBUG',
    ],
    dependencies: [
        ssstr_dep,
        unity_dep,
    ],
)
test('ssstr-extra-asserts-test', ssstr_test_extra_asserts)

ssstr_test_no_asserts = executable(
    'test_ssstr_no_asserts',
    test_sources,
    c_args: [
        extra_args,
        '-DNDEBUG',
    ],
    dependencies: [
        ssstr_dep,
        unity_dep,
    ],
)
test('ssstr-no-asserts-test', ssstr_test_no_asserts)

test_assertions_sources = files(
    'test_ss8str_assertions.c',
)

ssstr_assertion_test = executable(
    'test_assertions',
    test_assertions_sources,
    c_args: [
        extra_args,
    ],
    dependencies: [
        ssstr_dep,
        unity_dep,
    ],
)
test('ssstr-assertion-test', ssstr_assertion_test)

ssstr_assertion_test_extra_asserts = executable(
    'test_assertions_extra_asserts',
    test_assertions_sources,
    c_args: [
        extra_args,
        '-DSSSTR_EXTRA_DEBUG',
    ],
    dependencies: [
        ssstr_dep,
        unity_dep,
    ],
)
test('ssstr-assertion-extra-asserts-test', ssstr_assertion_test_extra_asserts)

test_cpp_include_sources = files(
    'test_ss8str_include.cpp',
)

if not get_option('test').disabled()
    ssstr_cpp_include_test = executable(
        'test_cpp_include',
        test_cpp_include_sources,
        dependencies: [
            ssstr_dep,
        ],
    )
    test('ssstr-cpp-include-test', ssstr_cpp_include_test)

    # Make sure C++ build with C 'extern inline' works
    ssstr_cpp_include_nonstatic_test = executable(
        'test_cpp_include_nonstatic',
        test_cpp_include_sources,
        cpp_args: [
            '-DSSSTR_USE_NONSTATIC_INLINE',
            '-DSSSTR_DEFINE_EXTERN_INLINE',
        ],
        dependencies: [
            ssstr_dep,
        ],
    )
    test('ssstr-cpp-include-nonstatic-test', ssstr_cpp_include_nonstatic_test)
endif

bench_sources = files(
    'bench_ss8str.cpp',
)

ssstr_bench = executable(
    'bench_ssstr',
    [
        bench_sources,
    ],
    override_options: [
        'cpp_std=c++14', # Avoid passing /std:c++11 to clang-cl
        'optimization=3',
    ],
    cpp_args: [
        '-DNDEBUG',
    ],
    c_args: [
        '-DNDEBUG',
    ],
    dependencies: [
        ssstr_dep,
        google_benchmark_dep,
    ],
)
benchmark(
    'ssstr-bench',
    ssstr_bench,
    timeout: 120,
    verbose: true,
)
