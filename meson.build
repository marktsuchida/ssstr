# This file is part of the Ssstr string library.
# Copyright 2022 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

project(
    'ssstr',
    'c',
    'cpp',
    version: '0.1.0-dev',
    license: 'MIT',
    meson_version: '>=0.62.0',
    default_options: [
        'c_std=c99',
        'cpp_std=c++11', # Supports c++03 but va_list functions require 11+
        'warning_level=3',
        'werror=true',
    ],
)

python_prog = find_program('python3', 'python')

public_inc = include_directories('.')

public_headers = files(
    'ss8str.h',
)

version_check_script = files(
    'scripts/checkversion.py',
)

test(
    'version-check',
    python_prog,
    args: [
        version_check_script,
        meson.project_version(),
        public_headers,
    ],
)

install_headers(public_headers)

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    filebase: 'ssstr',
    name: 'ssstr',
    description: 'Ssstr small-string-optimized string library',
)

unity_dep = dependency('unity')

test_sources = files(
    'test_ss8str.c',
)

ssstr_test = executable(
    'test_ssstr',
    test_sources,
    c_args: [
        '-UNDEBUG',
    ],
    dependencies: [
        unity_dep,
    ],
)
test('ssstr-test', ssstr_test)

ssstr_test_extra_asserts = executable(
    'test_ssstr_extra_asserts',
    test_sources,
    c_args: [
        '-UNDEBUG',
        '-DSSSTR_EXTRA_DEBUG',
    ],
    dependencies: [
        unity_dep,
    ],
)
test('ssstr-extra-asserts-test', ssstr_test_extra_asserts)

ssstr_test_no_asserts = executable(
    'test_ssstr_no_asserts',
    test_sources,
    c_args: [
        '-DNDEBUG',
    ],
    dependencies: [
        unity_dep,
    ],
)
test('ssstr-no-asserts-test', ssstr_test_no_asserts)

test_cpp_include_sources = files(
    'test_ss8str_include.cpp',
)

ssstr_cpp_include_test = executable(
    'test_cpp_include',
    test_cpp_include_sources,
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
)
test('ssstr-cpp-include-nonstatic-test', ssstr_cpp_include_nonstatic_test)

google_benchmark_dep = dependency(
    'benchmark',
    required: get_option('benchmark'),
    disabler: true,
    fallback: 'google-benchmark',
    default_options: [
        'werror=false',
    ],
)

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
        google_benchmark_dep,
    ],
)
benchmark(
    'ssstr-bench',
    ssstr_bench,
    timeout: 120,
    verbose: true,
)

subdir('examples')
subdir('man')

man_pages_check_script = files('scripts/checkmanpages.py')

test(
    'man-pages-check',
    python_prog,
    args: [
        man_pages_check_script,
        public_headers,
        example_test_sources,
        '--',
        man3_pages,
        man3_links,
        man7_pages,
        man7_links,
    ],
)

groff_prog = find_program(
    'groff',
    required: false,
    disabler: true,
)
htmlman_script = files('scripts/htmlman.py')

htmlman = custom_target(
    build_always_stale: true, # Since we cannot list output files
    output: 'htmlman',
    input: [
        man3_pages,
        man3_links,
        man7_pages,
        man7_links,
    ],
    command: [
        python_prog,
        htmlman_script,
        '@OUTPUT@',
        groff_prog,
        '@INPUT@',
    ],
)

readme_check_script = files(
    'scripts/checkreadme.py',
)

readme = files(
    'README.md',
)

readme_snippet_test_source = custom_target(
    build_by_default: true,
    output: 'test_readme_snippets.c',
    input: [
        readme,
        man3_pages,
        man3_links,
    ],
    command: [
        python_prog,
        readme_check_script,
        '@OUTPUT@',
        '@INPUT@',
    ],
)

readme_snippet_test = executable(
    'test_readme_snippets',
    readme_snippet_test_source,
    c_args: [
        '-UNDEBUG',
        '-DSSSTR_EXTRA_DEBUG',
        '-D_CRT_SECURE_NO_WARNINGS',
    ],
    dependencies: [
        unity_dep,
    ],
)
test('readme-snippet-test', readme_snippet_test)
