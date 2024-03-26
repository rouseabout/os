#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int tty_fd = 0;
static struct termios t0;

static void signal_handler(int sig)
{
    tcsetattr(tty_fd, 0, &t0);
    exit(0);
}

int main(int argc, char ** argv)
{
    int fd;
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror(argv[1]);
            return -1;
        }
    } else
        fd = STDIN_FILENO;

    tty_fd = open("/dev/tty", O_RDONLY);
    if (tty_fd < 0) {
        perror("/dev/tty");
        return -1;
    }

    struct winsize ws;
    if (ioctl(tty_fd, TIOCGWINSZ, &ws) < 0) {
        ws.ws_row = 25;
        ws.ws_col = 80;
    }

    tcgetattr(tty_fd, &t0);
    signal(SIGINT, signal_handler);
    struct termios t1 = t0;
    t1.c_lflag &= ~ICANON;
    tcsetattr(tty_fd, 0, &t1);

    int row = 0, col = 0;
    int ret;
    do {
        unsigned char buf[256];
        ret = read(fd, buf, sizeof(buf));
        if (ret <= 0)
            break;
        for (int i = 0; i < ret; i++) {
            int c = buf[i];
            putchar(c);

            if (c == '\n') {
                row++;
                col = 0;
            } else if (c == '\t') {
                col += 8;
                col &= ~7;
            } else
                col++;

            if (col >= ws.ws_col) {
                row++;
                col = 0;
            }

            if (row >= ws.ws_row - 1) {
                printf("\033[7m --MORE-- \033[0m");
                fflush(stdout);
                char c2;
                read(tty_fd, &c2, 1);
                if (c2 == 'q')  {
                    printf("\n");
                    goto done;
                }
                row = 0;
                col = 0;
            }
        }

    } while (1);

done:
    tcsetattr(tty_fd, 0, &t0);
    close(fd);
    close(tty_fd);
    return 0;
}
