#include <sys/wait.h>
#include <os/syscall.h>

MK_SYSCALL3(pid_t, waitpid, OS_WAITPID, pid_t, int *, int)
