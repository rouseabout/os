#include <sys/utsname.h>
#include <os/syscall.h>

MK_SYSCALL1(int, uname, OS_UNAME, struct utsname *)
