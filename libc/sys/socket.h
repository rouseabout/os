#ifndef SYS_SOCKET_H
#define SYS_SOCKET_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AF_UNSPEC 0
#define PF_UNSPEC AF_UNSPEC
#define AF_UNIX 1
#define PF_UNIX AF_UNIX
#define AF_INET 2
#define PF_INET AF_INET
#define AF_INET6 10
#define PF_INET6 AF_INET6

#define MSG_PEEK 1

#define SOL_SOCKET 1

#define SO_ACCEPTCONN 0
#define SO_BROADCAST 1
#define SO_DEBUG 2
#define SO_DONTROUTE 3
#define SO_ERROR 4
#define SO_KEEPALIVE 5
#define SO_LINGER 6
#define SO_OOBINLINE 7
#define SO_RCVBUF 8
#define SO_REUSEADDR 9
#define SO_SNDBUF 10
#define SO_TYPE 11

#define SOMAXCONN 4096

#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3
#define SOCK_SEQPACKET 5

#define SCM_RIGHTS 1

#define SHUT_RD 1
#define SHUT_WR 2
#define SHUT_RDWR 3

typedef unsigned short sa_family_t;
typedef unsigned int socklen_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[32];
};

struct sockaddr_storage {
    sa_family_t ss_family;
    char sa_data[32];
};

struct msghdr {
    void * msg_name;
    socklen_t msg_namelen;
    struct iovec * msg_iov;
    int msg_iovlen;
    void * msg_control;
    socklen_t msg_controllen;
    int msg_flags;
};

struct cmsghdr {
    socklen_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

struct linger {
    int l_onoff;
    int l_linger;
};

#define CMSG_DATA(x) ((struct cmsghdr *)(x) + 1)
#define CMSG_FIRSTHDR(mhdr) ((struct cmsghdr *)0)
#define CMSG_NXTHDR(mhdr, cmsg) ((struct cmsghdr *)0)
#define CMSG_LEN(len) (sizeof(struct cmsghdr) + (len))
#define CMSG_SPACE(len) (sizeof(struct cmsghdr) + (len))

int accept(int, struct sockaddr *, socklen_t *);
int bind(int, const struct sockaddr *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int getpeername(int, struct sockaddr *, socklen_t *);
int listen(int, int);
int getsockname(int, struct sockaddr *, socklen_t *);
int getsockopt(int, int, int, void *, socklen_t *);
ssize_t recv(int, void *, size_t, int);
ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t recvmsg(int, struct msghdr *, int);
ssize_t send(int, const void *, size_t, int);
ssize_t sendmsg(int, const struct msghdr *, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
int setsockopt(int, int, int, const void *, socklen_t);
int shutdown(int, int);
int socket(int, int, int);
int socketpair(int, int, int, int[2]);

#ifdef __cplusplus
}
#endif

#endif /* SYS_SOCKET_H */
