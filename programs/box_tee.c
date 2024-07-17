#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int tee_main(int argc, char ** argv, char ** envp)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    int fd = open(argv[1], O_WRONLY|O_CREAT, 0666);
    if (fd == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    char block[1024];
    int size;
    while ((size = read(STDIN_FILENO, block, sizeof(block))) > 0) {
        write(STDOUT_FILENO, block, size);
        write(fd, block, size);
    }
    close(fd);
    return EXIT_SUCCESS;
}
