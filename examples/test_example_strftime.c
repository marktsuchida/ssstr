/*
 * This file is part of the Ssstr string library.
 * Copyright 2022 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#define SNIPPET
#include "ss8str.h"
#include <stdint.h>
#include <time.h>
#undef SNIPPET

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_example_strftime(void) {
#define SNIPPET
    time_t now = time(NULL);
    ss8str timestr;
    ss8_init(&timestr);

    for (;;) {
        ss8_grow_len(&timestr, SIZE_MAX, SIZE_MAX);

        size_t n = strftime(ss8_mutable_cstr(&timestr), ss8_len(&timestr) + 1,
                            "%c", localtime(&now));
        if (n > 0) {
            ss8_set_len(&timestr, n);
            break;
        }
    }

    // ...

    ss8_destroy(&timestr);
#undef SNIPPET
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_example_strftime);
    return UNITY_END();
}
