#include <fnmatch.h>
#include <stdio.h>
#include <syslog.h>

int fnmatch(const char *pattern, const char *string, int flags)
{
    syslog(LOG_DEBUG, "libc: fnmatch");
    return 1; //FIXME:
}
