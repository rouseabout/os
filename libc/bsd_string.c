#include <bsd/string.h>
#include <string.h>

size_t strlcat(char *dst, const char *src, size_t size)
{
    int n = strlen(dst);
    strlcpy(dst + n, src, size - n);
    return strlen(dst);
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t i;
    for (i = 0; i < size - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = 0;
    return i;
}
