#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char *dup_str(const char *s) {
    size_t len = strlen(s) + 1;
    char *t = malloc(len);
    memcpy(t, s, len);
    return t;
}

bool get_line(char **lineptr, size_t *n, FILE *stream) {
    if (!*lineptr) {
	*n = 128;
	*lineptr = malloc(*n);
    }

    bool success = false;
    size_t len = 0;
    while (fgets(*lineptr + len, *n - len, stream)) {
	success = true;
	len += strlen(*lineptr + len);
	if ((*lineptr)[len - 1] == '\n') {
	    if (len > 1 && (*lineptr)[len - 2] == '\r')	// DOS newline
		len--;
	    (*lineptr)[len - 1] = '\0';
	    break;
	}
	if (feof(stream))	// last line in file lacks a newline
	    break;
	*n *= 2;
	*lineptr = realloc(*lineptr, *n);
    }
    return success;
}
