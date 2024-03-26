#include <grp.h>
#include <errno.h>
#include <syslog.h>

void endgrent()
{
}

struct group * getgrent()
{
    syslog(LOG_DEBUG, "getgrent");
    errno = ENOSYS;
    return NULL;
}

struct group * getgrgid(gid_t gid)
{
    syslog(LOG_DEBUG, "getgrgid");
    errno = ENOSYS;
    return NULL;
}

struct group * getgrnam(const char * name)
{
    syslog(LOG_DEBUG, "getgrgid");
    errno = ENOSYS;
    return NULL;
}

void setgrent()
{
}
