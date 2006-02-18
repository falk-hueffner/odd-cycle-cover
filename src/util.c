#include <stdlib.h>
#include <string.h>

#include "util.h"

char *dup_str(const char *s) {
    size_t len = strlen(s) + 1;
    char *t = malloc(len);
    memcpy(t, s, len);
    return t;
}
