#include <utime.h>
#include <os/syscall.h>

MK_SYSCALL2(int, utime, OS_UTIME, const char *, const struct utimbuf *)
