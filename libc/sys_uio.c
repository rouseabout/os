#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

ssize_t readv(int fildes, const struct iovec * iov, int iovcnt)
{
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        int ret = read(fildes, iov[i].iov_base, iov[i].iov_len);
        if (ret < 0)
            return ret;
        total += ret;
        if (ret < iov[i].iov_len)
            return total;
    }
    return total;
}

ssize_t writev(int fildes, const struct iovec * iov, int iovcnt)
{
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        int ret = write(fildes, iov[i].iov_base, iov[i].iov_len);
        if (ret < 0)
            return ret;
        total += ret;
    }
    return total;
}
