/*
 * ss8str.h, version 0.1.0-dev
 *
 * This file is part of the Ssstr string library.
 * Copyright 2022 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef SS8STR_H_INCLUDED
#define SS8STR_H_INCLUDED

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Names containing 'iNtErNaL' are internal to ssstr and should not be used by
// user code.

#ifdef __cplusplus
extern "C" {
#endif

// By default, functions are static inline.
#ifdef SSSTR_USE_NONSTATIC_INLINE
#ifdef SSSTR_DEFINE_EXTERN_INLINE
#define SSSTR_INLINE extern inline
#else
#define SSSTR_INLINE inline
#endif
#define SSSTR_INLINE_DEF inline
#else
#ifdef SSSTR_DEFINE_EXTERN_INLINE
#error SSSTR_DEFINE_EXTERN_INLINE defined but SSSTR_USE_NONSTATIC_INLINE not defined
#endif
#define SSSTR_INLINE static inline
#define SSSTR_INLINE_DEF static inline
#endif

// Try to get 'restrict' in C++ if available.
#if defined(__cplusplus)
#if defined(__GNUC__)
#define SSSTR_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define SSSTR_RESTRICT __restrict
#else
#define SSSTR_RESTRICT
#endif
#elif defined(_MSC_VER) && __STDC_VERSION__ < 201112L
#define SSSTR_RESTRICT __restrict
#else
#define SSSTR_RESTRICT restrict
#endif

// Panic is used for runtime errors other than assertions. Does not return.
// All uses of panic other than those documented as undefined behavior can be
// customized; see SSSTR_OUT_OF_MEMORY() and SSSTR_SIZE_OVERFLOW().
#define SSSTR_PANIC(msgliteral)                                               \
    do {                                                                      \
        fputs("ssstr: " msgliteral "\n", stderr);                             \
        abort();                                                              \
    } while (0)

#define SSSTR_PANIC_ERRNO(msgliteral)                                         \
    do {                                                                      \
        if (errno != 0)                                                       \
            perror("ssstr: " msgliteral);                                     \
        else                                                                  \
            fputs("ssstr: " msgliteral "\n", stderr);                         \
        abort();                                                              \
    } while (0)

// Customizable malloc/realloc/free.
// Ssstr never calls malloc/realloc with size zero, so custom allocators need
// not handle that case. Otherwise they should behave like standard functions.
#if !defined(SSSTR_MALLOC) && !defined(SSSTR_REALLOC) && !defined(SSSTR_FREE)
#define SSSTR_USING_DEFAULT_MALLOC
#define SSSTR_MALLOC(size) malloc(size)
#define SSSTR_REALLOC(ptr, size) realloc((ptr), (size))
#define SSSTR_FREE(ptr) free(ptr)
#elif !defined(SSSTR_MALLOC) || !defined(SSSTR_REALLOC) || !defined(SSSTR_FREE)
#error All 3, or none, of the following macros must be defined: SSSTR_MALLOC, SSSTR_REALLOC, SSSTR_FREE
#endif

#ifdef __cplusplus
#define SSSTR_CHARP_MALLOC(size) (char *)SSSTR_MALLOC(size)
#define SSSTR_CHARP_REALLOC(ptr, size) (char *)SSSTR_REALLOC(ptr, size)
#define SSSTR_CHARP_MEMCHR(s, c, n) (char *)memchr((s), (c), (n))
#else
#define SSSTR_CHARP_MALLOC(size) SSSTR_MALLOC(size)
#define SSSTR_CHARP_REALLOC(ptr, size) SSSTR_REALLOC(ptr, size)
#define SSSTR_CHARP_MEMCHR(s, c, n) memchr((s), (c), (n))
#endif

// Customizable assert macro
#ifndef SSSTR_ASSERT
#define SSSTR_USING_DEFAULT_ASSERT
// Used for precondition violations, not runtime errors. Not safe to longjmp().
#define SSSTR_ASSERT(condition) assert(condition)
#endif

#define SSSTR_ASSERT_MSG(msg, condition) SSSTR_ASSERT(((void)msg, condition))

// Define SSSTR_EXTRA_DEBUG to enable all possible assertions at the cost of
// performance and binary size.
// Extra assertions include pointer null checks, checks for corrupted ss8str
// values (best effort), and checks for violation of restrict pointer
// semantics. Pattern filling of uninitialized or moved-out (sub)strings is
// also enabled.
#ifdef SSSTR_EXTRA_DEBUG
#ifdef NDEBUG
// Avoid confusing behavior in case custom SSSTR_ASSERT() is NDEBUG-agnostic.
#error NDEBUG and SSSTR_EXTRA_DEBUG must not both be defined
#endif
#define SSSTR_EXTRA_ASSERT(condition) SSSTR_ASSERT(condition)
#define SSSTR_EXTRA_ASSERT_MSG(msg, condition) SSSTR_ASSERT_MSG(msg, condition)
#else
#define SSSTR_EXTRA_ASSERT(condition) ((void)0)
#define SSSTR_EXTRA_ASSERT_MSG(msg, condition) ((void)0)
#endif

// Customizable error handling for runtime errors: OOM and size overflow.
// These macros must not return, but may safely call longjmp().
//
// In theory, calling longjmp() from these macros could cause issues (such as
// memory leaks or, in theory, worse) if any of the functions taking variable
// arguments (but not va_list) are called, because the call to va_end() will be
// skipped. In practice, there is no issue with mainstream ABIs where
// va_start() does not allocate dynamic memory and va_end() is a no-op. To be
// strictly correct, you should use ss8[_cat]_vs[n]printf() rather than
// ss8[_cat]_s[n]printf() and call setjmp() after va_start().

#ifndef SSSTR_OUT_OF_MEMORY
#define SSSTR_USING_DEFAULT_OUT_OF_MEMORY
// Allocation failed (either out of memory or unreasonably large result size).
#define SSSTR_OUT_OF_MEMORY(bytes) SSSTR_PANIC("Cannot allocate memory")
#endif

#ifndef SSSTR_SIZE_OVERFLOW
#define SSSTR_USING_DEFAULT_SIZE_OVERFLOW
// Size of result string would exceed SIZE_MAX.
#define SSSTR_SIZE_OVERFLOW() SSSTR_PANIC("Result too large")
#endif

typedef union {
    // 32 bytes on 64-bit platforms and 16 bytes on 32-bit. This could be
    // reduced to 24/12 bytes, but then we would need to deal with endian
    // differences and type punning.

    // Note that long mode is allowed even if len fits in short mode. This is
    // so that capacity can be reserved. Long mode bufsiz is always greater
    // than short mode bufsiz, which is constant.

    struct ss8iNtErNaL_L {
        char *ptr;     // Never NULL in long mode
        size_t len;    // Always < bufsiz; >= 0
        size_t bufsiz; // Always > ss8iNtErNaL_shortbufsiz
        void *pad;     // Never accessed by this name
    } iNtErNaL_L;

    char iNtErNaL_S[sizeof(struct ss8iNtErNaL_L)];
} ss8str;

#if !defined(__cplusplus)
#define SS8_STATIC_INITIALIZER                                                \
    {                                                                         \
        .iNtErNaL_S = {                                                       \
            [0] = '\0',                                                       \
            [sizeof(ss8str) - 1] = sizeof(ss8str) - 1                         \
        }                                                                     \
    }
#endif

// All public functions have their prototypes listed below, and this list is
// parsed by the man page checking script. Internal functions (ss8iNtErNaL_*)
// have their prototypes immediately before their definitions, to ensure that
// they also have correct linkage under SSSTR_USE_NONSTATIC_INLINE.

///// BEGIN_DOCUMENTED_PROTOTYPES

SSSTR_INLINE ss8str *ss8_init(ss8str *str);
SSSTR_INLINE void ss8_destroy(ss8str *str);
SSSTR_INLINE size_t ss8_len(ss8str const *str);
SSSTR_INLINE bool ss8_is_empty(ss8str const *str);
SSSTR_INLINE size_t ss8_capacity(ss8str const *str);
SSSTR_INLINE char *ss8_cstr(ss8str *str);
SSSTR_INLINE char const *ss8_const_cstr(ss8str const *str);
SSSTR_INLINE char *ss8_cstr_suffix(ss8str *str, size_t start);
SSSTR_INLINE char const *ss8_const_cstr_suffix(ss8str const *str,
                                               size_t start);
SSSTR_INLINE char ss8_at(ss8str const *str, size_t pos);
SSSTR_INLINE ss8str *ss8_set_at(ss8str *str, size_t pos, char ch);
SSSTR_INLINE char ss8_front(ss8str const *str);
SSSTR_INLINE ss8str *ss8_set_front(ss8str *str, char ch);
SSSTR_INLINE char ss8_back(ss8str const *str);
SSSTR_INLINE ss8str *ss8_set_back(ss8str *str, char ch);
SSSTR_INLINE ss8str *ss8_reserve(ss8str *str, size_t capacity);
SSSTR_INLINE void ss8_set_len(ss8str *str, size_t newlen);
SSSTR_INLINE size_t ss8_grow_len(ss8str *str, size_t maxlen, size_t maxdelta);
SSSTR_INLINE void ss8_set_len_to_cstrlen(ss8str *str);
SSSTR_INLINE ss8str *ss8_shrink_to_fit(ss8str *str);
SSSTR_INLINE ss8str *ss8_clear(ss8str *str);
SSSTR_INLINE ss8str *ss8_copy_bytes(ss8str *SSSTR_RESTRICT dest,
                                    char const *SSSTR_RESTRICT src,
                                    size_t srclen);
SSSTR_INLINE ss8str *ss8_copy_cstr(ss8str *SSSTR_RESTRICT dest,
                                   char const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_copy(ss8str *SSSTR_RESTRICT dest,
                              ss8str const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_copy_ch_n(ss8str *dest, char ch, size_t count);
SSSTR_INLINE ss8str *ss8_copy_ch(ss8str *dest, char ch);
SSSTR_INLINE ss8str *ss8_init_copy_bytes(ss8str *SSSTR_RESTRICT str,
                                         char const *SSSTR_RESTRICT src,
                                         size_t len);
SSSTR_INLINE ss8str *ss8_init_copy_cstr(ss8str *SSSTR_RESTRICT str,
                                        char const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_init_copy(ss8str *SSSTR_RESTRICT str,
                                   ss8str const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_init_copy_ch_n(ss8str *str, char ch, size_t count);
SSSTR_INLINE ss8str *ss8_init_copy_ch(ss8str *str, char ch);
SSSTR_INLINE bool ss8_copy_to_bytes(ss8str const *SSSTR_RESTRICT str,
                                    char *SSSTR_RESTRICT buf, size_t bufsize);
SSSTR_INLINE bool ss8_copy_to_cstr(ss8str const *SSSTR_RESTRICT str,
                                   char *SSSTR_RESTRICT buf, size_t bufsize);
SSSTR_INLINE void ss8_swap(ss8str *SSSTR_RESTRICT str1,
                           ss8str *SSSTR_RESTRICT str2);
SSSTR_INLINE ss8str *ss8_move(ss8str *SSSTR_RESTRICT dest,
                              ss8str *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_move_destroy(ss8str *SSSTR_RESTRICT dest,
                                      ss8str *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_init_move(ss8str *SSSTR_RESTRICT str,
                                   ss8str *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_init_move_destroy(ss8str *SSSTR_RESTRICT str,
                                           ss8str *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_copy_substr(ss8str *SSSTR_RESTRICT dest,
                                     ss8str const *SSSTR_RESTRICT src,
                                     size_t start, size_t len);
SSSTR_INLINE ss8str *ss8_substr_inplace(ss8str *str, size_t start, size_t len);
SSSTR_INLINE ss8str *ss8_insert_bytes(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                      char const *SSSTR_RESTRICT src,
                                      size_t srclen);
SSSTR_INLINE ss8str *ss8_insert_cstr(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                     char const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_insert(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                ss8str const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_cat_bytes(ss8str *SSSTR_RESTRICT dest,
                                   char const *SSSTR_RESTRICT src,
                                   size_t srclen);
SSSTR_INLINE ss8str *ss8_cat_cstr(ss8str *SSSTR_RESTRICT dest,
                                  char const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_cat(ss8str *SSSTR_RESTRICT dest,
                             ss8str const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_erase(ss8str *str, size_t pos, size_t len);
SSSTR_INLINE ss8str *ss8_replace_bytes(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                       size_t len,
                                       char const *SSSTR_RESTRICT src,
                                       size_t srclen);
SSSTR_INLINE ss8str *ss8_replace_cstr(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                      size_t len,
                                      char const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_replace(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                 size_t len, ss8str const *SSSTR_RESTRICT src);
SSSTR_INLINE ss8str *ss8_insert_ch_n(ss8str *dest, size_t pos, char ch,
                                     size_t count);
SSSTR_INLINE ss8str *ss8_cat_ch_n(ss8str *dest, char ch, size_t count);
SSSTR_INLINE ss8str *ss8_replace_ch_n(ss8str *dest, size_t pos, size_t len,
                                      char ch, size_t count);
SSSTR_INLINE ss8str *ss8_insert_ch(ss8str *dest, size_t pos, char ch);
SSSTR_INLINE ss8str *ss8_cat_ch(ss8str *dest, char ch);
SSSTR_INLINE ss8str *ss8_replace_ch(ss8str *dest, size_t pos, size_t len,
                                    char ch);
SSSTR_INLINE int ss8_cmp_bytes(ss8str const *lhs, char const *rhs,
                               size_t rhslen);
SSSTR_INLINE int ss8_cmp_cstr(ss8str const *lhs, char const *rhs);
SSSTR_INLINE int ss8_cmp(ss8str const *lhs, ss8str const *rhs);
SSSTR_INLINE int ss8_cmp_ch(ss8str const *lhs, char rhs);
SSSTR_INLINE bool ss8_equals_bytes(ss8str const *lhs, char const *rhs,
                                   size_t rhslen);
SSSTR_INLINE bool ss8_equals_cstr(ss8str const *lhs, char const *rhs);
SSSTR_INLINE bool ss8_equals(ss8str const *lhs, ss8str const *rhs);
SSSTR_INLINE bool ss8_equals_ch(ss8str const *lhs, char rhs);
SSSTR_INLINE size_t ss8_find_bytes(ss8str const *haystack, size_t start,
                                   char const *needle, size_t needlelen);
SSSTR_INLINE size_t ss8_find_cstr(ss8str const *haystack, size_t start,
                                  char const *needle);
SSSTR_INLINE size_t ss8_find(ss8str const *haystack, size_t start,
                             ss8str const *needle);
SSSTR_INLINE size_t ss8_find_ch(ss8str const *haystack, size_t start,
                                char needle);
SSSTR_INLINE size_t ss8_find_not_ch(ss8str const *haystack, size_t start,
                                    char needle);
SSSTR_INLINE size_t ss8_rfind_bytes(ss8str const *haystack, size_t start,
                                    char const *needle, size_t needlelen);
SSSTR_INLINE size_t ss8_rfind_cstr(ss8str const *haystack, size_t start,
                                   char const *needle);
SSSTR_INLINE size_t ss8_rfind(ss8str const *haystack, size_t start,
                              ss8str const *needle);
SSSTR_INLINE size_t ss8_rfind_ch(ss8str const *haystack, size_t start,
                                 char needle);
SSSTR_INLINE size_t ss8_rfind_not_ch(ss8str const *haystack, size_t start,
                                     char needle);
SSSTR_INLINE size_t ss8_find_first_of_bytes(ss8str const *haystack,
                                            size_t start, char const *needles,
                                            size_t count);
SSSTR_INLINE size_t ss8_find_first_not_of_bytes(ss8str const *haystack,
                                                size_t start,
                                                char const *needles,
                                                size_t count);
SSSTR_INLINE size_t ss8_find_last_of_bytes(ss8str const *haystack,
                                           size_t start, char const *needles,
                                           size_t count);
SSSTR_INLINE size_t ss8_find_last_not_of_bytes(ss8str const *haystack,
                                               size_t start,
                                               char const *needles,
                                               size_t count);
SSSTR_INLINE size_t ss8_find_first_of_cstr(ss8str const *haystack,
                                           size_t start, char const *needles);
SSSTR_INLINE size_t ss8_find_first_not_of_cstr(ss8str const *haystack,
                                               size_t start,
                                               char const *needles);
SSSTR_INLINE size_t ss8_find_last_of_cstr(ss8str const *haystack, size_t start,
                                          char const *needles);
SSSTR_INLINE size_t ss8_find_last_not_of_cstr(ss8str const *haystack,
                                              size_t start,
                                              char const *needles);
SSSTR_INLINE size_t ss8_find_first_of(ss8str const *haystack, size_t start,
                                      ss8str const *needles);
SSSTR_INLINE size_t ss8_find_first_not_of(ss8str const *haystack, size_t start,
                                          ss8str const *needles);
SSSTR_INLINE size_t ss8_find_last_of(ss8str const *haystack, size_t start,
                                     ss8str const *needles);
SSSTR_INLINE size_t ss8_find_last_not_of(ss8str const *haystack, size_t start,
                                         ss8str const *needles);
SSSTR_INLINE bool ss8_starts_with_bytes(ss8str const *str, char const *prefix,
                                        size_t prefixlen);
SSSTR_INLINE bool ss8_starts_with_cstr(ss8str const *str, char const *prefix);
SSSTR_INLINE bool ss8_starts_with(ss8str const *str, ss8str const *prefix);
SSSTR_INLINE bool ss8_starts_with_ch(ss8str const *str, char ch);
SSSTR_INLINE bool ss8_ends_with_bytes(ss8str const *str, char const *suffix,
                                      size_t suffixlen);
SSSTR_INLINE bool ss8_ends_with_cstr(ss8str const *str, char const *suffix);
SSSTR_INLINE bool ss8_ends_with(ss8str const *str, ss8str const *suffix);
SSSTR_INLINE bool ss8_ends_with_ch(ss8str const *str, char ch);
SSSTR_INLINE bool ss8_contains_bytes(ss8str const *str, char const *infix,
                                     size_t infixlen);
SSSTR_INLINE bool ss8_contains_cstr(ss8str const *str, char const *infix);
SSSTR_INLINE bool ss8_contains(ss8str const *str, ss8str const *infix);
SSSTR_INLINE bool ss8_contains_ch(ss8str const *str, char ch);
SSSTR_INLINE ss8str *ss8_lstrip_bytes(ss8str *SSSTR_RESTRICT str,
                                      char const *SSSTR_RESTRICT chars,
                                      size_t count);
SSSTR_INLINE ss8str *ss8_rstrip_bytes(ss8str *SSSTR_RESTRICT str,
                                      char const *SSSTR_RESTRICT chars,
                                      size_t count);
SSSTR_INLINE ss8str *ss8_strip_bytes(ss8str *SSSTR_RESTRICT str,
                                     char const *SSSTR_RESTRICT chars,
                                     size_t count);
SSSTR_INLINE ss8str *ss8_lstrip_cstr(ss8str *SSSTR_RESTRICT str,
                                     char const *SSSTR_RESTRICT chars);
SSSTR_INLINE ss8str *ss8_rstrip_cstr(ss8str *SSSTR_RESTRICT str,
                                     char const *SSSTR_RESTRICT chars);
SSSTR_INLINE ss8str *ss8_strip_cstr(ss8str *SSSTR_RESTRICT str,
                                    char const *SSSTR_RESTRICT chars);
SSSTR_INLINE ss8str *ss8_lstrip(ss8str *SSSTR_RESTRICT str,
                                ss8str const *SSSTR_RESTRICT chars);
SSSTR_INLINE ss8str *ss8_rstrip(ss8str *SSSTR_RESTRICT str,
                                ss8str const *SSSTR_RESTRICT chars);
SSSTR_INLINE ss8str *ss8_strip(ss8str *SSSTR_RESTRICT str,
                               ss8str const *SSSTR_RESTRICT chars);
SSSTR_INLINE ss8str *ss8_lstrip_ch(ss8str *str, char ch);
SSSTR_INLINE ss8str *ss8_rstrip_ch(ss8str *str, char ch);
SSSTR_INLINE ss8str *ss8_strip_ch(ss8str *str, char ch);
#if !defined(__cplusplus) || __cplusplus >= 201103L // C++ >= 11
SSSTR_INLINE ss8str *ss8_cat_vsprintf(ss8str *SSSTR_RESTRICT dest,
                                      char const *SSSTR_RESTRICT fmt,
                                      va_list args);
SSSTR_INLINE ss8str *ss8_vsprintf(ss8str *SSSTR_RESTRICT dest,
                                  char const *SSSTR_RESTRICT fmt,
                                  va_list args);
SSSTR_INLINE ss8str *ss8_cat_vsnprintf(ss8str *SSSTR_RESTRICT dest,
                                       size_t maxlen,
                                       char const *SSSTR_RESTRICT fmt,
                                       va_list args);
SSSTR_INLINE ss8str *ss8_vsnprintf(ss8str *SSSTR_RESTRICT dest, size_t maxlen,
                                   char const *SSSTR_RESTRICT fmt,
                                   va_list args);
SSSTR_INLINE ss8str *ss8_cat_sprintf(ss8str *SSSTR_RESTRICT dest,
                                     char const *SSSTR_RESTRICT fmt, ...);
SSSTR_INLINE ss8str *ss8_sprintf(ss8str *SSSTR_RESTRICT dest,
                                 char const *SSSTR_RESTRICT fmt, ...);
SSSTR_INLINE ss8str *ss8_cat_snprintf(ss8str *SSSTR_RESTRICT dest,
                                      size_t maxlen,
                                      char const *SSSTR_RESTRICT fmt, ...);
SSSTR_INLINE ss8str *ss8_snprintf(ss8str *SSSTR_RESTRICT dest, size_t maxlen,
                                  char const *SSSTR_RESTRICT fmt, ...);
#endif // C++ >= 11

///// END_DOCUMENTED_PROTOTYPES

enum { ss8iNtErNaL_shortbufsiz = sizeof(ss8str) };
enum { ss8iNtErNaL_shortcap = ss8iNtErNaL_shortbufsiz - 1 };
#define ss8iNtErNaL_longmode ((char)-1)

SSSTR_INLINE void ss8iNtErNaL_extra_assert_invariants(ss8str const *str);
SSSTR_INLINE_DEF void ss8iNtErNaL_extra_assert_invariants(ss8str const *str) {
    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode) {
        SSSTR_EXTRA_ASSERT_MSG("short string invariant",
                               ((unsigned char)lastbyte) <
                                   ss8iNtErNaL_shortbufsiz);
        SSSTR_EXTRA_ASSERT_MSG(
            "short string invariant",
            str->iNtErNaL_S[ss8iNtErNaL_shortcap - lastbyte] == '\0');
    } else {
        SSSTR_EXTRA_ASSERT_MSG("long string invariant",
                               str->iNtErNaL_L.bufsiz >
                                   ss8iNtErNaL_shortbufsiz);
        SSSTR_EXTRA_ASSERT_MSG("long string invariant",
                               str->iNtErNaL_L.ptr != NULL);
        SSSTR_EXTRA_ASSERT_MSG("long string invariant",
                               str->iNtErNaL_L.ptr[str->iNtErNaL_L.len] ==
                                   '\0');
    }
}

SSSTR_INLINE_DEF ss8str *ss8_init(ss8str *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    str->iNtErNaL_S[0] = '\0';
    str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_shortcap;
    return str;
}

// Called when we make str invalid
SSSTR_INLINE void ss8iNtErNaL_deinit(ss8str *str);
SSSTR_INLINE_DEF void ss8iNtErNaL_deinit(ss8str *str) {
#ifndef NDEBUG
    // Set to invalid so that use-after-free is more likely to be caught.
    str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_longmode;
    str->iNtErNaL_L.ptr = NULL;
#else
    (void)str;
#endif
}

SSSTR_INLINE_DEF void ss8_destroy(ss8str *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);

    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte == ss8iNtErNaL_longmode) {
        SSSTR_ASSERT_MSG("must not already be destroyed",
                         str->iNtErNaL_L.ptr != NULL);
        SSSTR_FREE(str->iNtErNaL_L.ptr);
    }
    ss8iNtErNaL_deinit(str);
}

SSSTR_INLINE_DEF size_t ss8_len(ss8str const *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);

    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode)
        return ss8iNtErNaL_shortcap - lastbyte;
    else
        return str->iNtErNaL_L.len;
}

// The newlen must be within current capacity. Caller is responsible for
// placing the null terminator.
SSSTR_INLINE void ss8iNtErNaL_setlen(ss8str *str, size_t newlen);
SSSTR_INLINE_DEF void ss8iNtErNaL_setlen(ss8str *str, size_t newlen) {
    char *lastbyte = &str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (*lastbyte != ss8iNtErNaL_longmode)
        *lastbyte = (char)(ss8iNtErNaL_shortcap - newlen);
    else
        str->iNtErNaL_L.len = newlen;
}

SSSTR_INLINE_DEF bool ss8_is_empty(ss8str const *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);

    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode)
        return lastbyte == ss8iNtErNaL_shortcap;
    else
        return str->iNtErNaL_L.len == 0;
}

SSSTR_INLINE size_t ss8iNtErNaL_bufsize(ss8str const *str);
SSSTR_INLINE_DEF size_t ss8iNtErNaL_bufsize(ss8str const *str) {
    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode)
        return ss8iNtErNaL_shortbufsiz;
    else
        return str->iNtErNaL_L.bufsiz;
}

SSSTR_INLINE_DEF size_t ss8_capacity(ss8str const *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);
    return ss8iNtErNaL_bufsize(str) - 1;
}

SSSTR_INLINE_DEF char *ss8_cstr(ss8str *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);
    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode)
        return str->iNtErNaL_S;
    else
        return str->iNtErNaL_L.ptr;
}

SSSTR_INLINE_DEF char const *ss8_const_cstr(ss8str const *str) {
    return ss8_cstr((ss8str *)str);
}

SSSTR_INLINE_DEF char *ss8_cstr_suffix(ss8str *str, size_t start) {
    SSSTR_ASSERT(start <= ss8_len(str));
    return ss8_cstr(str) + start;
}

SSSTR_INLINE_DEF char const *ss8_const_cstr_suffix(ss8str const *str,
                                                   size_t start) {
    SSSTR_ASSERT(start <= ss8_len(str));
    return ss8_const_cstr(str) + start;
}

SSSTR_INLINE_DEF char ss8_at(ss8str const *str, size_t pos) {
    SSSTR_ASSERT(pos < ss8_len(str));
    return ss8_const_cstr(str)[pos];
}

SSSTR_INLINE_DEF ss8str *ss8_set_at(ss8str *str, size_t pos, char ch) {
    SSSTR_ASSERT(pos < ss8_len(str));
    ss8_cstr(str)[pos] = ch;
    return str;
}

SSSTR_INLINE_DEF char ss8_front(ss8str const *str) {
    SSSTR_ASSERT(ss8_len(str) > 0);
    return ss8_at(str, 0);
}

SSSTR_INLINE_DEF ss8str *ss8_set_front(ss8str *str, char ch) {
    SSSTR_ASSERT(ss8_len(str) > 0);
    return ss8_set_at(str, 0, ch);
}

SSSTR_INLINE_DEF char ss8_back(ss8str const *str) {
    SSSTR_ASSERT(ss8_len(str) > 0);
    return ss8_at(str, ss8_len(str) - 1);
}

SSSTR_INLINE_DEF ss8str *ss8_set_back(ss8str *str, char ch) {
    SSSTR_ASSERT(ss8_len(str) > 0);
    return ss8_set_at(str, ss8_len(str) - 1, ch);
}

// Only call this when an allocation is required. This 'impl' function is made
// separate so that it is more likely that the initial check will be inlined.
// Returns pointer to string buffer.
SSSTR_INLINE char *ss8iNtErNaL_reserve_impl(ss8str *str, size_t cap);
SSSTR_INLINE_DEF char *ss8iNtErNaL_reserve_impl(ss8str *str, size_t cap) {
    char const lastbyte = str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (lastbyte != ss8iNtErNaL_longmode) {
        char *p = SSSTR_CHARP_MALLOC(cap + 1);
        if (!p)
            SSSTR_OUT_OF_MEMORY(cap + 1);
        size_t len = ss8iNtErNaL_shortcap - lastbyte;
        // Use fixed len so that compiler can inline memcpy().
        memcpy(p, str->iNtErNaL_S, ss8iNtErNaL_shortbufsiz);

        str->iNtErNaL_L.ptr = p;
        str->iNtErNaL_L.len = len;
        str->iNtErNaL_L.bufsiz = cap + 1;
        str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] = ss8iNtErNaL_longmode;
    } else {
        char *p = str->iNtErNaL_L.ptr;
        if (str->iNtErNaL_L.len > 0) {
            p = SSSTR_CHARP_REALLOC(p, cap + 1);
            if (!p)
                SSSTR_OUT_OF_MEMORY(cap + 1);
        } else {
            // When we don't need to copy the data, free+malloc is likely
            // faster (https://stackoverflow.com/a/39562813) (TODO: benchmark).
            SSSTR_FREE(p);
            p = SSSTR_CHARP_MALLOC(cap + 1);
            if (!p) {
                str->iNtErNaL_S[0] = '\0'; // longjmp() safety.
                str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] =
                    ss8iNtErNaL_shortcap;
                SSSTR_OUT_OF_MEMORY(cap + 1);
            }
            p[0] = '\0';
        }
        str->iNtErNaL_L.bufsiz = cap + 1;
        str->iNtErNaL_L.ptr = p;
    }
    return str->iNtErNaL_L.ptr;
}

SSSTR_INLINE char *ss8iNtErNaL_reserve(ss8str *str, size_t capacity);
SSSTR_INLINE_DEF char *ss8iNtErNaL_reserve(ss8str *str, size_t capacity) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);
    if (capacity >= ss8iNtErNaL_bufsize(str))
        return ss8iNtErNaL_reserve_impl(str, capacity);
    return ss8_cstr(str);
}

SSSTR_INLINE_DEF ss8str *ss8_reserve(ss8str *str, size_t capacity) {
    ss8iNtErNaL_reserve(str, capacity);
    return str;
}

SSSTR_INLINE_DEF void ss8_set_len(ss8str *str, size_t newlen) {
    char *buf = ss8iNtErNaL_reserve(str, newlen);
    buf[newlen] = '\0';
#ifdef SSSTR_EXTRA_DEBUG
    // Fill any uninitialized portion with a recognizable character.
    size_t oldlen = ss8_len(str);
    if (newlen > oldlen) {
        memset(ss8_cstr(str) + oldlen, '~', newlen - oldlen);
    }
#endif
    ss8iNtErNaL_setlen(str, newlen);
}

// Compute 1.5 * s, clamped to [0, max], without overflow.
SSSTR_INLINE size_t ss8iNtErNaL_three_halves(size_t s, size_t max);
SSSTR_INLINE_DEF size_t ss8iNtErNaL_three_halves(size_t s, size_t max) {
    size_t const half = s / 2;
    return half <= max / 3 ? half * 3 : max;
}

SSSTR_INLINE_DEF size_t ss8_grow_len(ss8str *str, size_t maxlen,
                                     size_t maxdelta) {
    size_t const len = ss8_len(str);
    size_t newlen = ss8iNtErNaL_three_halves(len, SIZE_MAX);

    size_t const cap = ss8_capacity(str);
    if (newlen < cap)
        newlen = cap;

    if (maxlen > SIZE_MAX - 1)
        maxlen = SIZE_MAX - 1;
    if (newlen > maxlen)
        newlen = maxlen;

    if (newlen <= len)
        return 0;

    size_t delta = newlen - len;
    if (delta > maxdelta)
        delta = maxdelta;

    ss8_set_len(str, len + delta);
    return delta;
}

SSSTR_INLINE_DEF void ss8_set_len_to_cstrlen(ss8str *str) {
    size_t len = strlen(ss8_const_cstr(str));
    ss8iNtErNaL_setlen(str, len);
}

SSSTR_INLINE_DEF ss8str *ss8_shrink_to_fit(ss8str *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);

    char *lastbyte = &str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (*lastbyte != ss8iNtErNaL_longmode)
        return str;

    size_t len = str->iNtErNaL_L.len;
    if (len < ss8iNtErNaL_shortbufsiz) {
        char *p = str->iNtErNaL_L.ptr;
        // Use fixed len so that compiler can inline memcpy().
        memcpy(str->iNtErNaL_S, p, ss8iNtErNaL_shortbufsiz);
        SSSTR_FREE(p);
        *lastbyte = (char)(ss8iNtErNaL_shortcap - len);
    } else if (len + 1 < str->iNtErNaL_L.bufsiz) {
        char *p = SSSTR_CHARP_REALLOC(str->iNtErNaL_L.ptr, len + 1);
        if (!p)
            SSSTR_OUT_OF_MEMORY(len + 1);
        str->iNtErNaL_L.ptr = p;
        str->iNtErNaL_L.bufsiz = len + 1;
    }
    return str;
}

SSSTR_INLINE_DEF ss8str *ss8_clear(ss8str *str) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    ss8iNtErNaL_extra_assert_invariants(str);

    char *lastbyte = &str->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1];
    if (*lastbyte != ss8iNtErNaL_longmode) {
        str->iNtErNaL_S[0] = '\0';
        *lastbyte = ss8iNtErNaL_shortcap;
    } else {
        str->iNtErNaL_L.ptr[0] = '\0';
        str->iNtErNaL_L.len = 0;
    }
    return str;
}

SSSTR_INLINE void ss8iNtErNaL_extra_assert_no_overlap(ss8str const *str,
                                                      char const *cstr,
                                                      size_t len);
SSSTR_INLINE_DEF void ss8iNtErNaL_extra_assert_no_overlap(ss8str const *str,
                                                          char const *cstr,
                                                          size_t len) {
    // Be sure to read:
    // https://devblogs.microsoft.com/oldnewthing/20170927-00/?p=97095

    // The following only works if the conversion from char * to uintptr_t uses
    // linear addresses. Whether this is the case is implementation-defined,
    // but it usually is the case on 32/64-bit platforms. On (embedded)
    // 8/16-bit platforms with near and far pointers, this may not necessarily
    // work correctly.

    uintptr_t lbegin = (uintptr_t)ss8_const_cstr(str);
    uintptr_t lend = lbegin + ss8iNtErNaL_bufsize(str);
    SSSTR_EXTRA_ASSERT(lbegin <= lend);

    uintptr_t rbegin = (uintptr_t)cstr;
    uintptr_t rend = rbegin + len;
    SSSTR_EXTRA_ASSERT(rbegin <= rend);

    SSSTR_EXTRA_ASSERT_MSG("char * must not overlap with ss8str buffer",
                           lend <= rbegin || rend <= lbegin);
    (void)lend;
    (void)rend;
}

SSSTR_INLINE_DEF ss8str *ss8_copy_bytes(ss8str *SSSTR_RESTRICT dest,
                                        char const *SSSTR_RESTRICT src,
                                        size_t srclen) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(dest, src, srclen);

    ss8_reserve(ss8_clear(dest), srclen);
    char *p = ss8_cstr(dest);
    memcpy(p, src, srclen);
    p[srclen] = '\0';
    ss8iNtErNaL_setlen(dest, srclen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_copy_cstr(ss8str *SSSTR_RESTRICT dest,
                                       char const *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    return ss8_copy_bytes(dest, src, strlen(src));
}

SSSTR_INLINE_DEF ss8str *ss8_copy(ss8str *SSSTR_RESTRICT dest,
                                  ss8str const *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(dest, ss8_const_cstr(src),
                                        ss8_len(src));

    // According to preliminary benchmarking, both of the following
    // optimizations are effective. However, short-mode-to-short-mode copy is
    // faster when the second optimization is disabled (this could be
    // compiler-dependent and needs more investigation).

#if 1
    // Optimize short mode copy with fixed length memcpy().
    if (dest->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] !=
            ss8iNtErNaL_longmode &&
        src->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] != ss8iNtErNaL_longmode) {
        memcpy(dest, src, sizeof(ss8str));
        return dest;
    }
#endif

    size_t len = ss8_len(src);
    char const *q = ss8_const_cstr(src);
#if 0
    // Optimize short copy even if src and/or dest is in long mode.
    if (len < ss8iNtErNaL_shortbufsiz) {
        char *p = ss8_cstr(dest);
        memcpy(p, q, ss8iNtErNaL_shortbufsiz);
        return dest;
    }
#endif

    return ss8_copy_bytes(dest, q, len);
}

SSSTR_INLINE_DEF ss8str *ss8_copy_ch_n(ss8str *dest, char ch, size_t count) {
    // TODO Short-string case (guaranteed capacity) can be optimized by fixed
    // memset().

    ss8_reserve(ss8_clear(dest), count);
    char *p = ss8_cstr(dest);
    memset(p, ch, count);
    p[count] = '\0';
    ss8iNtErNaL_setlen(dest, count);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_copy_ch(ss8str *dest, char ch) {
    // TODO Compiler can't see the invariant (sufficient buffer size)
    return ss8_copy_ch_n(dest, ch, 1);
}

SSSTR_INLINE_DEF ss8str *ss8_init_copy_bytes(ss8str *SSSTR_RESTRICT str,
                                             char const *SSSTR_RESTRICT src,
                                             size_t len) {
    return ss8_copy_bytes(ss8_init(str), src, len);
}

SSSTR_INLINE_DEF ss8str *ss8_init_copy_cstr(ss8str *SSSTR_RESTRICT str,
                                            char const *SSSTR_RESTRICT src) {
    return ss8_copy_cstr(ss8_init(str), src);
}

SSSTR_INLINE_DEF ss8str *ss8_init_copy(ss8str *SSSTR_RESTRICT str,
                                       ss8str const *SSSTR_RESTRICT src) {
    return ss8_copy(ss8_init(str), src);
}

SSSTR_INLINE_DEF ss8str *ss8_init_copy_ch_n(ss8str *str, char ch,
                                            size_t count) {
    return ss8_copy_ch_n(ss8_init(str), ch, count);
}

SSSTR_INLINE_DEF ss8str *ss8_init_copy_ch(ss8str *str, char ch) {
    return ss8_copy_ch(ss8_init(str), ch);
}

SSSTR_INLINE_DEF bool ss8_copy_to_bytes(ss8str const *SSSTR_RESTRICT str,
                                        char *SSSTR_RESTRICT buf,
                                        size_t bufsize) {
    SSSTR_EXTRA_ASSERT(buf != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(str, buf, bufsize);

    size_t copylen = ss8_len(str);
    bool did_fit = true;
    if (copylen > bufsize) {
        copylen = bufsize;
        did_fit = false;
    }

    memcpy(buf, ss8_const_cstr(str), copylen);
    return did_fit;
}

SSSTR_INLINE_DEF bool ss8_copy_to_cstr(ss8str const *SSSTR_RESTRICT str,
                                       char *SSSTR_RESTRICT buf,
                                       size_t bufsize) {
    SSSTR_EXTRA_ASSERT(buf != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(str, buf, bufsize);
    SSSTR_ASSERT(bufsize >= 1);
    if (bufsize == 0) // Be "safe" despite the assertion.
        return false;

    size_t copylen = ss8_len(str);
    bool did_fit = true;
    if (copylen >= bufsize) {
        copylen = bufsize - 1;
        did_fit = false;
    }

    // We could use strncpy(), but the principle of least surprise probably
    // means that we should copy any part after an internal '\0' as well.
    memcpy(buf, ss8_const_cstr(str), copylen);
    buf[copylen] = '\0';
    return did_fit;
}

SSSTR_INLINE_DEF void ss8_swap(ss8str *SSSTR_RESTRICT str1,
                               ss8str *SSSTR_RESTRICT str2) {
    SSSTR_EXTRA_ASSERT(str1 != NULL);
    SSSTR_EXTRA_ASSERT(str2 != NULL);
    ss8iNtErNaL_extra_assert_invariants(str1);
    ss8iNtErNaL_extra_assert_invariants(str2);
    ss8iNtErNaL_extra_assert_no_overlap(str1, ss8_cstr(str2),
                                        ss8iNtErNaL_bufsize(str2));

    ss8str temp;
    memcpy(&temp, str1, sizeof(ss8str));
    memcpy(str1, str2, sizeof(ss8str));
    memcpy(str2, &temp, sizeof(ss8str));
}

SSSTR_INLINE void ss8iNtErNaL_mark_moved_out(ss8str *s);
SSSTR_INLINE_DEF void ss8iNtErNaL_mark_moved_out(ss8str *s) {
#ifdef SSSTR_EXTRA_DEBUG
    ss8_copy_cstr(s, "!!! MOVED OUT SS8STR !!!");
#else
    (void)s;
#endif
}

SSSTR_INLINE_DEF ss8str *ss8_move(ss8str *SSSTR_RESTRICT dest,
                                  ss8str *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(dest != NULL);
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_invariants(dest);
    ss8iNtErNaL_extra_assert_invariants(src);
    ss8iNtErNaL_extra_assert_no_overlap(dest, ss8_const_cstr(src),
                                        ss8_len(src));

    // Our API contract places no restriction on the contents of src after the
    // move, or whether or not free() is called.

    // Swap appears to be at least as fast as short mode copy when codegen is
    // good. Free-dest-and-memcpy is also just as fast in the case where the
    // destination is not in long mode. More experience is needed to determine
    // which is better, but I'm inclined to provide the non-swap behavior,
    // because the user can always call ss8_swap() when they know it is
    // advantageous.

#if 0
    ss8_swap(dest, src);
#else
    if (dest->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] == ss8iNtErNaL_longmode)
        SSSTR_FREE(dest->iNtErNaL_L.ptr);
    memcpy(dest, src, sizeof(ss8str));
    if (src->iNtErNaL_S[ss8iNtErNaL_shortbufsiz - 1] == ss8iNtErNaL_longmode)
        ss8_init(src);
#endif

    ss8iNtErNaL_mark_moved_out(src);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_move_destroy(ss8str *SSSTR_RESTRICT dest,
                                          ss8str *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(dest != NULL);
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_invariants(dest);
    ss8iNtErNaL_extra_assert_invariants(src);
    ss8iNtErNaL_extra_assert_no_overlap(dest, ss8_const_cstr(src),
                                        ss8_len(src));

    ss8_destroy(dest);
    memcpy(dest, src, sizeof(ss8str));
    ss8iNtErNaL_deinit(src);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_init_move(ss8str *SSSTR_RESTRICT str,
                                       ss8str *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_invariants(src);
    SSSTR_EXTRA_ASSERT(str != src);

    memcpy(str, src, sizeof(ss8str));
    ss8_init(src);

    ss8iNtErNaL_mark_moved_out(src);
    return str;
}

SSSTR_INLINE_DEF ss8str *ss8_init_move_destroy(ss8str *SSSTR_RESTRICT str,
                                               ss8str *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(str != NULL);
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_invariants(src);
    SSSTR_EXTRA_ASSERT(str != src);

    memcpy(str, src, sizeof(ss8str));
    ss8iNtErNaL_deinit(src);
    return str;
}

SSSTR_INLINE_DEF ss8str *ss8_copy_substr(ss8str *SSSTR_RESTRICT dest,
                                         ss8str const *SSSTR_RESTRICT src,
                                         size_t start, size_t len) {
    ss8iNtErNaL_extra_assert_no_overlap(dest, ss8_const_cstr(src),
                                        ss8iNtErNaL_bufsize(src));

    size_t srclen = ss8_len(src);
    SSSTR_ASSERT(start <= srclen);
    if (len >= srclen - start)
        len = srclen - start;
    return ss8_copy_bytes(dest, ss8_const_cstr(src) + start, len);
}

SSSTR_INLINE_DEF ss8str *ss8_substr_inplace(ss8str *str, size_t start,
                                            size_t len) {
    size_t slen = ss8_len(str);
    SSSTR_ASSERT(start <= slen);
    if (len >= slen - start)
        len = slen - start;

    char *p = ss8_cstr(str);
    memmove(p, p + start, len);
    p[len] = '\0';
    ss8iNtErNaL_setlen(str, len);
    return str;
}

// Return size_t r = s + t but check for overflow.
SSSTR_INLINE size_t ss8iNtErNaL_add_sizes(size_t s, size_t t);
SSSTR_INLINE_DEF size_t ss8iNtErNaL_add_sizes(size_t s, size_t t) {
    size_t r = s + t;
    if (r < s) { // Extra braces, in case macro is poorly customized.
        SSSTR_SIZE_OVERFLOW();
    }
    return r;
}

// Must be cap < mincap.
SSSTR_INLINE size_t ss8iNtErNaL_growcap(size_t cap, size_t mincap);
SSSTR_INLINE_DEF size_t ss8iNtErNaL_growcap(size_t cap, size_t mincap) {
    size_t const maxcap = SIZE_MAX - 1; // Room for null terminator.
    size_t newcap = ss8iNtErNaL_three_halves(cap, maxcap);
    if (newcap < mincap)
        return mincap;
    return newcap;
}

SSSTR_INLINE void ss8iNtErNaL_grow(ss8str *str, size_t mincap);
SSSTR_INLINE_DEF void ss8iNtErNaL_grow(ss8str *str, size_t mincap) {
    size_t cap = ss8_capacity(str);
    if (mincap > cap) {
        size_t newcap = ss8iNtErNaL_growcap(cap, mincap);
        ss8iNtErNaL_reserve_impl(str, newcap);
    }
}

SSSTR_INLINE_DEF ss8str *ss8_insert_bytes(ss8str *SSSTR_RESTRICT dest,
                                          size_t pos,
                                          char const *SSSTR_RESTRICT src,
                                          size_t srclen) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(dest, src, srclen);

    size_t destlen = ss8_len(dest);
    SSSTR_ASSERT(pos <= destlen);
    size_t newlen = ss8iNtErNaL_add_sizes(destlen, srclen);

    ss8iNtErNaL_grow(dest, newlen);
    char *p = ss8_cstr(dest) + pos;
    memmove(p + srclen, p, destlen - pos + 1);
    memcpy(p, src, srclen);
    ss8iNtErNaL_setlen(dest, newlen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_insert_cstr(ss8str *SSSTR_RESTRICT dest,
                                         size_t pos,
                                         char const *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    return ss8_insert_bytes(dest, pos, src, strlen(src));
}

SSSTR_INLINE_DEF ss8str *ss8_insert(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                    ss8str const *SSSTR_RESTRICT src) {
    return ss8_insert_bytes(dest, pos, ss8_const_cstr(src), ss8_len(src));
}

SSSTR_INLINE_DEF ss8str *ss8_cat_bytes(ss8str *SSSTR_RESTRICT dest,
                                       char const *SSSTR_RESTRICT src,
                                       size_t srclen) {
    // TODO Compiler may not inline; can elide memmove()
    return ss8_insert_bytes(dest, ss8_len(dest), src, srclen);
}

SSSTR_INLINE_DEF ss8str *ss8_cat_cstr(ss8str *SSSTR_RESTRICT dest,
                                      char const *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    return ss8_cat_bytes(dest, src, strlen(src));
}

SSSTR_INLINE_DEF ss8str *ss8_cat(ss8str *SSSTR_RESTRICT dest,
                                 ss8str const *SSSTR_RESTRICT src) {
    return ss8_cat_bytes(dest, ss8_const_cstr(src), ss8_len(src));
}

SSSTR_INLINE_DEF ss8str *ss8_erase(ss8str *str, size_t pos, size_t len) {
    size_t slen = ss8_len(str);
    SSSTR_ASSERT(pos <= slen);
    if (len > slen - pos)
        len = slen - pos;

    char *p = ss8_cstr(str) + pos;
    memmove(p, p + len, slen - pos - len + 1);
    ss8iNtErNaL_setlen(str, slen - len);
    return str;
}

SSSTR_INLINE_DEF ss8str *ss8_replace_bytes(ss8str *SSSTR_RESTRICT dest,
                                           size_t pos, size_t len,
                                           char const *SSSTR_RESTRICT src,
                                           size_t srclen) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(dest, src, srclen);

    size_t destlen = ss8_len(dest);
    SSSTR_ASSERT(pos <= destlen);
    if (len > destlen - pos)
        len = destlen - pos;
    size_t newlen = ss8iNtErNaL_add_sizes(destlen - len, srclen);

    ss8iNtErNaL_grow(dest, newlen);
    char *p = ss8_cstr(dest) + pos;
    memmove(p + srclen, p + len, destlen - pos - len + 1);
    memcpy(p, src, srclen);
    ss8iNtErNaL_setlen(dest, newlen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_replace_cstr(ss8str *SSSTR_RESTRICT dest,
                                          size_t pos, size_t len,
                                          char const *SSSTR_RESTRICT src) {
    SSSTR_EXTRA_ASSERT(src != NULL);
    return ss8_replace_bytes(dest, pos, len, src, strlen(src));
}

SSSTR_INLINE_DEF ss8str *ss8_replace(ss8str *SSSTR_RESTRICT dest, size_t pos,
                                     size_t len,
                                     ss8str const *SSSTR_RESTRICT src) {
    return ss8_replace_bytes(dest, pos, len, ss8_const_cstr(src),
                             ss8_len(src));
}

SSSTR_INLINE_DEF ss8str *ss8_insert_ch_n(ss8str *dest, size_t pos, char ch,
                                         size_t count) {
    size_t destlen = ss8_len(dest);
    SSSTR_ASSERT(pos <= destlen);
    size_t newlen = ss8iNtErNaL_add_sizes(destlen, count);

    ss8iNtErNaL_grow(dest, newlen);
    char *p = ss8_cstr(dest) + pos;
    memmove(p + count, p, destlen - pos + 1);
    memset(p, ch, count);
    ss8iNtErNaL_setlen(dest, newlen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_cat_ch_n(ss8str *dest, char ch, size_t count) {
    // TODO Compiler may not inline; can elide memmove()
    return ss8_insert_ch_n(dest, ss8_len(dest), ch, count);
}

SSSTR_INLINE_DEF ss8str *ss8_replace_ch_n(ss8str *dest, size_t pos, size_t len,
                                          char ch, size_t count) {
    size_t destlen = ss8_len(dest);
    SSSTR_ASSERT(pos <= destlen);
    if (len > destlen - pos)
        len = destlen - pos;
    size_t newlen = ss8iNtErNaL_add_sizes(destlen - len, count);

    ss8iNtErNaL_grow(dest, newlen);
    char *p = ss8_cstr(dest) + pos;
    memmove(p + count, p + len, destlen - pos - len + 1);
    memset(p, ch, count);
    ss8iNtErNaL_setlen(dest, newlen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_insert_ch(ss8str *dest, size_t pos, char ch) {
    return ss8_insert_ch_n(dest, pos, ch, 1);
}

SSSTR_INLINE_DEF ss8str *ss8_cat_ch(ss8str *dest, char ch) {
    return ss8_cat_ch_n(dest, ch, 1);
}

SSSTR_INLINE_DEF ss8str *ss8_replace_ch(ss8str *dest, size_t pos, size_t len,
                                        char ch) {
    return ss8_replace_ch_n(dest, pos, len, ch, 1);
}

SSSTR_INLINE_DEF int ss8_cmp_bytes(ss8str const *lhs, char const *rhs,
                                   size_t rhslen) {
    SSSTR_EXTRA_ASSERT(rhs != NULL);

    size_t llen = ss8_len(lhs);
    size_t cmplen = llen;
    if (rhslen < cmplen)
        cmplen = rhslen;
    char const *l = ss8_const_cstr(lhs);
    int c = memcmp(l, rhs, cmplen);
    if (c != 0 || llen == rhslen)
        return c;
    if (llen < rhslen) // cmplen == llen
        return -1;
    return 1;
}

// Note that this differs from strcmp(): if the lhs has embedded nulls, it will
// compare greater than an rhs with the same byte sequence. In other words,
// only the rhs is treated as a null-terminated string.
SSSTR_INLINE_DEF int ss8_cmp_cstr(ss8str const *lhs, char const *rhs) {
    SSSTR_EXTRA_ASSERT(rhs != NULL);
    return ss8_cmp_bytes(lhs, rhs, strlen(rhs));
}

SSSTR_INLINE_DEF int ss8_cmp(ss8str const *lhs, ss8str const *rhs) {
    return ss8_cmp_bytes(lhs, ss8_const_cstr(rhs), ss8_len(rhs));
}

SSSTR_INLINE_DEF int ss8_cmp_ch(ss8str const *lhs, char rhs) {
    size_t llen = ss8_len(lhs);
    if (llen == 0)
        return -1;
    int c = (unsigned char)ss8_const_cstr(lhs)[0] - (unsigned char)rhs;
    if (c != 0 || llen == 1)
        return c;
    return 1;
}

SSSTR_INLINE_DEF bool ss8_equals_bytes(ss8str const *lhs, char const *rhs,
                                       size_t rhslen) {
    SSSTR_EXTRA_ASSERT(rhs != NULL);

    size_t llen = ss8_len(lhs);
    if (llen != rhslen)
        return false;
    return memcmp(ss8_const_cstr(lhs), rhs, llen) == 0;
}

SSSTR_INLINE_DEF bool ss8_equals_cstr(ss8str const *lhs, char const *rhs) {
    SSSTR_EXTRA_ASSERT(rhs != NULL);
    return ss8_equals_bytes(lhs, rhs, strlen(rhs));
}

SSSTR_INLINE_DEF bool ss8_equals(ss8str const *lhs, ss8str const *rhs) {
    return ss8_equals_bytes(lhs, ss8_const_cstr(rhs), ss8_len(rhs));
}

SSSTR_INLINE_DEF bool ss8_equals_ch(ss8str const *lhs, char rhs) {
    return ss8_len(lhs) == 1 && ss8_const_cstr(lhs)[0] == rhs;
}

SSSTR_INLINE_DEF size_t ss8_find_bytes(ss8str const *haystack, size_t start,
                                       char const *needle, size_t needlelen) {
    SSSTR_EXTRA_ASSERT(needle != NULL);

    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    SSSTR_ASSERT(start <= haystacklen);
    if (needlelen == 0)
        return start;
    char const *begin = h + start;
    char const *end = h + haystacklen - needlelen + 1;
    for (char const *p = begin; p < end; ++p) {
        p = SSSTR_CHARP_MEMCHR(p, needle[0], end - p);
        if (!p)
            return SIZE_MAX;
        if (memcmp(p, needle, needlelen) == 0)
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_cstr(ss8str const *haystack, size_t start,
                                      char const *needle) {
    SSSTR_EXTRA_ASSERT(needle != NULL);
    return ss8_find_bytes(haystack, start, needle, strlen(needle));
}

SSSTR_INLINE_DEF size_t ss8_find(ss8str const *haystack, size_t start,
                                 ss8str const *needle) {
    return ss8_find_bytes(haystack, start, ss8_const_cstr(needle),
                          ss8_len(needle));
}

SSSTR_INLINE_DEF size_t ss8_find_ch(ss8str const *haystack, size_t start,
                                    char needle) {
    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    SSSTR_ASSERT(start <= haystacklen);
    char const *p = SSSTR_CHARP_MEMCHR(h + start, needle, haystacklen - start);
    return p ? (size_t)(p - h) : SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_not_ch(ss8str const *haystack, size_t start,
                                        char needle) {
    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    SSSTR_ASSERT(start <= haystacklen);
    char const *begin = h + start;
    char const *end = h + haystacklen;
    for (char const *p = begin; p < end; ++p) {
        if (*p != needle)
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_rfind_bytes(ss8str const *haystack, size_t start,
                                        char const *needle, size_t needlelen) {
    SSSTR_EXTRA_ASSERT(needle != NULL);

    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    if (needlelen > haystacklen)
        return SIZE_MAX;
    SSSTR_ASSERT(start <= haystacklen);
    if (needlelen == 0)
        return start;
    char const *end = h + haystacklen - needlelen + 1;
    char const *rbegin = h + start;
    if (rbegin >= end)
        rbegin = end - 1;
    // Avoid 'rend = h - 1' because GCC -Warray-bounds will flag it.

    for (char const *p = rbegin; p >= h; --p) {
        // Pay the price of a function call only when first byte matches.
        if (*p != needle[0])
            continue;
        if (memcmp(p, needle, needlelen) == 0)
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_rfind_cstr(ss8str const *haystack, size_t start,
                                       char const *needle) {
    SSSTR_EXTRA_ASSERT(needle != NULL);
    return ss8_rfind_bytes(haystack, start, needle, strlen(needle));
}

SSSTR_INLINE_DEF size_t ss8_rfind(ss8str const *haystack, size_t start,
                                  ss8str const *needle) {
    return ss8_rfind_bytes(haystack, start, ss8_const_cstr(needle),
                           ss8_len(needle));
}

SSSTR_INLINE_DEF size_t ss8_rfind_ch(ss8str const *haystack, size_t start,
                                     char needle) {
    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    if (haystacklen == 0)
        return SIZE_MAX;
    char const *end = h + haystacklen - 1 + 1;
    SSSTR_ASSERT(start <= haystacklen);
    char const *rbegin = h + start;
    if (rbegin >= end)
        rbegin = end - 1;
    char const *rend = h - 1;

    for (char const *p = rbegin; p > rend; --p) {
        if (*p == needle)
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_rfind_not_ch(ss8str const *haystack, size_t start,
                                         char needle) {
    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    char const *end = h + haystacklen;
    SSSTR_ASSERT(start <= haystacklen);
    char const *rbegin = h + start;
    if (rbegin >= end)
        rbegin = end - 1;
    char const *rend = h - 1;

    for (char const *p = rbegin; p > rend; --p) {
        if (*p != needle)
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_first_of_bytes(ss8str const *haystack,
                                                size_t start,
                                                char const *needles,
                                                size_t count) {
    SSSTR_EXTRA_ASSERT(needles != NULL);

    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    SSSTR_ASSERT(start <= haystacklen);
    char const *begin = h + start;
    char const *end = h + haystacklen;
    for (char const *p = begin; p < end; ++p) {
        // TODO: Clang can inline memchr() when needles is a literal but GCC
        // and MSVC appear not to. Would it be faster if we use a plain loop
        // when needles is short?
        if (SSSTR_CHARP_MEMCHR(needles, *p, count))
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_first_not_of_bytes(ss8str const *haystack,
                                                    size_t start,
                                                    char const *needles,
                                                    size_t count) {
    SSSTR_EXTRA_ASSERT(needles != NULL);

    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    SSSTR_ASSERT(start <= haystacklen);
    char const *begin = h + start;
    char const *end = h + haystacklen;
    for (char const *p = begin; p < end; ++p) {
        if (!SSSTR_CHARP_MEMCHR(needles, *p, count))
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_last_of_bytes(ss8str const *haystack,
                                               size_t start,
                                               char const *needles,
                                               size_t count) {
    SSSTR_EXTRA_ASSERT(needles != NULL);

    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    char const *end = h + haystacklen;
    SSSTR_ASSERT(start <= haystacklen);
    char const *rbegin = h + start;
    if (rbegin >= end)
        rbegin = end - 1;
    char const *rend = h - 1;

    for (char const *p = rbegin; p > rend; --p) {
        if (SSSTR_CHARP_MEMCHR(needles, *p, count))
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_last_not_of_bytes(ss8str const *haystack,
                                                   size_t start,
                                                   char const *needles,
                                                   size_t count) {
    SSSTR_EXTRA_ASSERT(needles != NULL);

    char const *h = ss8_const_cstr(haystack);
    size_t haystacklen = ss8_len(haystack);
    char const *end = h + haystacklen;
    SSSTR_ASSERT(start <= haystacklen);
    char const *rbegin = h + start;
    if (rbegin >= end)
        rbegin = end - 1;
    char const *rend = h - 1;

    for (char const *p = rbegin; p > rend; --p) {
        if (!SSSTR_CHARP_MEMCHR(needles, *p, count))
            return p - h;
    }
    return SIZE_MAX;
}

SSSTR_INLINE_DEF size_t ss8_find_first_of_cstr(ss8str const *haystack,
                                               size_t start,
                                               char const *needles) {
    SSSTR_EXTRA_ASSERT(needles != NULL);
    return ss8_find_first_of_bytes(haystack, start, needles, strlen(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_first_not_of_cstr(ss8str const *haystack,
                                                   size_t start,
                                                   char const *needles) {
    SSSTR_EXTRA_ASSERT(needles != NULL);
    return ss8_find_first_not_of_bytes(haystack, start, needles,
                                       strlen(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_last_of_cstr(ss8str const *haystack,
                                              size_t start,
                                              char const *needles) {
    SSSTR_EXTRA_ASSERT(needles != NULL);
    return ss8_find_last_of_bytes(haystack, start, needles, strlen(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_last_not_of_cstr(ss8str const *haystack,
                                                  size_t start,
                                                  char const *needles) {
    SSSTR_EXTRA_ASSERT(needles != NULL);
    return ss8_find_last_not_of_bytes(haystack, start, needles,
                                      strlen(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_first_of(ss8str const *haystack, size_t start,
                                          ss8str const *needles) {
    return ss8_find_first_of_bytes(haystack, start, ss8_const_cstr(needles),
                                   ss8_len(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_first_not_of(ss8str const *haystack,
                                              size_t start,
                                              ss8str const *needles) {
    return ss8_find_first_not_of_bytes(
        haystack, start, ss8_const_cstr(needles), ss8_len(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_last_of(ss8str const *haystack, size_t start,
                                         ss8str const *needles) {
    return ss8_find_last_of_bytes(haystack, start, ss8_const_cstr(needles),
                                  ss8_len(needles));
}

SSSTR_INLINE_DEF size_t ss8_find_last_not_of(ss8str const *haystack,
                                             size_t start,
                                             ss8str const *needles) {
    return ss8_find_last_not_of_bytes(haystack, start, ss8_const_cstr(needles),
                                      ss8_len(needles));
}

SSSTR_INLINE_DEF bool ss8_starts_with_bytes(ss8str const *str,
                                            char const *prefix,
                                            size_t prefixlen) {
    SSSTR_EXTRA_ASSERT(prefix != NULL || prefixlen == 0);

    if (ss8_len(str) < prefixlen)
        return false;
    return memcmp(ss8_const_cstr(str), prefix, prefixlen) == 0;
}

SSSTR_INLINE_DEF bool ss8_starts_with_cstr(ss8str const *str,
                                           char const *prefix) {
    SSSTR_EXTRA_ASSERT(prefix != NULL);
    return ss8_starts_with_bytes(str, prefix, strlen(prefix));
}

SSSTR_INLINE_DEF bool ss8_starts_with(ss8str const *str,
                                      ss8str const *prefix) {
    return ss8_starts_with_bytes(str, ss8_const_cstr(prefix), ss8_len(prefix));
}

SSSTR_INLINE_DEF bool ss8_starts_with_ch(ss8str const *str, char ch) {
    return ss8_len(str) > 0 && ss8_front(str) == ch;
}

SSSTR_INLINE_DEF bool
ss8_ends_with_bytes(ss8str const *str, char const *suffix, size_t suffixlen) {
    SSSTR_EXTRA_ASSERT(suffix != NULL);

    size_t len = ss8_len(str);
    if (len < suffixlen)
        return false;
    char const *end = ss8_const_cstr(str) + len;
    return memcmp(end - suffixlen, suffix, suffixlen) == 0;
}

SSSTR_INLINE_DEF bool ss8_ends_with_cstr(ss8str const *str,
                                         char const *suffix) {
    SSSTR_EXTRA_ASSERT(suffix != NULL);
    return ss8_ends_with_bytes(str, suffix, strlen(suffix));
}

SSSTR_INLINE_DEF bool ss8_ends_with(ss8str const *str, ss8str const *suffix) {
    return ss8_ends_with_bytes(str, ss8_const_cstr(suffix), ss8_len(suffix));
}

SSSTR_INLINE_DEF bool ss8_ends_with_ch(ss8str const *str, char ch) {
    return ss8_len(str) > 0 && ss8_back(str) == ch;
}

SSSTR_INLINE_DEF bool ss8_contains_bytes(ss8str const *str, char const *infix,
                                         size_t infixlen) {
    return ss8_find_bytes(str, 0, infix, infixlen) != SIZE_MAX;
}

SSSTR_INLINE_DEF bool ss8_contains_cstr(ss8str const *str, char const *infix) {
    SSSTR_EXTRA_ASSERT(infix != NULL);
    return ss8_contains_bytes(str, infix, strlen(infix));
}

SSSTR_INLINE_DEF bool ss8_contains(ss8str const *str, ss8str const *infix) {
    return ss8_contains_bytes(str, ss8_const_cstr(infix), ss8_len(infix));
}

SSSTR_INLINE_DEF bool ss8_contains_ch(ss8str const *str, char ch) {
    return ss8_find_ch(str, 0, ch) != SIZE_MAX;
}

SSSTR_INLINE_DEF ss8str *ss8_lstrip_bytes(ss8str *SSSTR_RESTRICT str,
                                          char const *SSSTR_RESTRICT chars,
                                          size_t count) {
    size_t b = ss8_find_first_not_of_bytes(str, 0, chars, count);
    if (b == SIZE_MAX)
        b = ss8_len(str);
    return ss8_substr_inplace(str, b, SIZE_MAX);
}

SSSTR_INLINE_DEF ss8str *ss8_rstrip_bytes(ss8str *SSSTR_RESTRICT str,
                                          char const *SSSTR_RESTRICT chars,
                                          size_t count) {
    size_t e = ss8_find_last_not_of_bytes(str, ss8_len(str), chars, count);
    size_t n = e == SIZE_MAX ? 0 : e + 1;
    return ss8_substr_inplace(str, 0, n);
}

SSSTR_INLINE_DEF ss8str *ss8_strip_bytes(ss8str *SSSTR_RESTRICT str,
                                         char const *SSSTR_RESTRICT chars,
                                         size_t count) {
    size_t len = ss8_len(str);
    size_t n;
    size_t b = ss8_find_first_not_of_bytes(str, 0, chars, count);
    if (b == SIZE_MAX) {
        b = n = 0;
    } else {
        size_t e = ss8_find_last_not_of_bytes(str, len, chars, count);
        n = e - b + 1;
    }
    return ss8_substr_inplace(str, b, n);
}

SSSTR_INLINE_DEF ss8str *ss8_lstrip_cstr(ss8str *SSSTR_RESTRICT str,
                                         char const *SSSTR_RESTRICT chars) {
    SSSTR_EXTRA_ASSERT(chars != NULL);
    return ss8_lstrip_bytes(str, chars, strlen(chars));
}

SSSTR_INLINE_DEF ss8str *ss8_rstrip_cstr(ss8str *SSSTR_RESTRICT str,
                                         char const *SSSTR_RESTRICT chars) {
    SSSTR_EXTRA_ASSERT(chars != NULL);
    return ss8_rstrip_bytes(str, chars, strlen(chars));
}

SSSTR_INLINE_DEF ss8str *ss8_strip_cstr(ss8str *SSSTR_RESTRICT str,
                                        char const *SSSTR_RESTRICT chars) {
    SSSTR_EXTRA_ASSERT(chars != NULL);
    return ss8_strip_bytes(str, chars, strlen(chars));
}

SSSTR_INLINE_DEF ss8str *ss8_lstrip(ss8str *SSSTR_RESTRICT str,
                                    ss8str const *SSSTR_RESTRICT chars) {
    return ss8_lstrip_bytes(str, ss8_const_cstr(chars), ss8_len(chars));
}

SSSTR_INLINE_DEF ss8str *ss8_rstrip(ss8str *SSSTR_RESTRICT str,
                                    ss8str const *SSSTR_RESTRICT chars) {
    return ss8_rstrip_bytes(str, ss8_const_cstr(chars), ss8_len(chars));
}

SSSTR_INLINE_DEF ss8str *ss8_strip(ss8str *SSSTR_RESTRICT str,
                                   ss8str const *SSSTR_RESTRICT chars) {
    return ss8_strip_bytes(str, ss8_const_cstr(chars), ss8_len(chars));
}

SSSTR_INLINE_DEF ss8str *ss8_lstrip_ch(ss8str *str, char ch) {
    size_t b = ss8_find_not_ch(str, 0, ch);
    if (b == SIZE_MAX)
        b = ss8_len(str);
    return ss8_substr_inplace(str, b, SIZE_MAX);
}

SSSTR_INLINE_DEF ss8str *ss8_rstrip_ch(ss8str *str, char ch) {
    size_t e = ss8_rfind_not_ch(str, ss8_len(str), ch);
    size_t n = e == SIZE_MAX ? 0 : e + 1;
    return ss8_substr_inplace(str, 0, n);
}

SSSTR_INLINE_DEF ss8str *ss8_strip_ch(ss8str *str, char ch) {
    size_t len = ss8_len(str);
    size_t n;
    size_t b = ss8_find_not_ch(str, 0, ch);
    if (b == SIZE_MAX) {
        b = n = 0;
    } else {
        size_t e = ss8_rfind_not_ch(str, len, ch);
        n = e - b + 1;
    }
    return ss8_substr_inplace(str, b, n);
}

// We need va_copy() for the [v]s[n]printf functions; va_copy requires (C99 or)
// C++11. (Note that we do not disable the prototypes for these functions.)
#if !defined(__cplusplus) || __cplusplus >= 201103L

// Should not be used with unbounded input, because behavior is undefined if
// the resulting string's length is not less than INT_MAX.
SSSTR_INLINE_DEF ss8str *ss8_cat_vsprintf(ss8str *SSSTR_RESTRICT dest,
                                          char const *SSSTR_RESTRICT fmt,
                                          va_list args) {
    SSSTR_EXTRA_ASSERT(fmt != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(dest, fmt, strlen(fmt));

    // The return value (int) of [v]snprintf() in the case that the resulting
    // string would exceed INT_MAX bytes is poorly specified. POSIX specifies
    // that a negative value be returned and errno set to EOVERFLOW, but ISO C
    // does not mention this situation at all. We'll just avoid ever calling
    // vsnprintf() with a size greater than or equal to INT_MAX, and panic on
    // all errors returned by vsnprintf().

    size_t const destlen = ss8_len(dest);
    size_t const destcap = ss8_capacity(dest);
    size_t maxformatsize = destcap - destlen + 1;
    if (maxformatsize > INT_MAX)
        maxformatsize = INT_MAX;

    // First try without enlarging dest.
    char *p = ss8_cstr(dest) + destlen;
    va_list args_copy;
    va_copy(args_copy, args);
    errno = 0;
    int r1 = vsnprintf(p, maxformatsize, fmt, args_copy);
    va_end(args_copy);
    if (r1 < 0)
        SSSTR_PANIC_ERRNO("vsnprintf error");
    size_t resultlen = r1;
    if (resultlen <= destcap - destlen) {
        ss8iNtErNaL_setlen(dest, destlen + resultlen);
        return dest;
    }
    *p = '\0'; // Recover invariant (longjmp() safety)

    // Second try with sufficient capacity.
    if (resultlen == INT_MAX) // Cannot handle this edge case.
        SSSTR_PANIC("vsnprintf result too large");
    size_t totallen = ss8iNtErNaL_add_sizes(destlen, resultlen);
    if (destlen == 0)
        ss8_reserve(dest, totallen);
    else
        ss8iNtErNaL_grow(dest, totallen);
    errno = 0;
    int r2 = vsnprintf(ss8_cstr(dest) + destlen, resultlen + 1, fmt, args);
    if (r2 != r1)
        SSSTR_PANIC_ERRNO("vsnprintf error");
    ss8iNtErNaL_setlen(dest, totallen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_vsprintf(ss8str *SSSTR_RESTRICT dest,
                                      char const *SSSTR_RESTRICT fmt,
                                      va_list args) {
    return ss8_cat_vsprintf(ss8_clear(dest), fmt, args);
}

// Behavior is undefined if maxlen is not less than INT_MAX.
SSSTR_INLINE_DEF ss8str *ss8_cat_vsnprintf(ss8str *SSSTR_RESTRICT dest,
                                           size_t maxlen,
                                           char const *SSSTR_RESTRICT fmt,
                                           va_list args) {
    SSSTR_EXTRA_ASSERT(fmt != NULL);
    ss8iNtErNaL_extra_assert_no_overlap(dest, fmt, strlen(fmt));

    // See comments in ss8_vsprintf().

    SSSTR_ASSERT(maxlen < INT_MAX);
    size_t const destlen = ss8_len(dest);
    size_t const destcap = ss8_capacity(dest);
    size_t maxformatsize = destcap - destlen + 1;
    if (maxformatsize > maxlen + 1)
        maxformatsize = maxlen + 1;

    // First try without enlarging dest.
    char *p = ss8_cstr(dest) + destlen;
    va_list args_copy;
    va_copy(args_copy, args);
    errno = 0;
    int r1 = vsnprintf(p, maxformatsize, fmt, args_copy);
    va_end(args_copy);
    if (r1 < 0)
        SSSTR_PANIC_ERRNO("vsnprintf error");
    size_t resultlen = r1;
    if (resultlen > maxlen)
        resultlen = maxlen;
    if (resultlen < maxformatsize) {
        ss8iNtErNaL_setlen(dest, destlen + resultlen);
        return dest;
    }
    *p = '\0'; // Recover invariant (longjmp() safety)

    // Second try with sufficient capacity.
    size_t totallen = ss8iNtErNaL_add_sizes(destlen, resultlen);
    if (destlen == 0)
        ss8_reserve(dest, totallen);
    else
        ss8iNtErNaL_grow(dest, totallen);
    errno = 0;
    int r2 = vsnprintf(ss8_cstr(dest) + destlen, resultlen + 1, fmt, args);
    if (r2 != r1)
        SSSTR_PANIC_ERRNO("vsnprintf error");
    ss8iNtErNaL_setlen(dest, totallen);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_vsnprintf(ss8str *SSSTR_RESTRICT dest,
                                       size_t maxlen,
                                       char const *SSSTR_RESTRICT fmt,
                                       va_list args) {
    return ss8_cat_vsnprintf(ss8_clear(dest), maxlen, fmt, args);
}

SSSTR_INLINE_DEF ss8str *ss8_cat_sprintf(ss8str *SSSTR_RESTRICT dest,
                                         char const *SSSTR_RESTRICT fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ss8_cat_vsprintf(dest, fmt, args);
    va_end(args);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_sprintf(ss8str *SSSTR_RESTRICT dest,
                                     char const *SSSTR_RESTRICT fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ss8_vsprintf(dest, fmt, args);
    va_end(args);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_cat_snprintf(ss8str *SSSTR_RESTRICT dest,
                                          size_t maxlen,
                                          char const *SSSTR_RESTRICT fmt,
                                          ...) {
    va_list args;
    va_start(args, fmt);
    ss8_cat_vsnprintf(dest, maxlen, fmt, args);
    va_end(args);
    return dest;
}

SSSTR_INLINE_DEF ss8str *ss8_snprintf(ss8str *SSSTR_RESTRICT dest,
                                      size_t maxlen,
                                      char const *SSSTR_RESTRICT fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ss8_vsnprintf(dest, maxlen, fmt, args);
    va_end(args);
    return dest;
}

#endif // C++ >= 11

// Avoid leaking internal macros
#ifndef SSSTR_TESTING

#undef SSSTR_INLINE
#undef SSSTR_INLINE_DEF
#undef SSSTR_RESTRICT
#undef SSSTR_PANIC
#undef SSSTR_PANIC_ERRNO
#undef SSSTR_CHARP_MALLOC
#undef SSSTR_CHARP_REALLOC
#undef SSSTR_CHARP_MEMCHR
#undef SSSTR_ASSERT_MSG
#undef SSSTR_EXTRA_ASSERT
#undef SSSTR_EXTRA_ASSERT_MSG

#ifdef SSSTR_USING_DEFAULT_MALLOC
#undef SSSTR_MALLOC
#undef SSSTR_REALLOC
#undef SSSTR_FREE
#undef SSSTR_USING_DEFAULT_MALLOC
#endif

#ifdef SSSTR_USING_DEFAULT_ASSERT
#undef SSSTR_ASSERT
#undef SSSTR_USING_DEFAULT_ASSERT
#endif

#ifdef SSSTR_USING_DEFAULT_OUT_OF_MEMORY
#undef SSSTR_OUT_OF_MEMORY
#undef SSSTR_USING_DEFAULT_OUT_OF_MEMORY
#endif

#ifdef SSSTR_USING_DEFAULT_SIZE_OVERFLOW
#undef SSSTR_SIZE_OVERFLOW
#undef SSSTR_USING_DEFAULT_SIZE_OVERFLOW
#endif

#endif // SSSTR_TESTING

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SS8STR_H_INCLUDED
