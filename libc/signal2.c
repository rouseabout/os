#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <os/syscall.h>
#include <syslog.h>

MK_SYSCALL2(int, kill, OS_KILL, pid_t, int)

int raise(int sig)
{
    return kill(getpid(), sig);
}

int pthread_sigmask(int how, const sigset_t * set, sigset_t * oset)
{
    return 0; //FIXME:
}

static MK_SYSCALL4(int, rt_sigaction, OS_RT_SIGACTION, int, const struct sigaction *, struct sigaction *, size_t)

int sigaction(int sig, const struct sigaction * act, struct sigaction * oact)
{
    return rt_sigaction(sig, act, oact, sizeof(sigset_t));
}

int siginterrupt(int sig, int flag)
{
    syslog(LOG_DEBUG, "libc: siginterrupt");
    errno = ENOSYS;
    return -1;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    struct sigaction act, oldact;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigaction(signum, &act, &oldact);
#if 0
    if ((oldact.sa_flags & SA_SIGINFO) && oldact.sa_handler != SIG_DFL && oldact.sa_handler != SIG_IGN)
        return SIG_DFL;
#endif
    return oldact.sa_handler;
}

int sigpending(sigset_t *set)
{
    return 0; //FIXME:
}

int sigprocmask(int how, const sigset_t * set, sigset_t * oset)
{
    return 0; //FIXME:
}

int sigsuspend(const sigset_t * sigmask)
{
    return 0; //FIXME:
}

int sigwait(const sigset_t * set, int * sig)
{
    return 0; //FIXME:
}
