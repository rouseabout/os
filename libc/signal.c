#include <signal.h>

int sigaddset(sigset_t *set, int signo)
{
#if 0
    if (signo <= 0 || signo >= 64) {
        errno = EINVAL;
        return -1;
    }
#endif
    *set |= 1 << signo;
    return 0;
}

int sigdelset(sigset_t * set, int signo)
{
    //FIXME: errno
    *set &= ~(1 << signo);
    return 0;
}

int sigemptyset(sigset_t * set)
{
    *set = 0;
    return 0;
}

int sigfillset(sigset_t * set)
{
    *set = 0xFFFFFFFFUL;
    return 0;
}

int sigismember(const sigset_t *set, int signo)
{
    //FIXME: errnp
    return !!(*set & (1 << signo));
}
