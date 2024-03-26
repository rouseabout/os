#ifndef POLL_H
#define POLL_H

#ifdef __cplusplus
extern "C" {
#endif

#define POLLIN 0x1
#define POLLNVAL 0x20
#define POLLOUT 0x4
#define POLLERR 0x8
#define POLLHUP 0x10
#define POLLPRI 0x2
#define POLLWRNORM 0x40

struct pollfd {
    int fd;
    short events;
    short revents;
};

typedef int nfds_t;

int poll(struct pollfd [], nfds_t, int);

#ifdef __cplusplus
}
#endif

#endif /* POLL_H */
