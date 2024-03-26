#include <string.h>
#include <stdlib.h>

char * strdup(const char * s1)
{
    char *s2 = malloc(strlen(s1) + 1);
    if (!s2)
        return NULL;
    memcpy(s2, s1, strlen(s1) + 1);
    return s2;
}

char * strndup(const char * s, size_t size)
{
    char * p = malloc(size + 1);
    if (!p)
        return NULL;

    char * q = p;
    while (size-- && *s)
        *q++ = *s++;
    *q = 0;

    return p;
}
