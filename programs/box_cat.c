#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int cat_main(int argc, char ** argv, char ** envp)
{
    int fd;
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
    } else
        fd = STDIN_FILENO;
    char block[1024];
    int size;
    while ((size = read(fd, block, sizeof(block))) > 0)
        write(STDOUT_FILENO, block, size);
    close(fd);
    return EXIT_SUCCESS;
}
