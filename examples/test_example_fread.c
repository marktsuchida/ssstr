/*
 * This file is part of the Ssstr string library.
 * Copyright 2022-2023 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#define SNIPPET
#include "ss8str.h"
#include <stdio.h>
#undef SNIPPET

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_example_fread(void) {
    FILE *fp = tmpfile();

#define SNIPPET
    ss8str bytes;
    ss8_init(&bytes);

    ss8_set_len(&bytes, 1024);
    size_t nread = fread(ss8_mutable_cstr(&bytes), ss8_len(&bytes), 1, fp);
    ss8_set_len(&bytes, nread);
#undef SNIPPET

    TEST_ASSERT_TRUE(ss8_is_empty(&bytes));

#define SNIPPET
    ss8_destroy(&bytes);
#undef SNIPPET
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_example_fread);
    return UNITY_END();
}
