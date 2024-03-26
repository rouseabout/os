#include <syslog.h>
#include <stdio.h>
#include <os/syscall.h>

void closelog(void)
{
}

int setlogmask(int maskpri)
{
    return 0;
}

static MK_SYSCALL2(int, sys_syslog, OS_SYSLOG, int, char *)
void syslog(int priority, const char *message, ... /* arguments */)
{
    char buf[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, sizeof(buf), message, args);
    va_end(args);
    sys_syslog(priority, buf);
}
