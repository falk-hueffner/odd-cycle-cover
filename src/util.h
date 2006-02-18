#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdio.h>

/// Return a malloced copy of the string S (like strdup from POSIX).
char *dup_str(const char *s);

/** Similar to GNU getline(3) (but slightly different interface).
    Read an entire line from STREAM, storing it into *LINEPTR without
    trailing newline. LINEPTR must be NULL or malloced to *N bytes. If
    the buffer is not sufficient, it will be realloced, and *LINEPTR
    and *N will be updated to reflect the new buffer. Returns true on
    success, or false on EOF or error.  */
bool get_line(char **lineptr, size_t *n, FILE *stream);

/// A string literal containing all whitespace characters.
#define WHITESPACE " \f\r\t\v"

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#endif	// UTIL_H
