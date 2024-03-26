#ifndef NETDB_H
#define NETDB_H

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hostent {
    char * h_name;
    char ** h_aliases;
    int h_addrtype;
    int h_length;
    char ** h_addr_list;
#define h_addr h_addr_list[0]
};

struct servent {
    char * s_name;
    char ** s_aliases;
    int s_port;
    char * s_proto;
};

struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr * ai_addr;
    char * ai_canonname;
    struct addrinfo * ai_next;
};

void freeaddrinfo(struct addrinfo *);
const char * gai_strerror(int);
int getaddrinfo(const char *, const char *, const struct addrinfo *, struct addrinfo **);
struct hostent * gethostbyaddr(const void *, socklen_t, int);
struct hostent * gethostbyname(const char *);
int getnameinfo(const struct sockaddr *, socklen_t, char *, socklen_t, char *, socklen_t, int);
struct servent * getservbyname(const char *, const char *);

extern int h_errno;

#define AI_ADDRCONFIG 0x1
#define AI_CANONNAME 0x2
#define AI_NUMERICHOST 0x4
#define AI_PASSIVE 0x8

#define NI_NUMERICHOST 0x1
#define NI_NUMERICSERV 0x2

#define HOST_NOT_FOUND 1
#define NO_DATA 2
#define NO_RECOVERY 3
#define TRY_AGAIN 4

#define IPPORT_RESERVED 1024

#define EAI_MEMORY -1
#define EAI_NODATA -2
#define EAI_SERVICE -3
#define EAI_SOCKTYPE -4
#define EAI_SYSTEM -5

#ifdef __cplusplus
}
#endif

#endif /* NETDB_H */
