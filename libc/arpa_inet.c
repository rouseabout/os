#include <arpa/inet.h>
#include <stdio.h>
#include <syslog.h>

uint32_t htonl(uint32_t hostlong)
{
    return __builtin_bswap32(hostlong);
}

uint16_t htons(uint16_t hostshort)
{
    return __builtin_bswap16(hostshort);
}

in_addr_t inet_addr(const char *cp)
{
    syslog(LOG_DEBUG, "libc: inet_addr");
    return 0; //FIXME:
}

static char ntoa_buf[16];
char * inet_ntoa(struct in_addr in)
{
    snprintf(ntoa_buf, sizeof(ntoa_buf), "%d.%d.%d.%d", (in.s_addr >> 24) & 0xFF, (in.s_addr >> 16) & 0xFF, (in.s_addr >> 8) & 0xFF, in.s_addr & 0xFF);
    return ntoa_buf;
}

const char * inet_ntop(int af, const void * src, char * dst, socklen_t size)
{
    syslog(LOG_DEBUG, "libc: inet_ntop");
    return 0; //FIXME
}

int inet_pton(int af, const char * src, void * dst)
{
    syslog(LOG_DEBUG, "libc: inet_pton");
    return 0; //FIXME:
}

uint32_t ntohl(uint32_t netlong)
{
    return __builtin_bswap32(netlong);
}

uint16_t ntohs(uint16_t netshort)
{
    return __builtin_bswap16(netshort);
}
