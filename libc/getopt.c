#include <getopt.h>
#include <syslog.h>

int getopt_long(int argc, char * const argv[], const char * optstring, const struct option * longopts, int * longindex)
{
    syslog(LOG_DEBUG, "libc: getopt_long");
    return -1;
}
