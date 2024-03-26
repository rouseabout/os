#include <libgen.h>
#include <string.h>
#include <bsd/string.h>
#include <limits.h>

char * basename(char * path)
{
    static char s[PATH_MAX];
    strlcpy(s, path, sizeof(s));

    if (!strcmp(s, "/"))
        return s;

    if (s[strlen(s) - 1] == '/')
        s[strlen(s) - 1] = 0;

    char * p = strrchr(s, '/');
    return p ? p + 1 : s;
}


char * dirname(char * path)
{
    static char s[PATH_MAX];
    static char dot[] = ".";
    static char slash[] = "/";
    strlcpy(s, path, sizeof(s));

    if (!strcmp(s, "/"))
        return s;

    if (s[strlen(s) - 1] == '/')
        s[strlen(s) - 1] = 0;

    char * p = strrchr(s, '/');
    if (!p)
        return dot;

    if (p == s)
        return slash;

    *p = 0;
    return s;
}
