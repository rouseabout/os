#ifndef NETINET_IN_H
#define NETINET_IN_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

#define IPPORT_USERRESERVED 5000

#define IPPROTO_IP 0
#define IPPROTO_UDP 1
#define IPPROTO_TCP 2

#define INADDR_ANY 0
#define INADDR_BROADCAST 0xFFFFFFFF
#define INADDR_LOOPBACK	0x7F000001 /* not posix */

#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[1];
};

struct in6_addr {
    uint8_t s6_addr[16];
};

struct sockaddr_in6 {
    sa_family_t sin6_family;
    in_port_t sin6_port;
    uint32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t sin6_scope_id;

};

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

#define IN6ADDR_ANY_INIT in6addr_any
#define IN6ADDR_LOOPBACK_INIT in6addr_loopback

#define IP_TOS 100
#define IP_TTL 101
#define IP_MULTICAST_IF 102
#define IP_MULTICAST_LOOP 103
#define IP_MULTICAST_TTL 104
#define IP_OPTIONS 105

#ifdef __cplusplus
}
#endif

#endif /* NETINET_IN_H */
