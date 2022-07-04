<!--
This file is part of the Ssstr string library.
Copyright 2022 Board of Regents of the University of Wisconsin System
SPDX-License-Identifier: MIT
-->

# Ssstr: a string library for C

**Ssstr** is a header-only C library for byte string manipulation.

```c
#include <ss8str.h>
```

## Features and Design

- Safer and simpler-to-use analogs of most of the C standard library `string.h`
  functions.
- Additional conveneince functions similar to C++ `std::string` member
  functions and more.
- Strings are always null-terminated and buffer size is managed automatically,
  avoiding tricky edge cases.
- Byte strings containing embedded null bytes can be handled.
- Convenient and efficient interoperability with standard null-terminated
  strings and unterminated byte buffers.
- Single-file **header-only library**.
- **Small string optimization**: strings up to 31 bytes (on 64-bit platforms)
  or 15 bytes (32-bit) can be stored directly in the object (usually on the
  stack), avoiding dynamic storage allocation.
- Compatible with C99 and later.
- Unit tested thoroughly.
- [Documented](https://marktsuchida.github.io/ssstr/man7/ssstr.7.html)
  carefully.

## Non-features

**Ssstr** is meant for simple byte string manipulation, with the interpretation
of the bytes completely left to the user.

- There is no explicit support for UTF-8 or any other specific character set
  (although UTF-8 strings will work well with **Ssstr** if used correctly).
- There is no support for breaking natural language strings into words, glyphs,
  codepoints, etc.

There are many C libraries (such as [ICU](https://icu.unicode.org/)) that
provide these capabilities, and **Ssstr** should be able to interoperate with
most of them.

Most of the **Ssstr** functions are safe for use with UTF-8 strings, but those
that break up strings at arbitrary offsets (such as `ss8_copy_substr()`,
`ss8_set_len()`, or `ss8_replace()`) need to be used with care so as not to end
up with partial encoding sequences. Also, string search and comparison will not
take into account possible Unicode canonical equivalents.

## Usage

API reference is available as
[manual pages](https://marktsuchida.github.io/ssstr/man7/ssstr.7.html).

### String lifecycle

Strings are represented by the type `ss8str`. An `ss8str` must be initialized
before any use by passing its address to `ss8_init()`:

<!--
%TEST_SNIPPET
-->

```c
ss8str s;
ss8_init(&s);
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&s);
-->

Note that directly assgning to, or initializing, an already initialized
`ss8str` results in undefined behavior.

An `ss8str` is intended to be allocated on the stack as a local variable unless
it is part of a larger data structure.

After use, an `ss8str` must be destroyed in order to release any dynamically
allocated memory it may have used:

<!--
%TEST_SNIPPET
%SNIPPET_PROLOGUE ss8str s;
%SNIPPET_PROLOGUE ss8_init(&s);
-->

```c
ss8_destroy(&s);
```

After a call to `ss8_destroy()`, the `ss8str` is in an invalid state and cannot
be reused unless another call to `ss8_init()` is made. Calling `ss8_destroy()`
on an uninitialized (or already-destroyed) `ss8str` results in undefined
behavior.

### Initializing a string with contents

Instead of `ss8_init()`, you can use one of the other "init" functions to set
the string upon creation:

<!--
%TEST_SNIPPET
-->

```c
ss8str greeting, another, bytes012, comma, fortran_indent;
ss8_init_copy_cstr(&greeting, "Hello");
ss8_init_copy(&another, &greeting);
ss8_init_copy_bytes(&bytes012, "\0\1\2", 3);
ss8_init_copy_ch(&comma, ',');                // Set to ","
ss8_init_copy_ch_n(&fortran_indent, ' ', 6);  // Set to "      "

// ...

ss8_destroy(&greeting);
ss8_destroy(&another);
ss8_destroy(&bytes012);
ss8_destroy(&comma);
ss8_destroy(&fortran_indent);
```

### Assigning to a string

There is a set of similar functions to set the contents of an existing (already
initialized) `ss8str`, overwriting whatever it contained:

<!--
%TEST_SNIPPET
-->

```c
ss8str s, t;
ss8_init(&s);
ss8_init(&t);

ss8_copy_cstr(&s, "Hello");       // Set s to "Hello"
ss8_copy(&t, &s);                 // Set t to "Hello"
ss8_copy_bytes(&s, "\0\1\2", 3);  // Set s to "\0\1\2"
ss8_copy_ch(&s, ',');             // Set s to ","
ss8_copy_ch_n(&s, ' ', 6);        // Set s to "      "
ss8_clear(&s);                    // Set s to ""

// ...

ss8_destroy(&s);
ss8_destroy(&t);
```

### Accessing contents

<!--
%TEST_SNIPPET
-->

```c
ss8str example;
ss8_init_copy_cstr(&example, "Hello");

size_t len = ss8_len(&example);             // 5
bool e = ss8_is_empty(&example);            // false

// Pass to a function expecting a null-terminated string
puts(ss8_const_cstr(&example));

// Pass a right substring (prints "lo!\n")
printf("%s!\n", ss8_const_cstr_suffix(&example, 3));

char ch = ss8_at(&example, 1);              // 'e'
char first = ss8_front(&example);           // 'H'
char last = ss8_back(&example);             // 'o'

ss8_destroy(&example);
```

<!--
%SNIPPET_EPILOGUE (void)len;
%SNIPPET_EPILOGUE (void)e;
%SNIPPET_EPILOGUE (void)ch;
%SNIPPET_EPILOGUE (void)first;
%SNIPPET_EPILOGUE (void)last;
-->

Note that the result of `ss8_len(&s)` and `strlen(ss8_const_cstr(&s))` would
differ if the string contained embedded null bytes.

The `ss8_const_cstr()` function returns a pointer to the string's internal
buffer, without making a copy. The string is always null-terminated, whether or
not it contains embedded null bytes.

### Mutating characters

<!--
%TEST_SNIPPET
-->

```c
ss8str s;
ss8_init_copy_cstr(&s, "ABCDE");
ss8_set_at(&s, 2, 'c');  // s is now "ABcDE"
ss8_set_front(&s, 'a');  // s is now "aBcDE"
ss8_set_back(&s, 'e');   // s is now "aBcDe"
ss8_destroy(&s);
```

### Calling C APIs that produce a string

Many C APIs have functions that take a string buffer pointer and size (or
maximum string length), and write a string into the buffer. These vary quite a
bit in how they deal with the size limit: some return the number of bytes
written, some return the number of bytes that would fit the whole result, and
some provide no indication of whether the result fit in the buffer or not.

**Ssstr** provides functions that make it easy to deal with most of these
functions.

The `ss8_set_len()` function adjusts the length of the string, leaving any new
portion of the string uninitialized. This can be used to prepare a destination
buffer of the desired size.

`ss8_cstr()`, the non-const version of `ss8_const_cstr()`, returns a pointer
that you can pass to functions that will write a string. There is also
`ss8_cstr_suffix()` which you can use to append onto an existing string.

If you adjusted the length of an `ss8str` using `ss8_set_len()`, you probably
need to readjust it after the call to the string-producing function, unless you
knew beforehand the exact length. For this, you can call `ss8_set_len()` again
(if the function returned the number of bytes written), or use
`ss8_set_len_to_cstrlen()`, (if the written string is null-terminated).

Some (poorly-designed) APIs may not provide a way to determine the result
length before you pass a large-enough buffer. In this case, you can use
`ss8_grow_len()` to progressively increase the length of the string being used
as the destination buffer, calling the API function in a loop until you
succeed. The `ss8_grow_len()` function is similar to `ss8_set_len()`, but will
automatically chose a new length.

Also depending on the API, you may need to pass either the _maximum string
length_ (not including any null terminator) or _destination buffer size_
(including the null terminator). In the latter case, it is safe to pass
`length + 1`, provided that the function will only ever write `\0` to the byte
just beyond the length of the `ss8str`. (But make sure to check that this is
the case; some functions, such as `strncpy()`, have confusing behavior.)

**Note:** Besure not to confuse length and capacity. It is illegal to write
beyond the _length_ of an `ss8str` when using `ss8_cstr()` (with the null
terminator exception mentioned above), however large its current _capacity_ may
be.

#### Example: Calling `strftime()`

The standard library function `strftime()` returns the number of bytes written
(not including the null terminator) upon success, but does not provide a way to
determine the necessary buffer size beforehand. It is therefore most correct to
call it with successively increasing buffer sizes:

<!--
%TEST_SNIPPET
-->

```c
// UTC time when Earth was closest to the Sun in 2022.
struct tm perihelion = {
    .tm_year = 122,  // 2022
    .tm_mon = 0,     // January
    .tm_mday = 4,    // 4th
    .tm_hour = 7, .tm_min = 10, .tm_sec = 0
};

ss8str datetime;
ss8_init(&datetime);

size_t len = 0;
while (!len) {
    ss8_grow_len(&datetime, SIZE_MAX, SIZE_MAX);
    len = strftime(ss8_cstr(&datetime), ss8_len(&datetime) + 1,
                   "%Y-%m-%d %H:%M:%S UTC", &perihelion);
}
ss8_set_len(&datetime, len);

printf("%s\n", ss8_const_cstr(&datetime));

ss8_destroy(&datetime);
```

<!--
%SNIPPET_EPILOGUE TEST_ASSERT_EQUAL_size_t(23, len);
-->

This example might be slightly superfluous because the format string used here
results in a fixed 23-byte result, but it is meant as a demonstration for this
common pattern (and other formats can generate results whose length depends on
the current locale). And the use of `ss8_grow_len()` ensures that the first
iteration will try the maximum length available without dynamic storage
allocation.

### Copying to a plain C buffer

Conversely, you may have an `ss8str` and wish to write an API function for
regular C string users. This is even easier:

<!--
%TEST_SNIPPET COMPILE_ONLY FILE_SCOPE
-->

```c
// Returns length that would have been written given sufficient bufsize.
// Can call with buf == NULL to determine required size.
size_t get_greeting(char *buf, size_t bufsize) {
    // In real code, this string would come from elsewhere.
    ss8str greeting;
    ss8_init_copy_cstr(&greeting, "Hello, World!");

    if (buf)
        ss8_copy_to_cstr(&greeting, buf, bufsize);  // Copy portion that fits.

    size_t ret = ss8_len(&greeting);
    ss8_destroy(&greeting);
    return ret;
}
```

There is also the variant `ss8_copy_to_bytes()`, which does the same thing as
`ss8_copy_to_cstr()` but doesn't add a null terminator.

### Concatenating and assembling strings

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str dest, src;
%SNIPPET_PROLOGUE ss8_init(&dest);
%SNIPPET_PROLOGUE ss8_init(&src);
%SNIPPET_PROLOGUE char *cstr = 0, *buf = 0;
%SNIPPET_PROLOGUE size_t len = 0, count = 0;
-->

```c
ss8_cat(&dest, &src);
ss8_cat_cstr(&dest, cstr);
ss8_cat_bytes(&dest, buf, len);
ss8_cat_ch(&dest, 'c');
ss8_cat_ch_n(&dest, 'c', count);
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&dest);
%SNIPPET_EPILOGUE ss8_destroy(&src);
-->

#### Reserving space

When assembling a string by concatenating multiple short strings, the `ss8str`
might need to reallocate its internal buffer multiple times to hold the growing
string. Memory allocation is slow, so this is done by enlarging the buffer
exponentially by a constant factor (currently 1.5), to avoid excessively
frequent reallocations.

However, if you know the exact or approximate final length of the string, it is
even more efficient to allocate the required capacity upfront:

<!--
%TEST_SNIPPET
-->

```c
ss8str s;
ss8_init(&s);

ss8_reserve(&s, 40);  // Without changing length, enlarges buffer capacity.
ss8_copy_cstr(&s, "[INFO]");
ss8_cat_ch(&s, ' ');
ss8_cat_cstr(&s, "01:23:45");
ss8_cat_ch(&s, ' ');
ss8_cat_cstr(&s, "This is a log entry.");
ss8_cat_ch(&s, '\n');

// ...

ss8_destroy(&s);
```

Reserving space when the string is empty is most efficient because no existing
data needs to be copied (otherwise, the entire current capacity's worth of
bytes are copied, even if the string is shorter).

You can get the current capacity of an `ss8str` by calling `ss8_capacity()`,
which returns the maximum number of bytes the string can contain without
allocating new memory.

Operations that cause the string to become shorter do not free the resulting
unused memory. If you shorten a string by a large factor and want to ensure
that the unused memory is freed, you can call `ss8_shrink_to_fit()`. This may
make sense if a large number of such strings will be kept around for a long
time.

### Chaining calls

Most of the functions that take an `ss8str *` as the first argument and modify
the string also return the first argument. This can be used to chain multiple
function calls (although it becomes nearly unreadable beyond 2 or 3 calls).

<!--
%TEST_SNIPPET
-->

```c
char const *heading = "error: ", *message = strerror(ERANGE);
ss8str log_line;
ss8_cat_cstr(ss8_init_copy_cstr(&log_line, heading), message);
// ...
ss8_destroy(&log_line);
```

### Moving strings around without copying

You might have an `ss8str` contained in some data structure (say, an array or
linked list element). If you want to set that string to one that you have in a
local variable, but no longer need the local copy, you can do a swap to avoid
making a copy of the whole string:

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str s1, s2;
%SNIPPET_PROLOGUE ss8_init(&s1);
%SNIPPET_PROLOGUE ss8_init(&s2);
-->

```c
// Swaps the contents of the two strings s1 and s2:
ss8_swap(&s1, &s2);
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&s1);
%SNIPPET_EPILOGUE ss8_destroy(&s2);
-->

For `ss8_swap()`, the two strings must not be the same `ss8str` object, or else
undefined behavior will result. Also, the two `ss8str` objects must be valid
(initialized); use `ss8_init_move_destroy()` to swap an unititialized `ss8str`
with an initialized one.

When the operation is asymmetric, i.e., you want to move the value of `s2` into
`s1`, but do not care what `s2` holds afterwards, you can use `ss8_move()`:

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str s1, s2, s3, s4;
%SNIPPET_PROLOGUE ss8_init(&s1);
%SNIPPET_PROLOGUE ss8_init(&s2);
%SNIPPET_PROLOGUE ss8_init(&s3);
%SNIPPET_PROLOGUE ss8_init(&s4);
-->

```c
ss8_move(&s1, &s2);  // Moves value of s2 into s1
// s2 remains valid (must be destroyed later), but its value is now
// indeterminate

ss8_move_destroy(&s3, &s4);  // Moves value of s4 into s3
// s4 is destroyed and must be initialized before reuse
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&s1);
%SNIPPET_EPILOGUE ss8_destroy(&s2);
%SNIPPET_EPILOGUE ss8_destroy(&s3);
-->

There is also `ss8_init_move()` and `ss8_init_move_destroy()`.

### Getting substrings

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str dest, src, s;
%SNIPPET_PROLOGUE ss8_init(&dest);
%SNIPPET_PROLOGUE ss8_init(&src);
%SNIPPET_PROLOGUE ss8_init(&s);
%SNIPPET_PROLOGUE size_t start = 0, len = 0;
-->

```c
ss8_copy_substr(&dest, &src, start, len);
ss8_substr_inplace(&s, start, len);
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&dest);
%SNIPPET_EPILOGUE ss8_destroy(&src);
%SNIPPET_EPILOGUE ss8_destroy(&s);
-->

### Inserting, erasing, and replacing

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str dest, src;
%SNIPPET_PROLOGUE ss8_init(&dest);
%SNIPPET_PROLOGUE ss8_init(&src);
%SNIPPET_PROLOGUE char *cstr = 0, *buf = 0;
%SNIPPET_PROLOGUE size_t pos = 0, len = 0, buflen = 0, count = 0;
-->

```c
// Insert the given string or char(s) at the given position
ss8_insert(&dest, pos, &src);
ss8_insert_cstr(&dest, pos, cstr);
ss8_insert_bytes(&dest, pos, buf, buflen);
ss8_insert_ch(&dest, pos, 'c');
ss8_insert_ch_n(&dest, pos, 'c', count);

// Remove the given range
ss8_erase(&dest, pos, len);

// Replace the given range with the given string
ss8_replace(&dest, pos, len, &src);
ss8_replace_cstr(&dest, pos, len, cstr);
ss8_replace_bytes(&dest, pos, len, buf, buflen);
ss8_replace_ch(&dest, pos, len, 'c');
ss8_replace_ch_n(&dest, pos, len, 'c', count);
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&dest);
%SNIPPET_EPILOGUE ss8_destroy(&src);
-->

### Comparing strings

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str lhs, rhs, s, prefix, suffix, infix;
%SNIPPET_PROLOGUE ss8_init(&lhs);
%SNIPPET_PROLOGUE ss8_init(&rhs);
%SNIPPET_PROLOGUE ss8_init(&s);
%SNIPPET_PROLOGUE ss8_init(&prefix);
%SNIPPET_PROLOGUE ss8_init(&suffix);
%SNIPPET_PROLOGUE ss8_init(&infix);
%SNIPPET_PROLOGUE char *cstr = 0, *buf = 0;
%SNIPPET_PROLOGUE size_t len = 0;
-->

```c
// Return <0, ==0, or >0, as with strcmp() or memcmp():
ss8_cmp(&lhs, &rhs);
ss8_cmp_cstr(&lhs, cstr);
ss8_cmp_bytes(&lhs, buf, len);
ss8_cmp_ch(&lhs, 'c');

// Boolean check for equality:
ss8_equals(&lhs, &rhs);
ss8_equals_cstr(&lhs, cstr);
ss8_equals_bytes(&lhs, buf, len);
ss8_equals_ch(&lhs, 'c');

// Boolean check for prefix match:
ss8_starts_with(&s, &prefix);
ss8_starts_with_cstr(&s, cstr);
ss8_starts_with_bytes(&s, buf, len);
ss8_starts_with_ch(&s, 'c');

// Boolean check for suffix match:
ss8_ends_with(&s, &suffix);
ss8_ends_with_cstr(&s, cstr);
ss8_ends_with_bytes(&s, buf, len);
ss8_ends_with_ch(&s, 'c');

// Boolean check for substring match:
ss8_contains(&s, &infix);
ss8_contains_cstr(&s, cstr);
ss8_contains_bytes(&s, buf, len);
ss8_contains_ch(&s, 'c');
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&lhs);
%SNIPPET_EPILOGUE ss8_destroy(&rhs);
%SNIPPET_EPILOGUE ss8_destroy(&s);
%SNIPPET_EPILOGUE ss8_destroy(&prefix);
%SNIPPET_EPILOGUE ss8_destroy(&suffix);
%SNIPPET_EPILOGUE ss8_destroy(&infix);
-->

### Searching strings

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str haystack, needle, needles;
%SNIPPET_PROLOGUE ss8_init(&haystack);
%SNIPPET_PROLOGUE ss8_init(&needle);
%SNIPPET_PROLOGUE ss8_init(&needles);
%SNIPPET_PROLOGUE char *cstr = 0, *buf = 0;
%SNIPPET_PROLOGUE size_t start = 0, len = 0;
-->

```c
// Search forward from start; return position, or SIZE_MAX if not found:
ss8_find(&haystack, start, &needle);
ss8_find_cstr(&haystack, start, cstr);
ss8_find_bytes(&haystack, start, buf, len);
ss8_find_ch(&haystack, start, 'c');
ss8_find_not_ch(&haystack, start, 'c');

// Search backward from start; return position, or SIZE_MAX if not found:
ss8_rfind(&haystack, start, &needle);
ss8_rfind_cstr(&haystack, start, cstr);
ss8_rfind_bytes(&haystack, start, buf, len);
ss8_rfind_ch(&haystack, start, 'c');
ss8_rfind_not_ch(&haystack, start, 'c');

// Search forward for any char in 'needles':
ss8_find_first_of(&haystack, start, &needles);
ss8_find_first_of_cstr(&haystack, start, cstr);
ss8_find_first_of_bytes(&haystack, start, buf, len);

// Search forward for any char not in 'needles':
ss8_find_first_not_of(&haystack, start, &needles);
ss8_find_first_not_of_cstr(&haystack, start, cstr);
ss8_find_first_not_of_bytes(&haystack, start, buf, len);

// Search backward for any char in 'needles':
ss8_find_last_of(&haystack, start, &needles);
ss8_find_last_of_cstr(&haystack, start, cstr);
ss8_find_last_of_bytes(&haystack, start, buf, len);

// Search backward for any char not in 'needles':
ss8_find_last_not_of(&haystack, start, &needles);
ss8_find_last_not_of_cstr(&haystack, start, cstr);
ss8_find_last_not_of_bytes(&haystack, start, buf, len);
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&haystack);
%SNIPPET_EPILOGUE ss8_destroy(&needle);
%SNIPPET_EPILOGUE ss8_destroy(&needles);
-->

### Stripping characters off the ends

<!--
%TEST_SNIPPET COMPILE_ONLY
%SNIPPET_PROLOGUE ss8str s, chars;
%SNIPPET_PROLOGUE ss8_init(&s);
%SNIPPET_PROLOGUE ss8_init(&chars);
%SNIPPET_PROLOGUE char *cstr = 0, *buf = 0;
%SNIPPET_PROLOGUE size_t len = 0;
-->

```c
// Remove any chars in 'chars' from either end of s:
ss8_strip(&s, &chars);
ss8_strip_cstr(&s, cstr);
ss8_strip_bytes(&s, buf, len);
ss8_strip_ch(&s, 'c');

// Remove any chars in 'chars' from the beginning of s:
ss8_lstrip(&s, &chars);
ss8_lstrip_cstr(&s, cstr);
ss8_lstrip_bytes(&s, buf, len);
ss8_lstrip_ch(&s, 'c');

// Remove any chars in 'chars' from the end of s:
ss8_rstrip(&s, &chars);
ss8_rstrip_cstr(&s, cstr);
ss8_rstrip_bytes(&s, buf, len);
ss8_rstrip_ch(&s, 'c');
```

<!--
%SNIPPET_EPILOGUE ss8_destroy(&s);
%SNIPPET_EPILOGUE ss8_destroy(&chars);
-->

### Formatting strings

```c
// Format string
ss8_sprintf(&dest, "fmt", ...);
ss8_snprintf(&dest, maxlen, "fmt", ...);

// Append formatted string
ss8_cat_sprintf(&dest, "fmt", ...);
ss8_cat_snprintf(&dest, maxlen, "fmt", ...);

// Versions taking va_list
ss8_vsprintf(&dest, "fmt", args);
ss8_vsnprintf(&dest, maxlen, "fmt", args);
ss8_cat_vsprintf(&dest, "fmt", args);
ss8_cat_vsnprintf(&dest, maxlen, "fmt", args);
```

These functions internally call `vsnprintf()`, so the format string has the
same meaning as the `printf()` family of functions provided by the standard
library.

In particular, any `"%s"` format specifier requires a corresponding standard
null-terminated string, so you need to use `ss8_const_cstr()` if printing an
`ss8str`:

<!--
%TEST_SNIPPET
-->

```c
ss8str warning, dest;
ss8_init_copy_cstr(&warning, "I'm not a C string");
ss8_init(&dest);
ss8_sprintf(&dest, "warning: %s (%d)", ss8_const_cstr(&warning), 42);
// ...
ss8_destroy(&dest);
ss8_destroy(&warning);
```

## Building Ssstr

If using as a header-only library, all you need is the file `ss8str.h`.

You can use the build system to build and run the unit tests, or install the
header and manual pages locally.

Requirements: [Python](https://python.org), [Meson](https://mesonbuild.com),
and [Ninja](https://ninja-build.org/).

```sh
meson setup builddir
cd builddir
ninja test
ninja install  # Install ss8str.h and the manual pages
ninja htmlman  # Generate the HTML manual pages (requires groff)
```

See the [Meson documentation](https://mesonbuild.com/Commands.html) or
`meson --help` for how to set the install location (prefix) and other details.

### Test coverage

Coverage information can be generated as follows (requires gcovr or similar):

```sh
cd builddir
meson configure -Db_coverage=true
ninja clean
ninja
./test_ssstr_no_asserts
ninja coverage-html
# Now open meson-logs/coveragereport/index.html
```

I try to maintain near-perfect coverage for the functions in `ss8str.h`, with
the exception of assertions, codepaths leading to panics, compile-time disabled
code, and (very few) codepaths that cannot be tested without buffers sized near
`INT_MAX` or larger. But unit tests should strive to test as many edge cases as
possible, not merely exercise every line of code.

## Customization

A few aspects of **Ssstr** can be customized by defining preprocessor macros
_before_ including `ss8str.h`. All of them are advanced features.

### Avoiding `static inline` functions

By default, all **Ssstr** functions are `static inline`, so that simply
including `ss8str.h` is all you need to do. However, if you include the header
from many translation units (source files), each will get their own copy of the
**Ssstr** functions, which may not be desirable in projects where small binary
size is important.

If the macro `SSSTR_USE_NONSTATIC_INLINE` is defined, **Ssstr** functions will
be defined as plain `inline`, so that duplicate copies of the functions will
not be generated. Because of the way
[C inline functions](https://en.cppreference.com/w/c/language/inline) work, the
macro `SSSTR_DEFINE_EXTERN_INLINE` must also be defined in one (and only one)
of the translation units.

### Customizing memory allocation

By default, **Ssstr** uses the standard library functions `malloc()`,
`realloc()`, and `free()` to manage dynamic storage. If you want **Ssstr** to
instead use a different allocator, you can define the function-style macros
`SSSTR_MALLOC(size)`, `SSSTR_REALLOC(ptr, size)`, and `SSSTR_FREE(ptr)`.

No **Ssstr** function calls `SSSTR_MALLOC()` or `SSSTR_REALLOC()` with a `size`
of zero or a null `ptr`, so your definitions need not handle these edge cases
in any specific manner. Also, **Ssstr** always checks the return value for
`NULL` (see below on error handling).

### Customizing run-time assertions

**Ssstr** calls the standard `assert()` macro if there is a precondition
violation (that is, programming error in user code). You can replace `assert()`
by defining `SSSTR_ASSERT(condition)`.

To disable assertions, you should define `NDEBUG`; there is no need to define
`SSSTR_ASSERT` just for this purpose.

`SSSTR_ASSERT()` must not return. It is _not_ safe to call `longjmp()` from
inside `SSSTR_ASSERT()`.

### Enabling more thorough run-time checks

For performance reasons, not all detectable precondition violations are checked
by default. In a debug build, you can enable extra assertions and debugging
features by defining the macro `SSSTR_EXTRA_DEBUG`.

Some of the violations that may be caught include null pointers passed as
arguments, overlapping buffers (where not allowed), and (with some luck)
corrupted `ss8str` objects.

In addition, when built with `SSSTR_EXTRA_DEBUG` defined, the portion of a
string extended by `ss8_set_len()` or `ss8_grow_len()` is filled with `'~'`,
and the right-hand-side of `ss8_move()` or `ss8_init_move()` is set to the
string `"!!! MOVED OUT SS8STR !!!"`. These behaviors are intended to increase
the likelihood of spotting bugs due to erroneous access to indeterminate data.

### Customizing error handling

By default, if memory allocation fails or if the size of a result is computed
to be larger than `size_t` can express, **Ssstr** will print a message to
`stderr` and call `abort()`.

You can customize this behavior by defining the macros
`SSSTR_OUT_OF_MEMORY(bytes)` and `SSSTR_SIZE_OVERFLOW()`.

`SSSTR_OUT_OF_MEMORY()` will be called if `SSSTR_MALLOC()` or `SSSTR_REALLOC()`
(or their default equivalents) return `NULL`. The number of bytes that
**Ssstr** attempted to allocate is passed as the argument.

`SSSTR_SIZE_OVERFLOW()` will be called if the result of a string operation
(such as concatenation, insertion, or replacement) would have a length greater
than `SIZE_MAX - 1`.

It is meant to be safe to call `longjmp()` from inside these 2 macros, but this
has not been tested.

Note that the string formatting functions `ss8_[cat_][v]s[n]printf()` can (at
least in theory) encounter additional errors that will lead to a call to
`abort()`. These include the result of string formatting being greater than or
equal to `INT_MAX` bytes or `vs[n]printf()` returning a negative number for
some other reason. Handling of these is not customizable. Programs that want
water-tight (and strictly platform-independent) string formatting should
probably use a specialized library rather than depending on these convenience
functions which are meant to prevent the more common mistakes when calling
`s[n]printf()` but inherit some of the limitations of the latter.

## Memory layout

An `ss8str` occupies 32 bytes on 64-bit platforms. Depending on the current
capacity (not including null terminator), one of two layouts is used. Capacity
is never less than 31.

Small string (capacity = 31; length \<= 31):

```text
+----------------------------------------------------+-----+
| Bytes 0-30: string buffer (always null-terminated) |  31 |
+----------------------------------------------------+-----+
| H e l l o ,   W o r l d ! \0                       | (*) |
+----------------------------------------------------+-----+
```

Byte 31 (`(*)`) is set to `31 - length`, which doubles as the null terminator
when the length is exactly 31. (This idea comes from
[`folly::fbstring`](https://github.com/facebook/folly/blob/main/folly/docs/FBString.md)
([video](https://www.youtube.com/watch?v=kPR8h4-qZdk&t=8s)), although that
implementation uses a 24 byte layout, requiring endian-specific manipulation.)

Large string (capacity > 31; any length):

```text
+-------------+-------------+-------------+-----------+----+
|  Bytes 0-7  |     8-15    |    16-23    |   24-30   | 31 |
+-------------+-------------+-------------+-----------+----+
|     ptr     |    length   |   bufsize   |  padding  | FF |
+-------------+-------------+-------------+-----------+----+
```

Dynamically allocated memory at `ptr` stores the null-terminated string.
Capacity is `bufsize - 1` (for null terminator). Byte 31 always contains `0xFF`
to distinguish from small strings.

On 32-bit platforms, an `ss8str` occupies 16 bytes and the capacity is never
less than 15.

## Versioning

**Ssstr** uses [Semantic Versioning](https://semver.org/). The API, for
versioning purposes, consists of the functions and data types documented in the
manual pages, plus the customization macros documented above.

ABI compatibility will be maintained more strictly: the memory layout of the
`ss8str` object will not change on a given platform (if it ever does, the type,
header, and library will be renamed). This will apply to any version following
the 0.1.0 release; until then, things might change.

(Note, however, that true binary compatibility requires that all copies of
**Ssstr** code that exchange (mutable) `ss8str` objects are built with
compatible C runtimes, use the same dynamic storage heap, and have mutually
compatible malloc customizations, if any. For this reason, you might want to
stick to traditional C string interfaces at your major module boundaries, or at
least limit the exchange of `ss8str` to `const ss8str *`, so that memory
allocation and deallocation is limited to one side of the boundary.)

## License

**Ssstr** is distributed under the MIT license. See `LICENSE.txt`.

## Other simple string libraries for C

- [SDS](https://github.com/antirez/sds) (Simple Dynamic Strings)
- [bstring](http://bstring.sourceforge.net/)
  ([GitHub](https://github.com/websnarf/bstrlib)) (Better String Library)
- utstring, part of [uthash](https://troydhanson.github.io/uthash/)
  ([GitHub](https://github.com/troydhanson/uthash))

It is difficult to search for string libraries in C (too much noise), so I
wouldn't be surprised if there are other good or notable ones that I am not
aware of. Also, several larger frameworks (e.g.
[GLib](https://docs.gtk.org/glib/struct.String.html)) have string types.
