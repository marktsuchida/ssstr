/*
 * This file is part of the Ssstr string library.
 * Copyright 2022, Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#define SNIPPET
#include "ss8str.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#undef SNIPPET

#include <unity.h>

void setUp() {}
void tearDown() {}

void test_example_fgets(void) {
    FILE *fp = tmpfile();

#define SNIPPET
    ss8str line;
    ss8_init(&line);

    size_t nread = 0;
    do {
        int growth = (int)ss8_grow_len(&line, SIZE_MAX, INT_MAX - 1);
        if (growth == 0) {
            ss8_clear(&line);
            break;
        }

        if (!fgets(ss8_cstr_suffix(&line, nread), growth + 1, fp)) {
            ss8_clear(&line);
            break;
        }

        ss8_set_len_to_cstrlen(&line);
        nread = ss8_len(&line);
    } while (!ss8_ends_with_ch(&line, '\n'));
#undef SNIPPET

    TEST_ASSERT_TRUE(ss8_is_empty(&line));

#define SNIPPET
    ss8_destroy(&line);
#undef SNIPPET
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_example_fgets);
    return UNITY_END();
}
