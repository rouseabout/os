#ifndef SYS_UIO_H
#define SYS_UIO_H

#include <sys/types.h>

struct iovec {
    void * iov_base;
    size_t iov_len;
};

ssize_t readv(int, const struct iovec *, int); 
ssize_t writev(int, const struct iovec *, int);

#endif /* SYS_UIO_H */
