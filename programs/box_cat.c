#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void cat2(int fd)
{
    char block[1024];
    int size;
    while ((size = read(fd, block, sizeof(block))) > 0)
        write(STDOUT_FILENO, block, size);
}

static int cat_main(int argc, char ** argv, char ** envp)
{
    if (argc == 1)
        cat2(STDIN_FILENO);
    else
        for (int i = 1; i < argc; i++) {
            int fd = open(argv[i], O_RDONLY);
            if (fd == -1) {
                perror(argv[i]);
                continue;
            }
            cat2(fd);
            close(fd);
    }
    return EXIT_SUCCESS;
}
