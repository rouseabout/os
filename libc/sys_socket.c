#include <os/syscall.h>
#include <sys/socket.h>
#include <errno.h>
#include <syslog.h>

#if defined(ARCH_i686)
static MK_SYSCALL4(int, accept4, OS_ACCEPT, int, struct sockaddr *, socklen_t *, int)
int accept(int socket, struct sockaddr * address, socklen_t * address_len)
{
    return accept4(socket, address, address_len, 0);
}
#else
MK_SYSCALL3(int, accept, OS_ACCEPT, int, struct sockaddr *, socklen_t *)
#endif

MK_SYSCALL3(int, bind, OS_BIND, int, const struct sockaddr *, socklen_t)
MK_SYSCALL3(int, connect, OS_CONNECT, int, const struct sockaddr *, socklen_t)

int getpeername(int socket, struct sockaddr * address, socklen_t * address_len)
{
    syslog(LOG_DEBUG, "libc: getpeername");
    return 0;
}

int getsockname(int socket, struct sockaddr * address, socklen_t * address_len)
{
    syslog(LOG_DEBUG, "libc: getsockname");
    return 0;
}

int getsockopt(int socket, int level, int option_name, void * option_value, socklen_t * option_len)
{
    syslog(LOG_DEBUG, "libc: getsockopt");
    return 0;
}

MK_SYSCALL2(int, listen, OS_LISTEN, int, int)

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    syslog(LOG_DEBUG, "libc: recv");
    return 0;
}

ssize_t recvfrom(int socket, void * buffer, size_t length, int flags, struct sockaddr * address, socklen_t * address_len)
{
    syslog(LOG_DEBUG, "libc: recvfrom");
    return 0;
}

ssize_t recvmsg(int socket, struct msghdr *message, int flags)
{
    syslog(LOG_DEBUG, "libc: recvmsg");
    return 0;
}

ssize_t send(int socket, const void * buffer, size_t length, int flags)
{
    syslog(LOG_DEBUG, "libc: send");
    return 0;
}

ssize_t sendmsg(int socket, const struct msghdr *message, int flags)
{
    syslog(LOG_DEBUG, "libc: sendmsg");
    return 0;
}

ssize_t sendto(int socket, const void * message, size_t length, int flags, const struct sockaddr * dest_addr, socklen_t dest_len)
{
    syslog(LOG_DEBUG, "libc: sendto");
    return 0;
}

int setsockopt(int socket, int level, int option_name, const void * option_value, socklen_t option_len)
{
    syslog(LOG_DEBUG, "libc: setsockopt");
    return  0;
}

int shutdown(int socket, int how)
{
    syslog(LOG_DEBUG, "libc: shutdown");
    return 0;
}

MK_SYSCALL3(int, socket, OS_SOCKET, int, int, int)

int socketpair(int domain, int type, int protocol, int socket_vector[2])
{
    syslog(LOG_DEBUG, "libc: socketpair");
    errno = ENOSYS;
    return -1;
}
