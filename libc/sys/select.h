#ifndef SYS_SELECT_H
#define SYS_SELECT_H

#include <sys/types.h>
#include <time.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __FD_SETSIZE 1024

typedef long int __fd_mask;
#define __NFDBITS (8 * sizeof(__fd_mask))

typedef struct {
    __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS];
} fd_set;

#define __FD_ELT(d) ((d) / __NFDBITS)
#define __FD_MASK(d) (1ULL << ((d) % __NFDBITS))

#define FD_ISSET(fd, fds) (!!((fds)->fds_bits[__FD_ELT(fd)] & __FD_MASK(fd)))
#define FD_CLR(fd, fds) (fds)->fds_bits[__FD_ELT(fd)] &= ~__FD_MASK(fd)
#define FD_SET(fd, fds) (fds)->fds_bits[__FD_ELT(fd)] |= __FD_MASK(fd)
#define FD_ZERO(fds) do { \
    for (int i = 0; i < sizeof((fds)->fds_bits)/sizeof(__fd_mask); i++) \
        (fds)->fds_bits[i] = 0; \
} while(0)

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
