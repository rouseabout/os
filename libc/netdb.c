#include <netdb.h>
#include <stddef.h>
#include <syslog.h>

int h_errno = 0;

void freeaddrinfo(struct addrinfo * ai)
{
}

const char * gai_strerror(int ecode)
{
    switch (ecode) {
    case EAI_MEMORY: return "EAI_MEMORY";
    case EAI_NODATA: return "EAI_NODATA";
    case EAI_SERVICE: return "EAI_SERVICE";
    case EAI_SOCKTYPE: return "EAI_SOCKTYPE";
    case EAI_SYSTEM: return "EAI_SYSTEM";
    }
    return "UNKNOWN";
}

int getaddrinfo(const char * nodename, const char * servname, const struct addrinfo * hints, struct addrinfo ** res)
{
    syslog(LOG_DEBUG, "libc: getaddrinfo");
    return EAI_SYSTEM; //FIXME:
}

struct hostent * gethostbyaddr(const void * addr, socklen_t len, int type)
{
    syslog(LOG_DEBUG, "libc: gethostbyaddr");
    return NULL; //FIXME:
}

struct hostent * gethostbyname(const char * name)
{
    syslog(LOG_DEBUG, "libc: gethostbyname");
    return NULL; //FIXME:
}

int getnameinfo(const struct sockaddr * sa, socklen_t salen, char * node, socklen_t nodelen, char * service, socklen_t servicelen, int flags)
{
    syslog(LOG_DEBUG, "libc: getnameinfo");
    return EAI_SYSTEM; //FIXME:
}

struct servent * getservbyname(const char * name, const char * proto)
{
    syslog(LOG_DEBUG, "libc: getservbyname");
    return NULL; //FIXME:
}
