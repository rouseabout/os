#include <strings.h>
#include <ctype.h>

int strcasecmp(const char * s1, const char * s2)
{
    int ret = 0;
    while (!(ret = toupper(*s1) - toupper(*s2)) && *s1 && *s2) {
        s1++;
        s2++;
    }
    return ret;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    int ret = 0;
    while (n && !(ret = toupper(*s1) - toupper(*s2)) && *s1 && *s2) {
        s1++;
        s2++;
        n--;
    }
    return ret;
}
