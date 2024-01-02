# This file is part of the Ssstr string library.
# Copyright 2022-2023 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

project(
    'ssstr',
    'c',
    'cpp',
    version: '0.1.1-dev',
    license: 'MIT',
    meson_version: '>=0.62.0',
    default_options: [
        'c_std=c99',
        'cpp_std=c++11', # Supports c++03 but va_list functions require 11+
        'warning_level=3',
        'werror=true',
        'c_args=-DUNITY_USE_FLUSH_STDOUT', # Doesn't work as subproject default
    ],
)

cc = meson.get_compiler('c')

if cc.get_id() in ['gcc', 'clang']
    extra_args = ['-Wconversion', '-Wshadow']
else
    extra_args = []
endif

# MinGW GCC 12 reports warnings that appear to be spurious. Disable only for 12
# because these _may_ go away in GCC 13
# (https://github.com/msys2/MINGW-packages/issues/13104).
if host_machine.system() == 'windows' and cc.get_id() == 'gcc'
    if cc.version().startswith('12.')
        extra_args += ['-Wno-array-bounds', '-Wno-stringop-overflow']
    endif
endif

python_for_tests = find_program(
    'python3',
    'python',
    required: get_option('test'),
    disabler: true,
)
python_for_docs = find_program(
    'python3',
    'python',
    required: get_option('docs'),
    disabler: true,
)

public_inc = include_directories('include')

public_headers = files(
    'include/ss8str.h',
)

version_check_script = files(
    'scripts/checkversion.py',
)

test(
    'version-check',
    python_for_tests,
    args: [
        version_check_script,
        meson.project_version(),
        public_headers,
    ],
)

install_headers(public_headers)

# Meson 'dependency' for use as subproject
ssstr_dep = declare_dependency(
    include_directories: public_inc,
)
meson.override_dependency('ssstr', ssstr_dep)

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    filebase: 'ssstr',
    name: 'ssstr',
    description: 'Ssstr small-string-optimized string library',
)

if find_program('cmake', required: false).found()
    cmake = import('cmake')
    cmake.write_basic_package_version_file(
        name: 'Ssstr',
        version: meson.project_version(),
        arch_independent: true,
    )
    cmake.configure_package_config_file(
        name: 'Ssstr',
        input: 'SsstrConfig.cmake.in',
        configuration: configuration_data(),
    )
endif

unity_dep = dependency('unity', required: get_option('test'), disabler: true)

google_benchmark_dep = dependency(
    'benchmark',
    required: get_option('benchmark'),
    disabler: true,
    fallback: 'google-benchmark',
    default_options: [
        'werror=false',
    ],
)

subdir('tests')
subdir('examples')
subdir('man')

man_pages_check_script = files('scripts/checkmanpages.py')

if not get_option('docs').disabled()
    test(
        'man-pages-check',
        python_for_tests,
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
endif

groff_prog = find_program(
    'groff',
    required: get_option('docs'),
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
        python_for_docs,
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
        python_for_tests,
        readme_check_script,
        '@OUTPUT@',
        '@INPUT@',
    ],
)

readme_snippet_test = executable(
    'test_readme_snippets',
    readme_snippet_test_source,
    c_args: [
        extra_args,
        '-UNDEBUG',
        '-DSSSTR_EXTRA_DEBUG',
        '-D_CRT_SECURE_NO_WARNINGS',
    ],
    dependencies: [
        ssstr_dep,
        unity_dep,
    ],
)
test('readme-snippet-test', readme_snippet_test)
