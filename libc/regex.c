#include <regex.h>
#include <errno.h>
#include <syslog.h>

int regcomp(regex_t * preg, const char * pattern, int cflags)
{
    syslog(LOG_DEBUG, "libc: regcomp");
    errno = ENOSYS;
    return -1; //FIXME
}

size_t regerror(int errcode, const regex_t * preg, char * errbuf, size_t errbuf_size)
{
    return 0;
}

int regexec(const regex_t * preg, const char * string, size_t nmatch, regmatch_t pmatch[1], int eflags)
{
    return 0;
}

void regfree(regex_t * preg)
{
}
