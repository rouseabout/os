#include <stdarg.h>
#include <fcntl.h>
#include <os/syscall.h>

int creat(const char * path, mode_t mode)
{
    return open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);
}

static MK_SYSCALL3(int, sys_open, OS_OPEN, const char *, int, mode_t)
int open(const char * path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return sys_open(path, flags, mode);
}

static MK_SYSCALL3(int, sys_fcntl, OS_FCNTL, int, int, int)
int fcntl(int fildes, int cmd, ...)
{
    int data = 0;
    if (cmd == F_SETFL || cmd == F_SETFD || cmd == F_DUPFD) {
        va_list args;
        va_start(args, cmd);
        data = va_arg(args, int);
        va_end(args);
    }

    return sys_fcntl(fildes, cmd, data);
}
