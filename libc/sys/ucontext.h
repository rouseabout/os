#ifndef SYS_UCONTEXT_H
#define SYS_UCONTEXT_H

#include <signal.h>

enum { /* not posix, system specific */
    REG_EIP = 0,
    REG_EBP,
    SYS_UCONTEXT_NB_REGS
};

typedef struct {
    int gregs[SYS_UCONTEXT_NB_REGS]; /* system specific */
} mcontext_t;

typedef struct ucontext_t ucontext_t;

struct ucontext_t {
    ucontext_t * uc_link;
    sigset_t uc_sigmask;
    stack_t uc_stack;
    mcontext_t  uc_mcontext;
};

#endif /* SYS_UCONTEXT_H */
