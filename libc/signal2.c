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

#if defined(ARCH_x86_64)
static MK_SYSCALL1(int, rt_sigreturn, OS_RT_SIGRETURN, unsigned long);
static void restorer()
{
    rt_sigreturn(0);
}
#endif

sighandler_t signal(int signum, sighandler_t handler)
{
    struct sigaction act, oldact;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigaddset(&act.sa_mask, signum);
    act.sa_flags = SA_RESTART;
#if defined(ARCH_x86_64)
    act.sa_flags |= SA_RESTORER;
    act.sa_restorer = restorer;
#endif
    if (sigaction(signum, &act, &oldact) < 0)
       return SIG_ERR;
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
