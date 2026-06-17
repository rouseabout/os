#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char ** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s line\n", argv[0]);
        return EXIT_FAILURE;
    }

    int oldfd = open("/dev/tty", O_RDWR);
    ioctl(oldfd, TIOCNOTTY);
    close(oldfd);

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    ioctl(fd, TIOCSCTTY, 1);

    system("sh");
    return EXIT_SUCCESS;
}
