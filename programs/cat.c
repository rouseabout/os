#include <fcntl.h>
#include <stdio.h>
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

    char block[1024];
    int size;
    while ((size = read(fd, block, sizeof(block))) > 0)
        write(STDOUT_FILENO, block, size);

    close(fd);

    return 0;
}
