#include <sys/wait.h>
#include <os/syscall.h>

pid_t wait(int * stat_loc)
{
    return waitpid((pid_t)-1, stat_loc, 0);
}

MK_SYSCALL3(pid_t, waitpid, OS_WAITPID, pid_t, int *, int)
