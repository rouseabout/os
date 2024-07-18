#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int head_main(int argc, char ** argv, char ** envp)
{
    int n = 10;
    int opt;
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
        case 'n':
            n = atoi(optarg);
            break;
        default:
            fprintf(stderr, "usage: %s [-n NUMBER] [FILE]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    int fd;
    if (optind < argc) {
        fd = open(argv[optind], O_RDONLY);
        if (fd == -1) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
    } else
        fd = STDIN_FILENO;
    char c;
    while (n && read(fd, &c, sizeof(c) == 1)) {
        write(STDOUT_FILENO, &c, sizeof(c));
        if (c == '\n')
            n--;
    }
    close(fd);
    return EXIT_SUCCESS;
}
