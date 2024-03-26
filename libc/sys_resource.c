#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>

int getrlimit(int resource, struct rlimit * rlp)
{
    switch(resource) {
    case RLIMIT_NOFILE:
        rlp->rlim_cur = OPEN_MAX;
        rlp->rlim_max = OPEN_MAX;
        return 0;
    }
    syslog(LOG_DEBUG, "libc: getrlimit resource=%d", resource);
    rlp->rlim_cur = RLIM_INFINITY;
    rlp->rlim_max = RLIM_INFINITY;
    return 0;
}

int getrusage(int who, struct rusage * r_usage)
{
    memset(r_usage, 0, sizeof(struct rusage));
    return 0;
}

int setrlimit(int resource, const struct rlimit * rlp)
{
    return 0;
}
