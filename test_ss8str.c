/*
 * This file is part of the Ssstr string library.
 * Copyright 2022 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#define SSSTR_TESTING

// Mock for testing
int size_overflow_count;
#define SSSTR_SIZE_OVERFLOW()                                                 \
    do {                                                                      \
        ++size_overflow_count;                                                \
    } while (0)

#include "ss8str.h"

#include <unity.h>

#ifdef _MSC_VER
#pragma warning(disable : 4127) // Conditional expression is constant
#endif

// Tests for low-level functions (those that access struct ss8str fields
// directly) are generally white-box tests, or at least treat the short-vs-long
// transitions as edge cases. Tests for other functions usually don't need to
// test separately for short and long strings, and are generally black-box
// tests where the short-vs-long transition is not considered an edge case.
// There are some exceptions.

// By convention, we use the bytes:
// '*' (42) to perturb unused memory,
// '+' (43) as a test string filler, and
// '_' (95) to clear a test copy destination.

void perturb_unused_bytes(ss8str *s) {
    char lastbyte = s->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode) {
        size_t len = ss8iNtErNaL_shortcap - lastbyte;
        if (len < ss8iNtErNaL_shortcap)
            memset(s->iNtErNaL_S + len + 1, '*',
                   ss8iNtErNaL_shortcap - len - 1);
    } else {
        size_t len = s->iNtErNaL_L.len;
        memset(&s->iNtErNaL_L.pad, '*', sizeof(s->iNtErNaL_L.pad) - 1);
        memset(s->iNtErNaL_L.ptr + len + 1, '*',
               s->iNtErNaL_L.bufsiz - len - 1);
    }
}

void perturb_buffer(void *p, size_t siz) { memset(p, '*', siz); }

void blank_buffer(char *p, size_t siz) {
    memset(p, '_', siz - 1);
    p[siz - 1] = '\0'; // To view as string during debugging.
}

void make_test_string(char *p, size_t siz) {
    memset(p, '+', siz - 1);
    p[siz - 1] = '\0';
}

// Required by Unity:
void setUp(void) {}
void tearDown(void) {}

void test_size(void) {
    // Failure may not be a bug, but we want to know if/when size changes
    TEST_ASSERT_EQUAL_size_t(4 * sizeof(void *), sizeof(ss8str));
    TEST_ASSERT_EQUAL_size_t(4 * sizeof(size_t), sizeof(ss8str));

    TEST_ASSERT_EQUAL_size_t(sizeof(ss8str), ss8iNtErNaL_shortbufsiz);

    ss8str s;
    TEST_ASSERT(sizeof(s.iNtErNaL_S) == sizeof(ss8str));
    TEST_ASSERT(sizeof(s.iNtErNaL_L) == sizeof(ss8str));
}

static ss8str static_test_var = SS8_STATIC_INITIALIZER;

void test_init(void) {
    ss8str s;
    perturb_buffer(&s, sizeof(s));
    ss8_init(&s);
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz, ss8iNtErNaL_bufsize(&s));

    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&static_test_var));
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz,
                             ss8iNtErNaL_bufsize(&static_test_var));

    ss8_destroy(&s);
}

void test_len(void) {
    // White-box test

    ss8str s;

    char *lastbyte = &s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap;
    s.iNtErNaL_S[0] = '\0';
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap - 1;
    s.iNtErNaL_S[1] = '\0';
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));

    perturb_buffer(&s, sizeof(s));
    *lastbyte = 0;
    s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = '\0';
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz - 1, ss8_len(&s));

    perturb_buffer(&s, sizeof(s));
    *lastbyte = 1;
    s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 2] = '\0';
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz - 2, ss8_len(&s));

    char buf[2] = "";
    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_longmode;
    s.iNtErNaL_L.len = 0;
    s.iNtErNaL_L.ptr = buf;
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    s.iNtErNaL_L.len = 1;
    buf[0] = '*';
    buf[1] = '\0';
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));

#ifndef SSSTR_EXTRA_DEBUG // Can only test with invariant checking disabled
    s.iNtErNaL_L.len = SIZE_MAX;
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_len(&s));
#endif

    // No free (s is not valid).
}

void test_is_empty(void) {
    // White-box test

    ss8str s;

    char *lastbyte = &s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap;
    s.iNtErNaL_S[0] = '\0';
    TEST_ASSERT_TRUE(ss8_is_empty(&s));

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap - 1;
    s.iNtErNaL_S[1] = '\0';
    TEST_ASSERT_FALSE(ss8_is_empty(&s));

    char buf[2] = "";
    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_longmode;
    s.iNtErNaL_L.len = 0;
    s.iNtErNaL_L.ptr = buf;
    TEST_ASSERT_TRUE(ss8_is_empty(&s));

    s.iNtErNaL_L.len = 1;
    buf[0] = '*';
    buf[1] = '\0';
    TEST_ASSERT_FALSE(ss8_is_empty(&s));

    // No free (s is not valid).
}

void test_bufsize(void) {
    // White-box test

    ss8str s;

    char *lastbyte = &s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap;
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz, ss8iNtErNaL_bufsize(&s));
    *lastbyte = ss8iNtErNaL_shortcap - 1;
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz, ss8iNtErNaL_bufsize(&s));
    *lastbyte = 0;
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz, ss8iNtErNaL_bufsize(&s));
    *lastbyte = 1;
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz, ss8iNtErNaL_bufsize(&s));

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_longmode;
    s.iNtErNaL_L.bufsiz = ss8iNtErNaL_shortbufsiz + 1; // min allowed
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz + 1,
                             ss8iNtErNaL_bufsize(&s));
    s.iNtErNaL_L.bufsiz = ss8iNtErNaL_shortbufsiz + 2;
    TEST_ASSERT_EQUAL_size_t(ss8iNtErNaL_shortbufsiz + 2,
                             ss8iNtErNaL_bufsize(&s));
    s.iNtErNaL_L.bufsiz = SIZE_MAX;
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8iNtErNaL_bufsize(&s));

    // No free (s is not valid).
}

void test_cstr(void) {
    // White-box
    ss8str s;

    char *lastbyte = &s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap;
    s.iNtErNaL_S[0] = '\0';
    TEST_ASSERT_EQUAL_PTR(s.iNtErNaL_S, ss8_mutable_cstr(&s));
    TEST_ASSERT_EQUAL_PTR(s.iNtErNaL_S, ss8_cstr(&s));
    TEST_ASSERT_EQUAL_PTR(s.iNtErNaL_S, ss8_mutable_cstr_suffix(&s, 0));
    TEST_ASSERT_EQUAL_PTR(s.iNtErNaL_S, ss8_cstr_suffix(&s, 0));

    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_shortcap - 1;
    s.iNtErNaL_S[1] = '\0';
    TEST_ASSERT_EQUAL_PTR(s.iNtErNaL_S + 1, ss8_mutable_cstr_suffix(&s, 1));
    TEST_ASSERT_EQUAL_PTR(s.iNtErNaL_S + 1, ss8_cstr_suffix(&s, 1));

    char buf[3] = "";
    perturb_buffer(&s, sizeof(s));
    *lastbyte = ss8iNtErNaL_longmode;
    s.iNtErNaL_L.len = 0;
    s.iNtErNaL_L.ptr = buf;
    TEST_ASSERT_EQUAL_PTR(buf, ss8_mutable_cstr(&s));
    TEST_ASSERT_EQUAL_PTR(buf, ss8_cstr(&s));

    s.iNtErNaL_L.len = 1;
    buf[0] = '*';
    buf[1] = '\0';
    TEST_ASSERT_EQUAL_PTR(buf, ss8_mutable_cstr_suffix(&s, 0));
    TEST_ASSERT_EQUAL_PTR(buf, ss8_cstr_suffix(&s, 0));

    s.iNtErNaL_L.len = 2;
    buf[0] = '*';
    buf[1] = '*';
    buf[2] = '\0';
    TEST_ASSERT_EQUAL_PTR(buf + 1, ss8_mutable_cstr_suffix(&s, 1));
    TEST_ASSERT_EQUAL_PTR(buf + 1, ss8_cstr_suffix(&s, 1));

    // No free (s is not valid).
}

void test_at(void) {
    ss8str s;
    ss8_init_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_CHAR('a', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('a', ss8_front(&s));
    TEST_ASSERT_EQUAL_CHAR('b', ss8_at(&s, 1));
    TEST_ASSERT_EQUAL_CHAR('c', ss8_at(&s, 2));
    TEST_ASSERT_EQUAL_CHAR('c', ss8_back(&s));
    TEST_ASSERT_EQUAL_PTR(&s, ss8_set_at(&s, 0, 'A'));
    TEST_ASSERT_EQUAL_PTR(&s, ss8_set_at(&s, 1, 'B'));
    TEST_ASSERT_EQUAL_PTR(&s, ss8_set_at(&s, 2, 'C'));
    TEST_ASSERT_EQUAL_CHAR('A', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 1));
    TEST_ASSERT_EQUAL_CHAR('C', ss8_at(&s, 2));
    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_set_front(&s, 'A'));
    TEST_ASSERT_EQUAL_PTR(&s, ss8_set_at(&s, 1, 'B'));
    TEST_ASSERT_EQUAL_PTR(&s, ss8_set_back(&s, 'C'));
    TEST_ASSERT_EQUAL_CHAR('A', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 1));
    TEST_ASSERT_EQUAL_CHAR('C', ss8_at(&s, 2));
    ss8_destroy(&s);
}

void test_reserve_short_to_short(void) {
    size_t const shortbufsiz = ss8iNtErNaL_shortbufsiz;

    ss8str s;
    ss8_init(&s);

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    perturb_unused_bytes(&s);
    ss8_reserve(&s, 0);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    perturb_unused_bytes(&s);
    ss8_reserve(&s, 1);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    perturb_unused_bytes(&s);
    ss8_reserve(&s, shortbufsiz - 1);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    ss8_destroy(&s);
    ss8_init(&s);

    // Content must not change. Trust ss8_set_ch_n().

    ss8_copy_ch_n(&s, '+', 1);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(ss8_reserve(&s, 2)));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));

    ss8_copy_ch_n(&s, '+', 2);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_STRING("++", ss8_cstr(ss8_reserve(&s, 3)));
    TEST_ASSERT_EQUAL_size_t(2, ss8_len(&s));

    ss8_copy_ch_n(&s, '+', shortbufsiz - 2);
    perturb_unused_bytes(&s);
    char buf[ss8iNtErNaL_shortbufsiz - 1];
    make_test_string(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(buf, ss8_cstr(ss8_reserve(&s, shortbufsiz - 1)));
    TEST_ASSERT_EQUAL_size_t(shortbufsiz - 2, ss8_len(&s));

    ss8_destroy(&s);
}

void test_reserve_short_to_long(void) {
    size_t const shortbufsiz = ss8iNtErNaL_shortbufsiz;
    ss8str s;
    ss8_init(&s);

    perturb_unused_bytes(&s);
    ss8_reserve(&s, shortbufsiz);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz + 1, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_NOT_NULL(s.iNtErNaL_L.ptr);
    ss8_destroy(&s);
    ss8_init(&s);

    perturb_unused_bytes(&s);
    ss8_reserve(&s, shortbufsiz + 1);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz + 2, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_NOT_NULL(s.iNtErNaL_L.ptr);
    ss8_destroy(&s);
    ss8_init(&s);

    // Content must not change. Trust ss8_set_ch_n().

    ss8_copy_ch_n(&s, '+', shortbufsiz - 1);
    perturb_unused_bytes(&s);
    char buf[ss8iNtErNaL_shortbufsiz];
    make_test_string(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(buf, ss8_cstr(ss8_reserve(&s, shortbufsiz)));
    TEST_ASSERT_EQUAL_size_t(shortbufsiz - 1, ss8_len(&s));
    ss8_destroy(&s);
}

void test_reserve_long_to_long(void) {
    size_t const shortbufsiz = ss8iNtErNaL_shortbufsiz;
    ss8str s;
    ss8_init(&s);

    ss8_reserve(&s, shortbufsiz);
    perturb_unused_bytes(&s);
    ss8_reserve(&s, shortbufsiz + 1);
    TEST_ASSERT_EQUAL_size_t(shortbufsiz + 2, ss8iNtErNaL_bufsize(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_NOT_NULL(s.iNtErNaL_L.ptr);

    // Content must not change. Trust ss8_set_ch_n().

    ss8_copy_ch_n(&s, '+', shortbufsiz);
    perturb_unused_bytes(&s);
    char buf[ss8iNtErNaL_shortbufsiz + 1];
    make_test_string(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(buf, ss8_cstr(ss8_reserve(&s, shortbufsiz + 1)));
    TEST_ASSERT_EQUAL_size_t(shortbufsiz, ss8_len(&s));
    ss8_destroy(&s);
}

void test_set_len(void) {
    ss8str s;
    ss8_init(&s);
    perturb_unused_bytes(&s);
    ss8_set_len(&s, 100);
    memset(ss8_mutable_cstr(&s), '+', 100);
    perturb_unused_bytes(&s);
    ss8str t;
    ss8_init_copy_ch_n(&t, '+', 100); // Trust.
    TEST_ASSERT_EQUAL_STRING(ss8_cstr(&t), ss8_cstr(&s));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_set_len_to_cstrlen(void) {
    ss8str s;
    ss8_init(&s);

    ss8_copy_bytes(&s, "aaa\0bbb", 7);
    perturb_unused_bytes(&s);
    ss8_set_len_to_cstrlen(&s);
    TEST_ASSERT_EQUAL_size_t(3, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("aaa", ss8_cstr(&s));

    ss8_copy_cstr(&s, "abc");
    perturb_unused_bytes(&s);
    ss8_set_len_to_cstrlen(&s);
    TEST_ASSERT_EQUAL_size_t(3, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("abc", ss8_cstr(&s));

    ss8_destroy(&s);
}

void test_three_halves(void) {
    TEST_ASSERT_EQUAL_size_t(0, ss8iNtErNaL_three_halves(0, 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8iNtErNaL_three_halves(0, 1));
    TEST_ASSERT_EQUAL_size_t(0, ss8iNtErNaL_three_halves(1, 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8iNtErNaL_three_halves(1, 1));
    TEST_ASSERT_EQUAL_size_t(3, ss8iNtErNaL_three_halves(2, 3));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX - 1,
                             ss8iNtErNaL_three_halves(SIZE_MAX, SIZE_MAX - 1));
    size_t third_max = SIZE_MAX / 3;
    TEST_ASSERT_EQUAL_size_t(
        third_max * 3, ss8iNtErNaL_three_halves(third_max * 2, SIZE_MAX));
    TEST_ASSERT_EQUAL_size_t(
        SIZE_MAX, ss8iNtErNaL_three_halves((third_max + 1) * 2, SIZE_MAX));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8iNtErNaL_three_halves(SIZE_MAX, SIZE_MAX));
}

void test_grow_len(void) {
    ss8str s;
    ss8_init(&s);

    size_t cap = ss8_capacity(&s);
    TEST_ASSERT_EQUAL_size_t(cap, ss8_grow_len(&s, SIZE_MAX, SIZE_MAX));
    TEST_ASSERT_EQUAL_size_t(cap, ss8_len(&s));

    size_t len = ss8_len(&s);
    size_t newlen = len / 2 * 3;
    TEST_ASSERT_EQUAL_size_t(newlen - len,
                             ss8_grow_len(&s, SIZE_MAX, SIZE_MAX));
    TEST_ASSERT_EQUAL_size_t(newlen, ss8_len(&s));

    TEST_ASSERT_EQUAL_size_t(0, ss8_grow_len(&s, SIZE_MAX, 0));
    TEST_ASSERT_EQUAL_size_t(1, ss8_grow_len(&s, SIZE_MAX, 1));

    len = ss8_len(&s);
    TEST_ASSERT_EQUAL_size_t(0, ss8_grow_len(&s, len, SIZE_MAX));
    TEST_ASSERT_EQUAL_size_t(1, ss8_grow_len(&s, len + 1, SIZE_MAX));

    ss8_destroy(&s);
}

void test_shrink_to_fit_short_to_short(void) {
    size_t const maxshortlen = ss8iNtErNaL_shortbufsiz - 1;

    ss8str s;
    ss8_init(&s);

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    ss8_copy_ch_n(&s, '+', 1);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    ss8_reserve(&s, maxshortlen);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    ss8_copy_ch_n(&s, '+', maxshortlen);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    char buf[ss8iNtErNaL_shortbufsiz];
    make_test_string(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(buf, ss8_cstr(&s));

    ss8_destroy(&s);
}

void test_shrink_to_fit_long_to_short(void) {
    size_t const maxshortlen = ss8iNtErNaL_shortbufsiz - 1;

    ss8str s;
    ss8_init(&s);

    ss8_reserve(&s, maxshortlen + 1);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    ss8_copy_ch_n(&s, '+', 1);
    ss8_reserve(&s, maxshortlen + 1);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    ss8_copy_ch_n(&s, '+', maxshortlen);
    ss8_reserve(&s, maxshortlen + 1);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_capacity(&s));
    char buf[ss8iNtErNaL_shortbufsiz];
    make_test_string(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(buf, ss8_cstr(&s));

    ss8_destroy(&s);
}

void test_shrink_to_fit_long_to_long(void) {
    size_t const maxshortlen = ss8iNtErNaL_shortbufsiz - 1;

    ss8str s;
    ss8_init(&s);

    ss8_copy_ch_n(&s, '+', maxshortlen + 1);
    ss8_reserve(&s, maxshortlen + 2);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_capacity(&s));
    char buf[ss8iNtErNaL_shortbufsiz + 1];
    make_test_string(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(buf, ss8_cstr(&s));

    // No-shrink case
    size_t captofill = ss8_capacity(&s);
    ss8_copy_ch_n(&s, '+', captofill);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_size_t(captofill, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(captofill, ss8_capacity(&s));
    TEST_ASSERT_EQUAL_PTR(&s, ss8_shrink_to_fit(&s));
    TEST_ASSERT_EQUAL_size_t(captofill, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(captofill, ss8_capacity(&s));

    ss8_destroy(&s);
}

#define TEST_ASSERT_EQUAL_SS8STR(expectedss8str, actual)                      \
    do {                                                                      \
        ss8str const *e = (expectedss8str), *a = (actual);                    \
        size_t elen = ss8_len(e);                                             \
        TEST_ASSERT_EQUAL_size_t(elen, ss8_len(a));                           \
        if (elen > 0) {                                                       \
            TEST_ASSERT_EQUAL_MEMORY(ss8_cstr(e), ss8_cstr(a), elen);         \
        }                                                                     \
    } while (0);

#define TEST_ASSERT_EXACT_SS8STR(expected_cstr, actual)                       \
    do {                                                                      \
        char const *e = (expected_cstr);                                      \
        ss8str const *a = (actual);                                           \
        size_t elen = strlen(e);                                              \
        TEST_ASSERT_EQUAL_size_t(elen, ss8_len(a));                           \
        if (elen > 0) {                                                       \
            TEST_ASSERT_EQUAL_MEMORY(e, ss8_cstr(a), elen);                   \
        }                                                                     \
    } while (0);

void test_clear(void) {
    ss8str s;
    ss8_init(&s);

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_clear(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    ss8_copy_cstr(&s, "Foo");
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_clear(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));

    // Must preserve capacity.
    ss8_copy_ch_n(&s, '+', 127);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_clear(&s));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, ss8_capacity(&s));

    ss8_destroy(&s);
}

void test_copy_bytes(void) {
    // Also exercises ss8_reserve().

    ss8str s;
    ss8_init(&s);

    // Empty to empty; empty to short mode
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, "x", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, "+", 1));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    size_t const maxshortlen = ss8iNtErNaL_shortbufsiz - 1;

    char buf[ss8iNtErNaL_shortbufsiz + 4];
    make_test_string(buf, sizeof(buf));
    char expected[sizeof(buf) + 1];

    // Max short-mode
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_len(&s));
    make_test_string(expected, maxshortlen + 1);
    TEST_ASSERT_EQUAL_STRING(expected, ss8_cstr(&s));

    make_test_string(expected, maxshortlen + 2);

    // Non-empty short mode -> long mode
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen + 1));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(expected, ss8_cstr(&s));

    // Empty long mode -> long mode
    ss8_clear(&s);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen + 1));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(expected, ss8_cstr(&s));

    // Empty short mode -> long mode
    ss8_destroy(&s);
    ss8_init(&s);
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen + 1));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(expected, ss8_cstr(&s));

    make_test_string(expected, maxshortlen + 1);

    // Long mode -> fits in short mode
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen + 1));
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(expected, ss8_cstr(&s));

    // Long mode -> empty
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, buf, maxshortlen + 1));
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_bytes(&s, "", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    ss8_destroy(&s);
}

void test_copy(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    // Empty to empty; empty to short mode
    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    perturb_unused_bytes(&s);
    ss8_copy_ch(&t, '+');
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    size_t const maxshortlen = ss8iNtErNaL_shortbufsiz - 1;

    // Max short-mode
    perturb_unused_bytes(&s);
    ss8_copy_ch_n(&t, '+', maxshortlen);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(ss8_cstr(&t), ss8_cstr(&s));

    // Non-empty short mode -> long mode
    perturb_unused_bytes(&s);
    ss8_copy_ch_n(&t, '+', maxshortlen + 1);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(ss8_cstr(&t), ss8_cstr(&s));

    // Empty long mode -> long mode
    ss8_clear(&s);
    perturb_unused_bytes(&s);
    ss8_copy_ch_n(&t, '+', maxshortlen + 1);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(ss8_cstr(&t), ss8_cstr(&s));

    // Empty short mode -> long mode
    ss8_destroy(&s);
    ss8_init(&s);
    perturb_unused_bytes(&s);
    ss8_copy_ch_n(&t, '+', maxshortlen + 1);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(maxshortlen + 1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(ss8_cstr(&t), ss8_cstr(&s));

    // Long mode -> fits in short mode
    ss8_copy_ch_n(&t, '+', maxshortlen + 1);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    perturb_unused_bytes(&s);
    ss8_copy_ch_n(&t, '+', maxshortlen);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(maxshortlen, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING(ss8_cstr(&t), ss8_cstr(&s));

    // Long mode -> empty
    ss8_copy_ch_n(&t, '+', maxshortlen + 1);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    perturb_unused_bytes(&s);
    ss8_clear(&t);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_copy_ch_n(void) {
    // Trust the short-vs-long mode abstraction.

    ss8str s;
    ss8_init(&s);

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_ch_n(&s, '+', 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_ch_n(&s, '+', 1));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_ch_n(&s, '+', 2));
    TEST_ASSERT_EQUAL_size_t(2, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("++", ss8_cstr(&s));

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_ch_n(&s, '+', 1));
    TEST_ASSERT_EQUAL_size_t(1, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("+", ss8_cstr(&s));

    perturb_unused_bytes(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_ch_n(&s, '+', 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("", ss8_cstr(&s));

    ss8_destroy(&s);
}

void test_init_copy(void) {
    ss8str s, t;

    TEST_ASSERT_EXACT_SS8STR("abc", ss8_init_copy_bytes(&s, "abc", 3));
    ss8_destroy(&s);

    TEST_ASSERT_EXACT_SS8STR("abc", ss8_init_copy_cstr(&s, "abc"));
    ss8_destroy(&s);

    ss8_init_copy_cstr(&t, "abc");
    TEST_ASSERT_EXACT_SS8STR("abc", ss8_init_copy(&s, &t));
    ss8_destroy(&s);
    ss8_destroy(&t);

    TEST_ASSERT_EXACT_SS8STR("a", ss8_init_copy_ch(&s, 'a'));
    ss8_destroy(&s);
    TEST_ASSERT_EXACT_SS8STR("aaa", ss8_init_copy_ch_n(&s, 'a', 3));
    ss8_destroy(&s);
}

void test_copy_to(void) {
    // Trust mode abstraction and ss8_set_ch_n().

    char buf[10];
    ss8str s;
    ss8_init(&s);

    // copy_to_bytes

    ss8_copy_ch_n(&s, '+', 0);
    perturb_unused_bytes(&s);
    char zerolen[1] = {'x'};
    TEST_ASSERT_TRUE(ss8_copy_to_bytes(&s, zerolen, 0));
    TEST_ASSERT_EQUAL_CHAR('x', zerolen[0]);

    ss8_copy_ch_n(&s, '+', 1);
    perturb_unused_bytes(&s);
    TEST_ASSERT_FALSE(ss8_copy_to_bytes(&s, zerolen, 0));
    TEST_ASSERT_EQUAL_CHAR('x', zerolen[0]);

    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_TRUE(ss8_copy_to_bytes(&s, buf, 1));
    TEST_ASSERT_EQUAL_CHAR('+', buf[0]);
    TEST_ASSERT_EQUAL_CHAR('_', buf[1]);

    ss8_copy_ch_n(&s, '+', 2);
    perturb_unused_bytes(&s);
    TEST_ASSERT_FALSE(ss8_copy_to_bytes(&s, zerolen, 0));
    TEST_ASSERT_EQUAL_CHAR('x', zerolen[0]);

    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_FALSE(ss8_copy_to_bytes(&s, buf, 1));
    TEST_ASSERT_EQUAL_CHAR('+', buf[0]);
    TEST_ASSERT_EQUAL_CHAR('_', buf[1]);

    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_TRUE(ss8_copy_to_bytes(&s, buf, 2));
    TEST_ASSERT_EQUAL_CHAR('+', buf[0]);
    TEST_ASSERT_EQUAL_CHAR('+', buf[1]);
    TEST_ASSERT_EQUAL_CHAR('_', buf[2]);

    // copy_to_cstr

    ss8_copy_ch_n(&s, '+', 0);
    perturb_unused_bytes(&s);
    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_TRUE(ss8_copy_to_cstr(&s, buf, 1));
    TEST_ASSERT_EQUAL_STRING("", buf);
    TEST_ASSERT_EQUAL_CHAR('_', buf[1]);

    ss8_copy_ch_n(&s, '+', 1);
    perturb_unused_bytes(&s);
    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_FALSE(ss8_copy_to_cstr(&s, buf, 1));
    TEST_ASSERT_EQUAL_STRING("", buf);
    TEST_ASSERT_EQUAL_CHAR('_', buf[1]);

    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_TRUE(ss8_copy_to_cstr(&s, buf, 2));
    TEST_ASSERT_EQUAL_STRING("+", buf);
    TEST_ASSERT_EQUAL_CHAR('_', buf[2]);

    ss8_copy_ch_n(&s, '+', 2);
    perturb_unused_bytes(&s);
    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_FALSE(ss8_copy_to_cstr(&s, buf, 2));
    TEST_ASSERT_EQUAL_STRING("+", buf);
    TEST_ASSERT_EQUAL_CHAR('_', buf[2]);

    blank_buffer(buf, sizeof(buf));
    TEST_ASSERT_TRUE(ss8_copy_to_cstr(&s, buf, 3));
    TEST_ASSERT_EQUAL_STRING("++", buf);
    TEST_ASSERT_EQUAL_CHAR('_', buf[3]);

    ss8_destroy(&s);
}

void test_swap(void) {
    // Short and short
    ss8str s, t;
    ss8_init_copy_cstr(&s, "Alice");
    ss8_init_copy_cstr(&t, "Bob");
    ss8_swap(&s, &t);
    TEST_ASSERT_EQUAL_size_t(strlen("Bob"), ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("Bob", ss8_cstr(&s));
    TEST_ASSERT_EQUAL_size_t(strlen("Alice"), ss8_len(&t));
    TEST_ASSERT_EQUAL_STRING("Alice", ss8_cstr(&t));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_destroy(&t);
    ss8_init(&t);

    // Short and long
    ss8_copy_cstr(&s, "Alice");
    ss8_copy_ch_n(&t, 'B', 127);
    ss8_swap(&s, &t);
    TEST_ASSERT_EQUAL_size_t(127, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, strlen(ss8_cstr(&s)));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 126));
    TEST_ASSERT_EQUAL_size_t(strlen("Alice"), ss8_len(&t));
    TEST_ASSERT_EQUAL_STRING("Alice", ss8_cstr(&t));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_destroy(&t);
    ss8_init(&t);

    // Long and long
    ss8_copy_ch_n(&s, 'A', 255);
    ss8_copy_ch_n(&t, 'B', 127);
    ss8_swap(&s, &t);
    TEST_ASSERT_EQUAL_size_t(127, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, strlen(ss8_cstr(&s)));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 126));
    TEST_ASSERT_EQUAL_size_t(255, ss8_len(&t));
    TEST_ASSERT_EQUAL_size_t(255, strlen(ss8_cstr(&t)));
    TEST_ASSERT_EQUAL_CHAR('A', ss8_at(&t, 0));
    TEST_ASSERT_EQUAL_CHAR('A', ss8_at(&t, 254));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_move(void) {
    // Short and short
    ss8str s, t;
    ss8_init_copy_cstr(&s, "Alice");
    ss8_init_copy_cstr(&t, "Bob");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move(&s, &t));
    TEST_ASSERT_EQUAL_size_t(strlen("Bob"), ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("Bob", ss8_cstr(&s));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_destroy(&t);
    ss8_init(&t);

    // Short and long
    ss8_copy_cstr(&s, "Alice");
    ss8_copy_ch_n(&t, 'B', 127);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move(&s, &t));
    TEST_ASSERT_EQUAL_size_t(127, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, strlen(ss8_cstr(&s)));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 126));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_destroy(&t);
    ss8_init(&t);

    // Long and short
    ss8_copy_ch_n(&s, 'B', 127);
    ss8_copy_cstr(&t, "Alice");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move(&s, &t));
    TEST_ASSERT_EQUAL_size_t(strlen("Alice"), ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("Alice", ss8_cstr(&s));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_destroy(&t);
    ss8_init(&t);

    // Long and long
    ss8_copy_ch_n(&s, 'A', 255);
    ss8_copy_ch_n(&t, 'B', 127);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move(&s, &t));
    TEST_ASSERT_EQUAL_size_t(127, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, strlen(ss8_cstr(&s)));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 126));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_move_destroy(void) {
    // Short and short
    ss8str s, t;
    ss8_init_copy_cstr(&s, "Alice");
    ss8_init_copy_cstr(&t, "Bob");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move_destroy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(strlen("Bob"), ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("Bob", ss8_cstr(&s));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_init(&t);

    // Short and long
    ss8_copy_cstr(&s, "Alice");
    ss8_copy_ch_n(&t, 'B', 127);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move_destroy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(127, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, strlen(ss8_cstr(&s)));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 126));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_init(&t);

    // Long and short
    ss8_copy_ch_n(&s, 'B', 127);
    ss8_copy_cstr(&t, "Alice");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move_destroy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(strlen("Alice"), ss8_len(&s));
    TEST_ASSERT_EQUAL_STRING("Alice", ss8_cstr(&s));
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_init(&t);

    // Long and long
    ss8_copy_ch_n(&s, 'A', 255);
    ss8_copy_ch_n(&t, 'B', 127);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_move_destroy(&s, &t));
    TEST_ASSERT_EQUAL_size_t(127, ss8_len(&s));
    TEST_ASSERT_EQUAL_size_t(127, strlen(ss8_cstr(&s)));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 0));
    TEST_ASSERT_EQUAL_CHAR('B', ss8_at(&s, 126));
    ss8_destroy(&s);
}

void test_init_move(void) {
    ss8str s, t;
    perturb_buffer(&s, sizeof(s));
    ss8_init_copy_cstr(&t, "abc");

    TEST_ASSERT_EQUAL_PTR(&s, ss8_init_move(&s, &t));
    TEST_ASSERT_EXACT_SS8STR("abc", &s);

    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_init_move_destroy(void) {
    ss8str s, t;
    perturb_buffer(&s, sizeof(s));
    ss8_init_copy_cstr(&t, "abc");

    TEST_ASSERT_EQUAL_PTR(&s, ss8_init_move_destroy(&s, &t));
    TEST_ASSERT_EXACT_SS8STR("abc", &s);

    ss8_destroy(&s);
}

void test_copy_substr(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&t, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 0, 1));
    TEST_ASSERT_EXACT_SS8STR("a", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 1, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&t, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 0, 1));
    TEST_ASSERT_EXACT_SS8STR("a", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 0, 2));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 1, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 1, 1));
    TEST_ASSERT_EXACT_SS8STR("b", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 2, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&t, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_copy_substr(&s, &t, 1, 1));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_substr_inplace(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);
    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 0, 1));
    TEST_ASSERT_EXACT_SS8STR("a", &s);
    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 1, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);
    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 0, 1));
    TEST_ASSERT_EXACT_SS8STR("a", &s);
    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 0, 2));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);
    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 1, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);
    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 1, 1));
    TEST_ASSERT_EXACT_SS8STR("b", &s);
    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 2, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_substr_inplace(&s, 1, 1));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_destroy(&s);
}

void test_add_sizes(void) {
    size_t r;

    size_overflow_count = 0;
    r = ss8iNtErNaL_add_sizes(10, 20);
    TEST_ASSERT_EQUAL_size_t(30, r);
    TEST_ASSERT_EQUAL_INT(0, size_overflow_count);

    size_overflow_count = 0;
    r = ss8iNtErNaL_add_sizes(SIZE_MAX - 1, 1);
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, r);
    TEST_ASSERT_EQUAL_INT(0, size_overflow_count);

    size_overflow_count = 0;
    r = ss8iNtErNaL_add_sizes(1, SIZE_MAX - 1);
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, r);
    TEST_ASSERT_EQUAL_INT(0, size_overflow_count);

    size_overflow_count = 0;
    ss8iNtErNaL_add_sizes(SIZE_MAX, 1);
    TEST_ASSERT_EQUAL_INT(1, size_overflow_count);

    size_overflow_count = 0;
    ss8iNtErNaL_add_sizes(1, SIZE_MAX);
    TEST_ASSERT_EQUAL_INT(1, size_overflow_count);

    size_overflow_count = 0;
    ss8iNtErNaL_add_sizes(SIZE_MAX, SIZE_MAX);
    TEST_ASSERT_EQUAL_INT(1, size_overflow_count);
}

void test_growcap(void) {
    size_t const mincap = ss8iNtErNaL_shortbufsiz - 1;

    // 1.5x growth case
    size_t newcap = mincap / 2 * 3;
    TEST_ASSERT_EQUAL_size_t(newcap, ss8iNtErNaL_growcap(mincap, mincap + 1));

    // >1.5x growth case
    TEST_ASSERT_EQUAL_size_t(mincap * 2,
                             ss8iNtErNaL_growcap(mincap, mincap * 2));

    // Should not overflow; max allowed retval is SIZE_MAX - 1
    size_t const maxcap = SIZE_MAX - 1;
    TEST_ASSERT_EQUAL_size_t(
        maxcap / 3 * 3,
        ss8iNtErNaL_growcap(maxcap / 3 * 2 + 1, maxcap / 3 * 2 + 2));
    TEST_ASSERT_EQUAL_size_t(
        maxcap, ss8iNtErNaL_growcap(maxcap / 3 * 2 + 2, maxcap / 3 * 2 + 3));
    TEST_ASSERT_EQUAL_size_t(maxcap,
                             ss8iNtErNaL_growcap(maxcap - 2, maxcap - 1));
}

void test_grow(void) {
    size_t const mincap = ss8iNtErNaL_shortbufsiz - 1;

    ss8str s;
    ss8_init(&s);
    size_t newcap = mincap;
    ss8iNtErNaL_grow(&s, newcap);
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(newcap, ss8_capacity(&s));
    newcap = mincap + 1;
    ss8iNtErNaL_grow(&s, newcap);
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(newcap, ss8_capacity(&s));
    ss8_destroy(&s);
}

void test_insert(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&t, "b");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "b");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("ba", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "b");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "aa");
    ss8_copy_cstr(&t, "bb");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("bbaa", &s);

    ss8_copy_cstr(&s, "aa");
    ss8_copy_cstr(&t, "bb");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("abba", &s);

    ss8_copy_cstr(&s, "aa");
    ss8_copy_cstr(&t, "bb");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert(&s, 2, &t));
    TEST_ASSERT_EXACT_SS8STR("aabb", &s);

    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_insert_cstr(void) {
    ss8str s;
    ss8_init_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_cstr(&s, 1, "c"));
    TEST_ASSERT_EXACT_SS8STR("acb", &s);
    ss8_destroy(&s);
}

void test_cat(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "a");
    ss8_init_copy_cstr(&t, "b");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_cat(&s, &t));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_cat_cstr(&s, "b"));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_erase(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 0, 1));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 1, 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 0, 0));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 0, 1));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 0, 2));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 1, 0));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 1, 1));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 2, 0));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 1, 1));
    TEST_ASSERT_EXACT_SS8STR("ac", &s);

    // Erase beyond end of string
    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_erase(&s, 1, 3));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_destroy(&s);
}

void test_replace(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("A", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("Aa", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("A", &s);

    ss8_copy_cstr(&s, "a");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("aA", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 2, &t));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 2, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("Aab", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("Ab", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 0, 2, &t));
    TEST_ASSERT_EXACT_SS8STR("A", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("aAb", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("aA", &s);

    ss8_copy_cstr(&s, "ab");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 2, 0, &t));
    TEST_ASSERT_EXACT_SS8STR("abA", &s);

    ss8_copy_cstr(&s, "abc");
    ss8_copy_cstr(&t, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("ac", &s);

    ss8_copy_cstr(&s, "abc");
    ss8_copy_cstr(&t, "A");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("aAc", &s);

    ss8_copy_cstr(&s, "abc");
    ss8_copy_cstr(&t, "AB");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 1, &t));
    TEST_ASSERT_EXACT_SS8STR("aABc", &s);

    // Replace beyond end of string
    ss8_copy_cstr(&s, "abc");
    ss8_copy_cstr(&t, "AB");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace(&s, 1, 3, &t));
    TEST_ASSERT_EXACT_SS8STR("aAB", &s);

    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_replace_cstr(void) {
    ss8str s;
    ss8_init_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_cstr(&s, 1, 1, "d"));
    TEST_ASSERT_EXACT_SS8STR("adc", &s);
    ss8_destroy(&s);
}

void test_insert_ch_n(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 0, 'b', 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 0, 'b', 1));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 0, 'b', 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 1, 'b', 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 0, 'b', 1));
    TEST_ASSERT_EXACT_SS8STR("ba", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 1, 'b', 1));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "aa");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 0, 'b', 2));
    TEST_ASSERT_EXACT_SS8STR("bbaa", &s);

    ss8_copy_cstr(&s, "aa");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 1, 'b', 2));
    TEST_ASSERT_EXACT_SS8STR("abba", &s);

    ss8_copy_cstr(&s, "aa");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch_n(&s, 2, 'b', 2));
    TEST_ASSERT_EXACT_SS8STR("aabb", &s);

    ss8_destroy(&s);
}

void test_cat_ch_n(void) {
    ss8str s;
    ss8_init_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_cat_ch_n(&s, 'b', 3));
    TEST_ASSERT_EXACT_SS8STR("abbb", &s);
    ss8_destroy(&s);
}

void test_replace_ch_n(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 0, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 0, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("A", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 0, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 1, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 0, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 0, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("Aa", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 1, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("A", &s);

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 0, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("aA", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 0, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 1, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("b", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 2, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 0, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 1, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("a", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 2, 0, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 0, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("Aab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 1, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("Ab", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 0, 2, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("A", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 0, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("aAb", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 1, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("aA", &s);

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 2, 0, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("abA", &s);

    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 1, 'A', 0));
    TEST_ASSERT_EXACT_SS8STR("ac", &s);

    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 1, 'A', 1));
    TEST_ASSERT_EXACT_SS8STR("aAc", &s);

    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 1, 'A', 2));
    TEST_ASSERT_EXACT_SS8STR("aAAc", &s);

    // Replace beyond end of string
    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch_n(&s, 1, 3, 'A', 2));
    TEST_ASSERT_EXACT_SS8STR("aAA", &s);

    ss8_destroy(&s);
}

void test_ch(void) {
    ss8str s;
    ss8_init(&s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_insert_ch(&s, 0, 'a'));
    TEST_ASSERT_EXACT_SS8STR("a", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_cat_ch(&s, 'b'));
    TEST_ASSERT_EXACT_SS8STR("ab", &s);
    TEST_ASSERT_EQUAL_PTR(&s, ss8_replace_ch(&s, 0, 1, 'c'));
    TEST_ASSERT_EXACT_SS8STR("cb", &s);
    ss8_destroy(&s);
}

void test_cmp_bytes(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "x", 0));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\0", 1));

    ss8_copy_bytes(&s, "\0", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "x", 0));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\0", 1));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\0\0", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\1", 1));

    ss8_copy_bytes(&s, "\0\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\0", 1));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\0\0", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\0\0\0", 3));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\1", 1));

    ss8_copy_bytes(&s, "\0\1", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\0", 1));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\0\1", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\0\1\0", 3));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\1", 1));

    ss8_copy_bytes(&s, "\1", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\0", 1));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\1", 1));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\1\0", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\2", 1));

    ss8_copy_bytes(&s, "\1\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\1", 1));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\1\0", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\1\1", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\2", 1));

    ss8_copy_bytes(&s, "\1\1", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\1", 1));
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\1\0", 2));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\1\1", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\1\2", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\2", 1));

    ss8_copy_bytes(&s, "\2", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\1\xff", 1));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\2", 1));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\2\0", 2));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\3", 1));

    ss8_copy_bytes(&s, "\xff", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_bytes(&s, "\xfe\xfe", 1));
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_bytes(&s, "\xff", 1));
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_bytes(&s, "\xff\0", 2));

    ss8_destroy(&s);
}

void test_cmp(void) {
    ss8str s, t;
    ss8_init_copy_bytes(&s, "abc\0def", 7);
    ss8_init_copy_bytes(&t, "abc\0def\0", 8);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp(&s, &t));
    // Actually compares "abc\0def" with "abc" due to '\0':
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_cstr(&s, "abc\0def"));
    ss8_copy_cstr(&s, "abc");
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_cstr(&s, "abc\0def"));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_cmp_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\0'));
    ss8_copy_bytes(&s, "\0", 1);
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_ch(&s, '\0'));
    ss8_copy_bytes(&s, "\0\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\0'));
    ss8_copy_bytes(&s, "\1", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\0'));
    ss8_copy_bytes(&s, "\xff", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\0'));

    ss8_copy_bytes(&s, "", 0);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\1'));
    ss8_copy_bytes(&s, "\0", 1);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\1'));
    ss8_copy_bytes(&s, "\0\0", 2);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\1'));
    ss8_copy_bytes(&s, "\1", 1);
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_ch(&s, '\1'));
    ss8_copy_bytes(&s, "\1\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\1'));
    ss8_copy_bytes(&s, "\xff", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\1'));

    ss8_copy_bytes(&s, "", 0);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\2'));
    ss8_copy_bytes(&s, "\1", 1);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\2'));
    ss8_copy_bytes(&s, "\1\0", 2);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\2'));
    ss8_copy_bytes(&s, "\2", 1);
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_ch(&s, '\2'));
    ss8_copy_bytes(&s, "\2\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\2'));

    ss8_copy_bytes(&s, "", 0);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\xfe'));
    ss8_copy_bytes(&s, "\0", 1);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\xfe'));
    ss8_copy_bytes(&s, "\xfd\0", 2);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\xfe'));
    ss8_copy_bytes(&s, "\xfe", 1);
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_ch(&s, '\xfe'));
    ss8_copy_bytes(&s, "\xfe\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\xfe'));
    ss8_copy_bytes(&s, "\xff", 1);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\xfe'));

    ss8_copy_bytes(&s, "", 0);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\xff'));
    ss8_copy_bytes(&s, "\0", 1);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\xff'));
    ss8_copy_bytes(&s, "\xfe\0", 2);
    TEST_ASSERT_LESS_THAN_INT(0, ss8_cmp_ch(&s, '\xff'));
    ss8_copy_bytes(&s, "\xff", 1);
    TEST_ASSERT_EQUAL_INT(0, ss8_cmp_ch(&s, '\xff'));
    ss8_copy_bytes(&s, "\xff\0", 2);
    TEST_ASSERT_GREATER_THAN_INT(0, ss8_cmp_ch(&s, '\xff'));

    ss8_destroy(&s);
}

void test_equals_bytes(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_TRUE(ss8_equals_bytes(&s, "x", 0));
    TEST_ASSERT_FALSE(ss8_equals_bytes(&s, "\0", 1));

    ss8_copy_ch(&s, '\0');
    TEST_ASSERT_FALSE(ss8_equals_bytes(&s, "x", 0));
    TEST_ASSERT_TRUE(ss8_equals_bytes(&s, "\0", 1));
    TEST_ASSERT_FALSE(ss8_equals_bytes(&s, "\0\0", 2));

    ss8_copy_ch(&s, 'a');
    TEST_ASSERT_FALSE(ss8_equals_bytes(&s, "x", 0));
    TEST_ASSERT_TRUE(ss8_equals_bytes(&s, "a", 1));
    TEST_ASSERT_FALSE(ss8_equals_bytes(&s, "a\0", 2));

    ss8_destroy(&s);
}

void test_equals(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "abc");
    ss8_init_copy_cstr(&t, "abc");
    TEST_ASSERT_TRUE(ss8_equals(&s, &t));
    TEST_ASSERT_TRUE(ss8_equals_cstr(&s, "abc\0def"));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_equals_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_FALSE(ss8_equals_ch(&s, '\0'));
    TEST_ASSERT_FALSE(ss8_equals_ch(&s, 'a'));

    ss8_copy_ch(&s, '\0');
    TEST_ASSERT_TRUE(ss8_equals_ch(&s, '\0'));
    TEST_ASSERT_FALSE(ss8_equals_ch(&s, 'a'));

    ss8_copy_bytes(&s, "\0\0", 2);
    TEST_ASSERT_FALSE(ss8_equals_ch(&s, '\0'));

    ss8_destroy(&s);
}

void test_find_bytes(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(0, ss8_find_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_bytes(&s, 0, "a", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(1, ss8_find_bytes(&s, 1, "x", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_bytes(&s, 0, "a", 1));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_bytes(&s, 1, "a", 1));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_bytes(&s, 0, "b", 1));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_bytes(&s, 1, "b", 1));

    // Case where prefix matches before whole needle
    ss8_copy_cstr(&s, "aaaabc");
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_bytes(&s, 0, "abc", 3));

    ss8_destroy(&s);
}

void test_find(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "abcabc");
    ss8_init_copy_cstr(&t, "ab");
    TEST_ASSERT_EQUAL_size_t(3, ss8_find(&s, 1, &t));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_cstr(&s, 1, "ab"));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_find_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 0, '\0'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 0, 'a'));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_ch(&s, 0, 'a'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 1, 'a'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 0, 'b'));

    ss8_copy_cstr(&s, "abcabc");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_ch(&s, 0, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_ch(&s, 1, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_ch(&s, 2, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_ch(&s, 3, 'a'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 4, 'a'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 5, 'a'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_ch(&s, 6, 'a'));

    ss8_destroy(&s);
}

void test_find_not_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_not_ch(&s, 0, '\0'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_not_ch(&s, 0, 'a'));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_not_ch(&s, 0, 'b'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_not_ch(&s, 1, 'b'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_not_ch(&s, 0, 'a'));

    ss8_copy_cstr(&s, "abcabc");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_not_ch(&s, 0, 'b'));
    TEST_ASSERT_EQUAL_size_t(2, ss8_find_not_ch(&s, 1, 'b'));
    TEST_ASSERT_EQUAL_size_t(2, ss8_find_not_ch(&s, 2, 'b'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_not_ch(&s, 3, 'b'));
    TEST_ASSERT_EQUAL_size_t(5, ss8_find_not_ch(&s, 4, 'b'));
    TEST_ASSERT_EQUAL_size_t(5, ss8_find_not_ch(&s, 5, 'b'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_not_ch(&s, 6, 'b'));

    ss8_destroy(&s);
}

void test_rfind_bytes(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_bytes(&s, 0, "a", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(1, ss8_rfind_bytes(&s, 1, "x", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_bytes(&s, 0, "a", 1));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_bytes(&s, 1, "a", 1));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_bytes(&s, 0, "b", 1));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_bytes(&s, 1, "b", 1));

    // Case where prefix matches before whole needle
    ss8_copy_cstr(&s, "abccabd");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_bytes(&s, 6, "abc", 3));

    ss8_destroy(&s);
}

void test_rfind(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "abcabc");
    ss8_init_copy_cstr(&t, "ab");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind(&s, 2, &t));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_cstr(&s, 2, "ab"));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_rfind_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_ch(&s, 0, '\0'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_ch(&s, 0, 'a'));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_ch(&s, 0, 'a'));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_ch(&s, 1, 'a'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_ch(&s, 1, 'b'));

    ss8_copy_cstr(&s, "abcabc");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_ch(&s, 0, 'a'));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_ch(&s, 1, 'a'));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_ch(&s, 2, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_rfind_ch(&s, 3, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_rfind_ch(&s, 4, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_rfind_ch(&s, 5, 'a'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_rfind_ch(&s, 6, 'a'));

    ss8_destroy(&s);
}

void test_rfind_not_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_not_ch(&s, 0, '\0'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_not_ch(&s, 0, 'a'));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_not_ch(&s, 0, 'b'));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_not_ch(&s, 1, 'b'));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_rfind_not_ch(&s, 1, 'a'));

    ss8_copy_cstr(&s, "abcabc");
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_not_ch(&s, 0, 'b'));
    TEST_ASSERT_EQUAL_size_t(0, ss8_rfind_not_ch(&s, 1, 'b'));
    TEST_ASSERT_EQUAL_size_t(2, ss8_rfind_not_ch(&s, 2, 'b'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_rfind_not_ch(&s, 3, 'b'));
    TEST_ASSERT_EQUAL_size_t(3, ss8_rfind_not_ch(&s, 4, 'b'));
    TEST_ASSERT_EQUAL_size_t(5, ss8_rfind_not_ch(&s, 5, 'b'));
    TEST_ASSERT_EQUAL_size_t(5, ss8_rfind_not_ch(&s, 6, 'b'));

    ss8_destroy(&s);
}

void test_find_first_of(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_first_of_bytes(&s, 0, "", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_first_of_bytes(&s, 0, "A", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_first_of_bytes(&s, 0, "", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_first_of_bytes(&s, 1, "", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_first_of_bytes(&s, 0, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_of_bytes(&s, 1, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_of_bytes(&s, 0, "AB", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_of_bytes(&s, 1, "AB", 2));

    ss8_copy_cstr(&s, "the quick\tbrown\nfox");
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_first_of_cstr(&s, 0, " \t\n\r"));
    TEST_ASSERT_EQUAL_size_t(9, ss8_find_first_of_cstr(&s, 4, " \t\n\r"));
    TEST_ASSERT_EQUAL_size_t(15, ss8_find_first_of_cstr(&s, 10, " \t\n\r"));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_of_cstr(&s, 16, " \t\n\r"));

    ss8str t;
    ss8_init(&t);
    ss8_copy_cstr(&t, " \t\n\r");
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_first_of(&s, 0, &t));
    ss8_destroy(&t);

    ss8_destroy(&s);
}

void test_find_first_not_of(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_bytes(&s, 0, "", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_bytes(&s, 0, "A", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_first_not_of_bytes(&s, 0, "", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_bytes(&s, 1, "", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_bytes(&s, 0, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_bytes(&s, 1, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_first_not_of_bytes(&s, 0, "AB", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_bytes(&s, 1, "AB", 2));

    ss8_copy_cstr(&s, "the quick\tbrown\nfox");
    char const *alphabet = "abcdefghijklmnopqrstuvwxyz";
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_first_not_of_cstr(&s, 0, alphabet));
    TEST_ASSERT_EQUAL_size_t(9, ss8_find_first_not_of_cstr(&s, 4, alphabet));
    TEST_ASSERT_EQUAL_size_t(15, ss8_find_first_not_of_cstr(&s, 10, alphabet));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_first_not_of_cstr(&s, 16, alphabet));

    ss8str t;
    ss8_init(&t);
    ss8_copy_cstr(&t, alphabet);
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_first_not_of(&s, 0, &t));
    ss8_destroy(&t);

    ss8_destroy(&s);
}

void test_find_last_of(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_last_of_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_last_of_bytes(&s, 0, "A", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_last_of_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_last_of_bytes(&s, 1, "x", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_last_of_bytes(&s, 0, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_last_of_bytes(&s, 1, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_last_of_bytes(&s, 0, "AB", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX, ss8_find_last_of_bytes(&s, 1, "AB", 2));

    ss8_copy_cstr(&s, "the quick\tbrown\nfox");
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_last_of_cstr(&s, 2, " \t\n\r"));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_last_of_cstr(&s, 8, " \t\n\r"));
    TEST_ASSERT_EQUAL_size_t(9, ss8_find_last_of_cstr(&s, 14, " \t\n\r"));
    TEST_ASSERT_EQUAL_size_t(15, ss8_find_last_of_cstr(&s, 19, " \t\n\r"));

    ss8str t;
    ss8_init(&t);
    ss8_copy_cstr(&t, " \t\n\r");
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_last_of(&s, 8, &t));
    ss8_destroy(&t);

    ss8_destroy(&s);
}

void test_find_last_not_of(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_last_not_of_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_last_not_of_bytes(&s, 0, "A", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_last_not_of_bytes(&s, 0, "x", 0));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_last_not_of_bytes(&s, 1, "x", 0));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_last_not_of_bytes(&s, 0, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_last_not_of_bytes(&s, 1, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_last_not_of_bytes(&s, 0, "AB", 2));
    TEST_ASSERT_EQUAL_size_t(0, ss8_find_last_not_of_bytes(&s, 1, "AB", 2));

    ss8_copy_cstr(&s, "the quick\tbrown\nfox");
    char const *alphabet = "abcdefghijklmnopqrstuvwxyz";
    TEST_ASSERT_EQUAL_size_t(SIZE_MAX,
                             ss8_find_last_not_of_cstr(&s, 2, alphabet));
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_last_not_of_cstr(&s, 8, alphabet));
    TEST_ASSERT_EQUAL_size_t(9, ss8_find_last_not_of_cstr(&s, 14, alphabet));
    TEST_ASSERT_EQUAL_size_t(15, ss8_find_last_not_of_cstr(&s, 19, alphabet));

    ss8str t;
    ss8_init(&t);
    ss8_copy_cstr(&t, alphabet);
    TEST_ASSERT_EQUAL_size_t(3, ss8_find_last_not_of(&s, 8, &t));
    ss8_destroy(&t);

    ss8_destroy(&s);
}

void test_starts_with_bytes(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_TRUE(ss8_starts_with_bytes(&s, "x", 0));
    TEST_ASSERT_FALSE(ss8_starts_with_bytes(&s, "a", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_TRUE(ss8_starts_with_bytes(&s, "x", 0));
    TEST_ASSERT_TRUE(ss8_starts_with_bytes(&s, "a", 1));
    TEST_ASSERT_FALSE(ss8_starts_with_bytes(&s, "a\0", 2));

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_TRUE(ss8_starts_with_bytes(&s, "x", 0));
    TEST_ASSERT_TRUE(ss8_starts_with_bytes(&s, "a", 1));
    TEST_ASSERT_FALSE(ss8_starts_with_bytes(&s, "a\0", 2));
    TEST_ASSERT_TRUE(ss8_starts_with_bytes(&s, "ab", 2));
    TEST_ASSERT_FALSE(ss8_starts_with_bytes(&s, "ac", 2));
    TEST_ASSERT_FALSE(ss8_starts_with_bytes(&s, "abc", 3));

    ss8_destroy(&s);
}

void test_starts_with(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "abc");
    ss8_init_copy_cstr(&t, "ab");
    TEST_ASSERT_TRUE(ss8_starts_with(&s, &t));
    TEST_ASSERT_TRUE(ss8_starts_with_cstr(&s, "ab"));
    TEST_ASSERT_TRUE(ss8_starts_with_ch(&s, 'a'));
    ss8_copy_ch(&s, 'a');
    TEST_ASSERT_TRUE(ss8_starts_with_ch(&s, 'a'));
    ss8_clear(&s);
    TEST_ASSERT_FALSE(ss8_starts_with_ch(&s, 'a'));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_ends_with_bytes(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_TRUE(ss8_ends_with_bytes(&s, "x", 0));
    TEST_ASSERT_FALSE(ss8_ends_with_bytes(&s, "a", 1));

    ss8_copy_cstr(&s, "a");
    TEST_ASSERT_TRUE(ss8_ends_with_bytes(&s, "x", 0));
    TEST_ASSERT_TRUE(ss8_ends_with_bytes(&s, "a", 1));
    TEST_ASSERT_FALSE(ss8_ends_with_bytes(&s, "a\0", 2));

    ss8_copy_cstr(&s, "ab");
    TEST_ASSERT_TRUE(ss8_ends_with_bytes(&s, "x", 0));
    TEST_ASSERT_TRUE(ss8_ends_with_bytes(&s, "b", 1));
    TEST_ASSERT_FALSE(ss8_ends_with_bytes(&s, "b\0", 2));
    TEST_ASSERT_TRUE(ss8_ends_with_bytes(&s, "ab", 2));
    TEST_ASSERT_FALSE(ss8_ends_with_bytes(&s, "cb", 2));
    TEST_ASSERT_FALSE(ss8_ends_with_bytes(&s, "abc", 3));

    ss8_destroy(&s);
}

void test_ends_with(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "abc");
    ss8_init_copy_cstr(&t, "bc");
    TEST_ASSERT_TRUE(ss8_ends_with(&s, &t));
    TEST_ASSERT_TRUE(ss8_ends_with_cstr(&s, "bc"));
    TEST_ASSERT_TRUE(ss8_ends_with_ch(&s, 'c'));
    ss8_copy_ch(&s, 'c');
    TEST_ASSERT_TRUE(ss8_ends_with_ch(&s, 'c'));
    ss8_clear(&s);
    TEST_ASSERT_FALSE(ss8_ends_with_ch(&s, 'c'));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_contains(void) {
    ss8str s, t;
    ss8_init_copy_cstr(&s, "abc");
    ss8_init_copy_cstr(&t, "b");
    TEST_ASSERT_TRUE(ss8_contains(&s, &t));
    TEST_ASSERT_TRUE(ss8_contains_cstr(&s, "b"));
    TEST_ASSERT_TRUE(ss8_contains_ch(&s, 'b'));
    ss8_destroy(&s);
    ss8_destroy(&t);
}

void test_strip(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EXACT_SS8STR("", ss8_lstrip_cstr(&s, ""));
    TEST_ASSERT_EXACT_SS8STR("", ss8_rstrip_cstr(&s, ""));
    TEST_ASSERT_EXACT_SS8STR("", ss8_strip_cstr(&s, ""));

    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("aabbbccc", ss8_lstrip_cstr(&s, ""));
    TEST_ASSERT_EXACT_SS8STR("aabbbccc", ss8_rstrip_cstr(&s, ""));
    TEST_ASSERT_EXACT_SS8STR("aabbbccc", ss8_strip_cstr(&s, ""));

    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("bbbccc", ss8_lstrip_cstr(&s, "acx"));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("aabbb", ss8_rstrip_cstr(&s, "acx"));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("bbb", ss8_strip_cstr(&s, "acx"));

    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("", ss8_lstrip_cstr(&s, "abc"));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("", ss8_rstrip_cstr(&s, "abc"));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("", ss8_strip_cstr(&s, "abc"));

    ss8str t;
    ss8_init_copy_cstr(&t, "acx");
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("bbbccc", ss8_lstrip(&s, &t));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("aabbb", ss8_rstrip(&s, &t));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("bbb", ss8_strip(&s, &t));
    ss8_destroy(&t);

    ss8_destroy(&s);
}

void test_strip_ch(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EXACT_SS8STR("", ss8_lstrip_ch(&s, 'x'));
    TEST_ASSERT_EXACT_SS8STR("", ss8_rstrip_ch(&s, 'x'));
    TEST_ASSERT_EXACT_SS8STR("", ss8_strip_ch(&s, 'x'));

    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("aabbbccc", ss8_lstrip_ch(&s, 'x'));
    TEST_ASSERT_EXACT_SS8STR("aabbbccc", ss8_rstrip_ch(&s, 'x'));
    TEST_ASSERT_EXACT_SS8STR("aabbbccc", ss8_strip_ch(&s, 'x'));

    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("bbbccc", ss8_lstrip_ch(&s, 'a'));
    ss8_copy_cstr(&s, "aabbbccc");
    TEST_ASSERT_EXACT_SS8STR("aabbb", ss8_rstrip_ch(&s, 'c'));
    ss8_copy_cstr(&s, "aabbbaaa");
    TEST_ASSERT_EXACT_SS8STR("bbb", ss8_strip_ch(&s, 'a'));

    ss8_destroy(&s);
}

void test_cat_sprintf(void) {
    ss8str s;
    ss8_init(&s);

    TEST_ASSERT_EXACT_SS8STR("", ss8_cat_sprintf(&s, ""));
    TEST_ASSERT_EXACT_SS8STR("", ss8_cat_snprintf(&s, 0, ""));

    ss8_copy_cstr(&s, "-->");
    TEST_ASSERT_EXACT_SS8STR("-->", ss8_cat_sprintf(&s, ""));
    ss8_copy_cstr(&s, "-->");
    TEST_ASSERT_EXACT_SS8STR("-->", ss8_cat_snprintf(&s, 0, ""));

    ss8_copy_cstr(&s, "-->");
    TEST_ASSERT_EXACT_SS8STR("--> abc 128",
                             ss8_cat_sprintf(&s, " %s %d", "abc", 128));
    ss8_copy_cstr(&s, "-->");
    TEST_ASSERT_EXACT_SS8STR("--> abc 128",
                             ss8_cat_snprintf(&s, 8, " %s %d", "abc", 128));
    ss8_copy_cstr(&s, "-->");
    TEST_ASSERT_EXACT_SS8STR("--> abc 12",
                             ss8_cat_snprintf(&s, 7, " %s %d", "abc", 128));
    ss8_copy_cstr(&s, "-->");
    TEST_ASSERT_EXACT_SS8STR("-->",
                             ss8_cat_snprintf(&s, 0, " %s %d", "abc", 128));

    char const *ten = "_123456789";

    // snprintf length limit on first try:
    ss8_reserve(&s, 10);
    ss8_clear(&s);
    ss8_snprintf(&s, 11, "%s", ten);
    TEST_ASSERT_EXACT_SS8STR("_123456789", &s);
    ss8_clear(&s);
    ss8_snprintf(&s, 10, "%s", ten);
    TEST_ASSERT_EXACT_SS8STR("_123456789", &s);
    ss8_clear(&s);
    ss8_snprintf(&s, 9, "%s", ten);
    TEST_ASSERT_EXACT_SS8STR("_12345678", &s);

    // Exercise second try (enlarge buffer):
    TEST_ASSERT_LESS_THAN_size_t(50, ss8iNtErNaL_shortbufsiz); // Assumption

    ss8_destroy(&s);
    ss8_init(&s);
    ss8_sprintf(&s, "%s%s%s%s%s", ten, ten, ten, ten, ten);
    TEST_ASSERT_EXACT_SS8STR(
        "_123456789_123456789_123456789_123456789_123456789", &s);
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_copy_cstr(&s, "-->");
    ss8_cat_sprintf(&s, "%s%s%s%s%s", ten, ten, ten, ten, ten);
    TEST_ASSERT_EXACT_SS8STR(
        "-->_123456789_123456789_123456789_123456789_123456789", &s);

    ss8_destroy(&s);
    ss8_init(&s);
    ss8_snprintf(&s, 51, "%s%s%s%s%s", ten, ten, ten, ten, ten);
    TEST_ASSERT_EXACT_SS8STR(
        "_123456789_123456789_123456789_123456789_123456789", &s);
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_snprintf(&s, 50, "%s%s%s%s%s", ten, ten, ten, ten, ten);
    TEST_ASSERT_EXACT_SS8STR(
        "_123456789_123456789_123456789_123456789_123456789", &s);
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_snprintf(&s, 49, "%s%s%s%s%s", ten, ten, ten, ten, ten);
    TEST_ASSERT_EXACT_SS8STR(
        "_123456789_123456789_123456789_123456789_12345678", &s);
    ss8_destroy(&s);
    ss8_init(&s);
    ss8_copy_cstr(&s, "-->");
    ss8_cat_snprintf(&s, 50, "%s%s%s%s%s", ten, ten, ten, ten, ten);
    TEST_ASSERT_EXACT_SS8STR(
        "-->_123456789_123456789_123456789_123456789_123456789", &s);

    ss8_destroy(&s);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_size);
    RUN_TEST(test_init);
    RUN_TEST(test_len);
    RUN_TEST(test_is_empty);
    RUN_TEST(test_bufsize);
    RUN_TEST(test_cstr);
    RUN_TEST(test_at);
    RUN_TEST(test_reserve_short_to_short);
    RUN_TEST(test_reserve_short_to_long);
    RUN_TEST(test_reserve_long_to_long);
    RUN_TEST(test_set_len);
    RUN_TEST(test_set_len_to_cstrlen);
    RUN_TEST(test_three_halves);
    RUN_TEST(test_grow_len);
    RUN_TEST(test_shrink_to_fit_short_to_short);
    RUN_TEST(test_shrink_to_fit_long_to_short);
    RUN_TEST(test_shrink_to_fit_long_to_long);
    RUN_TEST(test_clear);
    RUN_TEST(test_copy_bytes);
    RUN_TEST(test_copy);
    RUN_TEST(test_copy_ch_n);
    RUN_TEST(test_init_copy);
    RUN_TEST(test_copy_to);
    RUN_TEST(test_swap);
    RUN_TEST(test_move);
    RUN_TEST(test_move_destroy);
    RUN_TEST(test_init_move);
    RUN_TEST(test_init_move_destroy);
    RUN_TEST(test_copy_substr);
    RUN_TEST(test_substr_inplace);
    RUN_TEST(test_add_sizes);
    RUN_TEST(test_growcap);
    RUN_TEST(test_grow);
    RUN_TEST(test_insert);
    RUN_TEST(test_insert_cstr);
    RUN_TEST(test_cat);
    RUN_TEST(test_erase);
    RUN_TEST(test_replace);
    RUN_TEST(test_replace_cstr);
    RUN_TEST(test_insert_ch_n);
    RUN_TEST(test_cat_ch_n);
    RUN_TEST(test_replace_ch_n);
    RUN_TEST(test_ch);
    RUN_TEST(test_cmp_bytes);
    RUN_TEST(test_cmp);
    RUN_TEST(test_cmp_ch);
    RUN_TEST(test_equals_bytes);
    RUN_TEST(test_equals);
    RUN_TEST(test_equals_ch);
    RUN_TEST(test_find_bytes);
    RUN_TEST(test_find);
    RUN_TEST(test_find_ch);
    RUN_TEST(test_find_not_ch);
    RUN_TEST(test_rfind_bytes);
    RUN_TEST(test_rfind);
    RUN_TEST(test_rfind_ch);
    RUN_TEST(test_rfind_not_ch);
    RUN_TEST(test_find_first_of);
    RUN_TEST(test_find_first_not_of);
    RUN_TEST(test_find_last_of);
    RUN_TEST(test_find_last_not_of);
    RUN_TEST(test_starts_with_bytes);
    RUN_TEST(test_starts_with);
    RUN_TEST(test_ends_with_bytes);
    RUN_TEST(test_ends_with);
    RUN_TEST(test_contains);
    RUN_TEST(test_strip);
    RUN_TEST(test_strip_ch);
    RUN_TEST(test_cat_sprintf);
    return UNITY_END();
}
