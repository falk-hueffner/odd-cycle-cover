#ifndef UTIL_H
#define UTIL_H

/// Return a malloced copy of the string S (like strdup from POSIX).
char *dup_str(const char *s);

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#endif	// UTIL_H
