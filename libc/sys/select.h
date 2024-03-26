#ifndef SYS_SELECT_H
#define SYS_SELECT_H

#include <sys/types.h>
#include <time.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int fds_bits[1]; } fd_set;

#define FD_ISSET(fd, fds) ((fds)->fds_bits[0] & (1 << fd))
#define FD_CLR(fd, fds) do { (fds)->fds_bits[0] &= ~(1 << fd); } while(0)
#define FD_SET(fd, fds) do { (fds)->fds_bits[0] |= (1 << fd); } while(0)
#define FD_ZERO(fds) do { (fds)->fds_bits[0] = 0; } while(0)

#define FD_SETSIZE 32

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

int pselect(int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *);
int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#ifdef __cplusplus
}
#endif

#endif /* SYS_SELECT_H */
