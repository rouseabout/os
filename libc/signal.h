#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

#define SIGHUP 1
#define SIGINT 2
#define SIGILL 4
#define SIGABRT 6
#define SIGQUIT 3
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGUSR2 12
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGWINCH 28
#define SIGXCPU 29
#define SIGTRAP 5 /* not posix */

#define NSIG 64  /* not posix */

#define SIG_BLOCK 0
#define SIG_SETMASK 1
#define SIG_UNBLOCK 2

#define SA_NOCLDSTOP 0x1
#define SA_RESETHAND 0x2
#define SA_RESTART 0x4
#define SA_SIGINFO 0x8
#define SA_ONSTACK 0x10
#define SA_NODEFER 0x20

#define FPE_INTDIV 1
#define FPE_FLTDIV 2

typedef void (* sighandler_t)(int);
typedef int sig_atomic_t;

typedef struct {
    void * ss_sp;
    size_t ss_size;
    int ss_flags;
} stack_t;

typedef unsigned long long sigset_t;

union sigval {
    int sigval_int;
    void * sigval_ptr;
};

typedef struct {
    int si_signo;
    int si_code;
    int si_errno;
    pid_t si_pid;
    uid_t si_uid;
    void * si_addr;
    int si_status;
    long si_band;
    union sigval si_value;
} siginfo_t;

struct sigaction {
    sighandler_t sa_handler;
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_sigaction)(int, siginfo_t *, void *);
};

int kill(pid_t, int);
int pthread_sigmask(int, const sigset_t *, sigset_t *);
int raise(int);
int sigaddset(sigset_t *, int);
int sigaction(int, const struct sigaction *, struct sigaction *);
int sigdelset(sigset_t *, int);
int sigemptyset(sigset_t *);
int sigfillset(sigset_t *);
int siginterrupt(int, int);
int sigismember(const sigset_t *, int);
sighandler_t signal(int, sighandler_t);
int sigpending(sigset_t *set);
int sigprocmask(int, const sigset_t *, sigset_t *);
int sigsuspend(const sigset_t *);
int sigwait(const sigset_t * set, int * sig);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_H */
