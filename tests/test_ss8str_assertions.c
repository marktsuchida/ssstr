/*
 * This file is part of the Ssstr string library.
 * Copyright 2022-2023 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

// Customize ssstr assertions:
void unwind_stack(char const *msg);
#define SSSTR_ASSERT(condition)                                               \
    do {                                                                      \
        if (!(condition))                                                     \
            unwind_stack("false assertion: " #condition);                     \
    } while (0)

// We cannot test things like ss8_copy(&s, &s) as is, because it is undefined
// behavior (and actually crashes with some compilers). Remove 'restrict' for
// these tests.
#define SSSTR_TESTING_NO_RESTRICT

#include "ss8str.h"

#include <unity.h>

#include <setjmp.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable : 4702) // Unreachable code
#endif

// Tests for precondition assertions.

// Jump buffer for our custom assert implementation.
static jmp_buf jbuf;

#define EXPECTING_ASSERTION_FAILURE if (!setjmp(jbuf))

void unwind_stack(char const *msg) {
    TEST_MESSAGE(msg);
    longjmp(jbuf, 1);
}

void setUp(void) {
    // Avoid any chance of setjmp-related crosstalk between tests.
    memset(&jbuf, 0, sizeof(jbuf));
}
void tearDown(void) {}

// Code after 'SKIP_UNLESS_EXTRA_DEBUG;' is skipped unless extra assertions are
// enabled.
#ifdef SSSTR_EXTRA_DEBUG
#define SKIP_UNLESS_EXTRA_DEBUG ((void)0)
#else
#define SKIP_UNLESS_EXTRA_DEBUG                                               \
    do {                                                                      \
        TEST_PASS_MESSAGE("skipping tests for extra assertions");             \
    } while (0)
#endif

void test_assertion_customization(void) {
    EXPECTING_ASSERTION_FAILURE {
        SSSTR_ASSERT(false);
        TEST_FAIL_MESSAGE("failed assertion returned");
    }
    TEST_MESSAGE("successfully unwound stack");
}

void test_extra_assert_invariants(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s = SS8_STATIC_INITIALIZER;
    s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_shortcap + 1;
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_invariants(&s);
        TEST_FAIL_MESSAGE("failed to detect invalid short length");
    }

    ss8str t = SS8_STATIC_INITIALIZER;
    t.iNtErNaL_S[0] = 'x'; // Where should be \0.
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_invariants(&t);
        TEST_FAIL_MESSAGE("failed to detect empty missing null terminator");
    }

    ss8str u = SS8_STATIC_INITIALIZER;
    ss8_copy_ch_n(&u, '\0', ss8iNtErNaL_shortcap - 1);
    u.iNtErNaL_S[ss8iNtErNaL_shortcap - 1] = 'x'; // Where should be \0.
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_invariants(&u);
        TEST_FAIL_MESSAGE("failed to detect short missing null terminator");
    }

    char vbuf[1] = {'\0'};
    ss8str v = {.iNtErNaL_L = {
                    .bufsiz = ss8iNtErNaL_shortbufsiz,
                    .len = 0,
                    .ptr = vbuf,
                }};
    v.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_longmode;
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_invariants(&v);
        TEST_FAIL_MESSAGE("failed to detect invalid long bufsiz");
    }

    ss8str w = {.iNtErNaL_L = {
                    .bufsiz = ss8iNtErNaL_shortbufsiz + 1,
                    .len = 0,
                    .ptr = NULL,
                }};
    w.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_longmode;
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_invariants(&w);
        TEST_FAIL_MESSAGE("failed to detect null long ptr");
    }

    char xbuf[1] = {'x'};
    ss8str x = {.iNtErNaL_L = {
                    .bufsiz = ss8iNtErNaL_shortbufsiz + 1,
                    .len = 0,
                    .ptr = xbuf,
                }};
    x.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_longmode;
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_invariants(&x);
        TEST_FAIL_MESSAGE("failed to detect long missing null terminator");
    }
}

void test_init(void) {
    SKIP_UNLESS_EXTRA_DEBUG;
    EXPECTING_ASSERTION_FAILURE {
        ss8_init(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_destroy(void) {
    ss8str s = {.iNtErNaL_L = {
                    .bufsiz = ss8iNtErNaL_shortbufsiz + 1,
                    .len = 0,
                    .ptr = NULL,
                }};
    s.iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_longmode;
    EXPECTING_ASSERTION_FAILURE {
        ss8_destroy(&s);
        TEST_FAIL_MESSAGE("failed to detect long str null ptr");
    }

    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_destroy(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_len(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_len(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_is_empty(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_is_empty(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_capacity(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_capacity(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_cstr(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_mutable_cstr(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cstr(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_mutable_cstr_suffix(NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cstr_suffix(NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_at_front_back(void) {
    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_at(&s, 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_set_at(&s, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_front(&s);
        TEST_FAIL_MESSAGE("failed to detect empty str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_set_front(&s, 'x');
        TEST_FAIL_MESSAGE("failed to detect empty str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_back(&s);
        TEST_FAIL_MESSAGE("failed to detect empty str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_set_back(&s, 'x');
        TEST_FAIL_MESSAGE("failed to detect empty str");
    }

    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_at(NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_set_at(NULL, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_front(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_set_front(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_back(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_set_back(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_reserve(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_reserve(NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_set_len(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_set_len(NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_grow_len(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_grow_len(NULL, SIZE_MAX, SIZE_MAX);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_set_len_to_cstrlen(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_set_len_to_cstrlen(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_shrink_to_fit(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_shrink_to_fit(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_clear(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_clear(NULL);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_extra_assert_no_overlap(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s = SS8_STATIC_INITIALIZER;
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_no_overlap(&s, s.iNtErNaL_S, 1);
        TEST_FAIL_MESSAGE("failed to detect overlap at beginning of str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8iNtErNaL_extra_assert_no_overlap(
            &s, s.iNtErNaL_S + ss8iNtErNaL_shortbufsiz - 1, 1);
        TEST_FAIL_MESSAGE("failed to detect overlap at end of str");
    }
}

void test_copy(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    // Must not be empty for self-copy detection to work.
    ss8_init_copy_ch(&s, 'x');

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_bytes(&s, ss8_cstr(&s), ss8_len(&s));
        TEST_FAIL_MESSAGE("failed to detect self-copy");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_cstr(&s, ss8_cstr(&s));
        TEST_FAIL_MESSAGE("failed to detect self-copy");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        // It is UB if dest == src even if empty, but ss8_copy() cannot
        // reliably assert pointer inequality (compiler may assume it due to
        // 'restrict'). So we only test with a non-empty string.
        ss8_copy(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-copy");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_ch_n(NULL, 'x', 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    ss8_destroy(&s);
}

// No tests for init_copy functions because they are simply compounds.

void test_copy_to(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    // Must not be empty for self-copy detection to work.
    ss8_init_copy_ch(&s, 'x');
    char some_buf[5];

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_to_bytes(NULL, some_buf, sizeof(some_buf));
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_to_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null buf");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_to_bytes(&s, ss8_mutable_cstr(&s), ss8_len(&s));
        TEST_FAIL_MESSAGE("failed to detect self-copy");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_to_cstr(NULL, some_buf, sizeof(some_buf));
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_to_cstr(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null buf");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_to_cstr(&s, ss8_mutable_cstr(&s), ss8_len(&s) + 1);
        TEST_FAIL_MESSAGE("failed to detect self-copy");
    }

    ss8_destroy(&s);
}

void test_swap(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    // Must not be empty for self-swap detection to work.
    ss8_init_copy_ch(&s, 'x');

    EXPECTING_ASSERTION_FAILURE {
        ss8_swap(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str1");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_swap(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null str2");
    }
    EXPECTING_ASSERTION_FAILURE {
        // It is UB if str1 == str2 even if empty, but ss8_swap() cannot
        // reliably assert pointer inequality (compiler may assume it due to
        // 'restrict'). So we only test with a non-empty string.
        ss8_swap(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-swap");
    }

    ss8_destroy(&s);
}

void test_move(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    // Must not be empty for self-move detection to work.
    ss8_init_copy_ch(&s, 'x');

    EXPECTING_ASSERTION_FAILURE {
        ss8_move(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_move(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        // It is UB if dest == src even if empty, but ss8_move() cannot
        // reliably assert pointer inequality (compiler may assume it due to
        // 'restrict'). So we only test with a non-empty string.
        ss8_move(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-move");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_move_destroy(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_move_destroy(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        // It is UB if dest == src even if empty, but ss8_move_destroy() cannot
        // reliably assert pointer inequality (compiler may assume it due to
        // 'restrict'). So we only test with a non-empty string.
        ss8_move_destroy(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-move");
    }

    ss8_destroy(&s);
}

void test_init_move(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_init_move(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_init_move(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_init_move(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-move");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_init_move_destroy(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_init_move_destroy(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_init_move_destroy(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-move");
    }

    ss8_destroy(&s);
}

void test_substr(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_substr(&s, &t, 1, 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_substr_inplace(&s, 1, 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }

    ss8_destroy(&t);
    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    ss8_init(&s);
    ss8_init_copy_cstr(&t, "x");

    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_substr(NULL, &s, 0, 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_copy_substr(&s, NULL, 0, 0);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        // It is UB if dest == src even if empty, but ss8_copy_substr() cannot
        // reliably assert pointer inequality (compiler may assume it due to
        // 'restrict'). So we only test with a non-empty string.
        ss8_copy_substr(&t, &t, 0, 0);
        TEST_FAIL_MESSAGE("failed to detect self-copy");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_substr_inplace(NULL, 0, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }

    ss8_destroy(&t);
    ss8_destroy(&s);
}

void test_insert(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert(&s, 1, &t);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_ch_n(&s, 1, 'x', 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_ch(&s, 1, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }

    ss8_destroy(&t);
    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    // Must not be empty in order to detect self-insert.
    ss8_init_copy_ch(&s, 'x');
    ss8_init(&t);

    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_bytes(&s, 0, ss8_cstr(&s), ss8_len(&s));
        TEST_FAIL_MESSAGE("failed to detect self-insert");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_cstr(&s, 0, ss8_cstr(&s));
        TEST_FAIL_MESSAGE("failed to detect self-insert");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_insert(NULL, 0, &t);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_insert(&s, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect self-insert");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_ch_n(NULL, 0, 'x', 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_insert_ch(NULL, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    ss8_destroy(&s);
}

void test_cat(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s, t;
    // Must not be empty in order to detect self-cat.
    ss8_init_copy_ch(&s, 'x');
    ss8_init(&t);

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_bytes(&s, ss8_cstr(&s), ss8_len(&s));
        TEST_FAIL_MESSAGE("failed to detect self-cat");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_cstr(&s, ss8_cstr(&s));
        TEST_FAIL_MESSAGE("failed to detect self-cat");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat(NULL, &t);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat(&s, &s);
        TEST_FAIL_MESSAGE("failed to detect self-cat");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_ch_n(NULL, 'x', 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    ss8_destroy(&t);
    ss8_destroy(&s);
}

void test_erase(void) {
    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_erase(&s, 1, 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }

    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    EXPECTING_ASSERTION_FAILURE {
        ss8_erase(NULL, 0, 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
}

void test_replace(void) {
    ss8str s, t;
    ss8_init(&s);
    ss8_init(&t);

    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_bytes(&s, 1, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_cstr(&s, 1, 0, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace(&s, 1, 0, &t);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_ch_n(&s, 1, 0, 'x', 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_ch(&s, 1, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds pos");
    }

    ss8_destroy(&t);
    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    // Must not be empty in order to detect self-replace.
    ss8_init_copy_ch(&s, 'x');
    ss8_init(&t);

    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_bytes(NULL, 0, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_bytes(&s, 0, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_bytes(&s, 0, 0, ss8_cstr(&s), ss8_len(&s));
        TEST_FAIL_MESSAGE("failed to detect self-replace");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_cstr(NULL, 0, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_cstr(&s, 0, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_cstr(&s, 0, 0, ss8_cstr(&s));
        TEST_FAIL_MESSAGE("failed to detect self-replace");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_replace(NULL, 0, 0, &t);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace(&s, 0, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null src");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_replace(&s, 0, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect self-replace");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_ch_n(NULL, 0, 0, 'x', 0);
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_replace_ch(NULL, 0, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }

    ss8_destroy(&t);
    ss8_destroy(&s);
}

void test_cmp_equals(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null rhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null rhs");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null rhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null rhs");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null rhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null rhs");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cmp_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_equals_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null lhs");
    }

    ss8_destroy(&s);
}

void test_find(void) {
    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find(&s, 1, &s);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_ch(&s, 1, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_not_ch(&s, 1, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind(&s, 1, &s);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_ch(&s, 1, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_not_ch(&s, 1, 'x');
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }

    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find(NULL, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_ch(NULL, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_not_ch(NULL, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind(NULL, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_ch(NULL, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_rfind_not_ch(NULL, 0, 'x');
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }

    ss8_destroy(&s);
}

void test_find_first_last_of(void) {
    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of_bytes(&s, 1, "", 0);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of_cstr(&s, 1, "");
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of(&s, 1, &s);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of(&s, 1, &s);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of(&s, 1, &s);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of(&s, 1, &s);
        TEST_FAIL_MESSAGE("failed to detect out-of-bounds start");
    }

    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of_bytes(NULL, 0, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of_bytes(&s, 0, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of_cstr(NULL, 0, "");
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of_cstr(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of(NULL, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_of(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of(NULL, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_first_not_of(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of(NULL, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_of(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of(NULL, 0, &s);
        TEST_FAIL_MESSAGE("failed to detect null haystack");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_find_last_not_of(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null needle");
    }

    ss8_destroy(&s);
}

void test_starts_ends_contains(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_starts_with_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_ends_with_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_contains_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_contains_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_contains_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_contains_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_contains(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_contains(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null prefix");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_contains_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }

    ss8_destroy(&s);
}

void test_strip(void) {
    SKIP_UNLESS_EXTRA_DEBUG;

    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip_bytes(NULL, "", 0);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip_bytes(&s, NULL, 0);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip_cstr(NULL, "");
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip_cstr(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip(NULL, &s);
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null chars");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_lstrip_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_rstrip_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_strip_ch(NULL, 'x');
        TEST_FAIL_MESSAGE("failed to detect null str");
    }

    ss8_destroy(&s);
}

void test_sprintf(void) {
    // Test the simple wrappers cat_s[n]printf; no direct tests for
    // cat_vs[n]printf.

    ss8str s;
    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_snprintf(&s, INT_MAX, "%%");
        TEST_FAIL_MESSAGE("failed to detect out-of-range maxlen");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_snprintf(&s, INT_MAX, "%%");
        TEST_FAIL_MESSAGE("failed to detect out-of-range maxlen");
    }

    ss8_destroy(&s);

    SKIP_UNLESS_EXTRA_DEBUG;

    ss8_init(&s);

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_sprintf(NULL, "%%");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_sprintf(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null fmt");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_snprintf(NULL, 0, "%%");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_cat_snprintf(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null fmt");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_sprintf(NULL, "%%");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_sprintf(&s, NULL);
        TEST_FAIL_MESSAGE("failed to detect null fmt");
    }

    EXPECTING_ASSERTION_FAILURE {
        ss8_snprintf(NULL, 0, "%%");
        TEST_FAIL_MESSAGE("failed to detect null dest");
    }
    EXPECTING_ASSERTION_FAILURE {
        ss8_snprintf(&s, 0, NULL);
        TEST_FAIL_MESSAGE("failed to detect null fmt");
    }

    ss8_destroy(&s);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_assertion_customization);
    RUN_TEST(test_extra_assert_invariants);
    RUN_TEST(test_init);
    RUN_TEST(test_destroy);
    RUN_TEST(test_len);
    RUN_TEST(test_is_empty);
    RUN_TEST(test_capacity);
    RUN_TEST(test_cstr);
    RUN_TEST(test_at_front_back);
    RUN_TEST(test_reserve);
    RUN_TEST(test_set_len);
    RUN_TEST(test_grow_len);
    RUN_TEST(test_set_len_to_cstrlen);
    RUN_TEST(test_shrink_to_fit);
    RUN_TEST(test_clear);
    RUN_TEST(test_extra_assert_no_overlap);
    RUN_TEST(test_copy);
    RUN_TEST(test_copy_to);
    RUN_TEST(test_swap);
    RUN_TEST(test_move);
    RUN_TEST(test_init_move);
    RUN_TEST(test_substr);
    RUN_TEST(test_insert);
    RUN_TEST(test_cat);
    RUN_TEST(test_erase);
    RUN_TEST(test_replace);
    RUN_TEST(test_cmp_equals);
    RUN_TEST(test_find);
    RUN_TEST(test_find_first_last_of);
    RUN_TEST(test_starts_ends_contains);
    RUN_TEST(test_strip);
    RUN_TEST(test_sprintf);
    return UNITY_END();
}
