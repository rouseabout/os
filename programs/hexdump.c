#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

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

    int pos = 0;
    int ret;
    do {
        unsigned char buf[16];
        ret = read(fd, buf, sizeof(buf));
        if (ret <= 0)
            break;
        printf("%5x:", pos);
        int i;
        for (i = 0; i < 16 && i < ret; i++)
            printf(" %02x", buf[i]);
        for (; i < 16; i++) printf("   ");
        printf(" | ");
        for (i = 0; i < 16 && i < ret; i++)
            printf("%c", buf[i] >= 32 && buf[i] <= 127 ? buf[i] : '.');
        pos += 16;
        printf("\n");
    } while (1);

    close(fd);
    return 0;
}
