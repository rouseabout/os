#include <sys/times.h>
#include <syslog.h>
#include <time.h>

clock_t times(struct tms * buffer)
{
    syslog(LOG_DEBUG, "libc: times");
    buffer->tms_utime = 0;
    buffer->tms_stime = 0;
    buffer->tms_cutime = 0;
    buffer->tms_cstime = 0;
    return time(NULL); //FIXME:
}
