#include <time.h>
#include <string.h>
#include <bsd/string.h>
#include <os/syscall.h>
#include <syslog.h>

int daylight = 0;
long timezone = 0;
char * tzname[2] = {"std", "dst" };

char * asctime_r(const struct tm * timeptr, char * buf)
{
    strftime(buf, 26, "%a %b %e %H:%M:%S %Y", timeptr);
    return buf;
}

clock_t clock()
{
    return (clock_t)-1;
}

int clock_getres(clockid_t clock_id, struct timespec * res)
{
    res->tv_nsec = 1;
    res->tv_sec = 0;
    return 0;
}

MK_SYSCALL2(int, clock_gettime, OS_CLOCK_GETTIME, clockid_t, struct timespec *)

char * ctime_r(const time_t * clock, char * buf)
{
    return asctime_r(localtime(clock), buf);
}

MK_SYSCALL2(int, nanosleep, OS_NANOSLEEP, const struct timespec *, struct timespec *)

time_t time(time_t * tloc)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (tloc)
        *tloc = ts.tv_sec;
    return ts.tv_sec;
}

void tzset()
{
    syslog(LOG_DEBUG, "libc: tzset");
}
