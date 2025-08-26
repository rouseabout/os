#include <sys/wait.h>
#include <os/syscall.h>

pid_t wait(int * stat_loc)
{
    return waitpid((pid_t)-1, stat_loc, 0);
}

#if defined(ARCH_x86_64)
static MK_SYSCALL4(pid_t, wait4, OS_WAITPID, pid_t, int *, int, void *)
pid_t waitpid(pid_t pid, int * stat_loc, int options)
{
    return wait4(pid, stat_loc, options, NULL);
}
#else
MK_SYSCALL3(pid_t, waitpid, OS_WAITPID, pid_t, int *, int)
#endif
