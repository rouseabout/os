#include <sys/select.h>
#include <os/syscall.h>

MK_SYSCALL5(int, select, OS_SELECT, int, fd_set *, fd_set *, fd_set *, struct timeval *)

int pselect(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, const struct timespec * timeout, const sigset_t * sigmask)
{
    struct timeval tv;
    if (timeout) {
        tv.tv_sec = timeout->tv_sec;
        tv.tv_usec = timeout->tv_nsec / 1000;
    };
    return select(nfds, readfds, writefds, errorfds, timeout ? &tv : NULL);
}
