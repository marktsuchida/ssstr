/*
 * This file is part of the Ssstr string library.
 * Copyright 2022 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#include "ss8str.h"

#include "cstdio"

int main() {
    ss8str s;
    ss8_init_copy_cstr(&s, "Hello, World!");
    printf("Here is a string: %s\n", ss8_const_cstr(&s));
    return 0;
}
