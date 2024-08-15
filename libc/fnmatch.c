#include <fnmatch.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

int fnmatch(const char *pattern, const char *string, int flags)
{
    syslog(LOG_DEBUG, "libc: fnmatch");
    return strstr(string, pattern) ? 0 : FNM_NOMATCH; //FIXME:
}
