#include <poll.h>
#include <sys/select.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))

int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    int max_fd = -1;

    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd < 0)
            continue;
        if (fds[i].events & POLLIN) {
            FD_SET(fds[i].fd, &rfds);
            max_fd = MAX(max_fd, fds[i].fd);
        }
        if (fds[i].events & POLLOUT) {
            FD_SET(fds[i].fd, &wfds);
            max_fd = MAX(max_fd, fds[i].fd);
        }
    }

    struct timeval timeout_tv = {.tv_sec=timeout / 1000, .tv_usec=(timeout % 1000) * 1000};
    int n = select(max_fd + 1, &rfds, &wfds, NULL, &timeout_tv);
    if (n < 0)
        return n;

    int ret = 0;
    for (int i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].fd < 0)
            continue;
        if (FD_ISSET(fds[i].fd, &rfds))
            fds[i].revents |= POLLIN;
        if (FD_ISSET(fds[i].fd, &wfds))
            fds[i].revents |= POLLOUT;
        if (fds[i].revents)
            ret++;
    }

    return ret;
}
