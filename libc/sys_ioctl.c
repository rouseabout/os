#include <sys/ioctl.h>
#include <errno.h>
#include <os/syscall.h>
#include <stdarg.h>

static MK_SYSCALL3(int, sys_ioctl, OS_IOCTL, int, int, void *)
int ioctl(int fildes, int request, ... /* arg */)
{
    va_list args;
    va_start(args, request);
    int ret = sys_ioctl(fildes, request, va_arg(args, void *));
    va_end(args);
    return ret;
}
