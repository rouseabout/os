#include <sys/time.h>
#include <time.h>
#include <os/syscall.h>

MK_SYSCALL2(int, getitimer, OS_GETITIMER, int, struct itimerval *)

int gettimeofday(struct timeval * tp, void * tzp)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tp->tv_sec = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / 1000;
    return 0;
}

MK_SYSCALL3(int, setitimer, OS_SETITIMER, int, const struct itimerval *, struct itimerval *)

int utimes(const char *path, const struct timeval times[2])
{
    return -1;
    errno = ENOSYS;
}
