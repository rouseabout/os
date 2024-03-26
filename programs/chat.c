#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static void echo(int fd, int state)
{
    struct termios t;
    tcgetattr(fd, &t);
    if (state)
        t.c_lflag |= ECHO;
    else
        t.c_lflag &= ~ECHO;
    tcsetattr(fd, 0, &t);
}

static int open_flags(char * path)
{
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open: %s: failed\n", path);
        return fd;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    echo(fd, 0);

    return fd;
}

static void read_write(int src_fd, int dst_fd)
{
    char buf[1024];
    int n = read(src_fd, buf, sizeof(buf));
    if (n > 0)
        write(dst_fd, buf, n);
}

int fda, fdb;

static void signal_handler(int sig)
{
    echo(fda, 1);
    echo(fdb, 1);
    exit(0);
}

int main(int argc, char ** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s device [device]\n", argv[0]);
        return EXIT_FAILURE;
    }

    fda = open_flags(argv[1]);
    if (fda < 0)
        return EXIT_FAILURE;

    fdb = open_flags(argc > 2 ? argv[2] : "/dev/tty");
    if (fdb < 0)
        return EXIT_FAILURE;

    signal(SIGINT, signal_handler);

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fda, &rfds);
        FD_SET(fdb, &rfds);

#define MAX(a, b) ((a)>(b)?(a):(b))
        int n = select(MAX(fda, fdb) + 1, &rfds, NULL, NULL, NULL);
        if (n < 0) {
            fprintf(stderr, "select failed\n");
            return EXIT_FAILURE;
        }

        if (FD_ISSET(fda, &rfds))
            read_write(fda, fdb);

        if (FD_ISSET(fdb, &rfds))
            read_write(fdb, fda);
    }

    return 0;
}
