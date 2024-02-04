/*
 * This file is part of the Ssstr string library.
 * Copyright 2022-2023 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#include "ss8str.h"

#include <cassert>
#include <cstdio>

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSVC_LANG >= 201103L)
namespace {

ss8str const static_test_var = SS8_STATIC_INITIALIZER;

void check_static_initializer() {
    assert(ss8_len(&static_test_var) == 0);
    assert(ss8iNtErNaL_bufsize(&static_test_var) == ss8iNtErNaL_shortbufsiz);
}

} // namespace
#endif

int main() {
    ss8str s;
    ss8_init_copy_cstr(&s, "Hello, World!");
    printf("Here is a string: %s\n", ss8_cstr(&s));

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSVC_LANG >= 201103L)
    // Test static initializer because it has a different implementation in
    // C++.
    check_static_initializer();
#endif

    return 0;
}
