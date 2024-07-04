#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int dd_main(int argc, char ** argv, char ** envp)
{
    int fdi = STDIN_FILENO, fdo = STDOUT_FILENO, bs = 512, count = 0, seek = 0, skip = 0;
    for (int i = 1; i < argc; i++) {
        char * eq = strchr(argv[i], '=');
        if (eq) {
            *eq++ = 0;
            if (!strcmp(argv[i], "bs")) {
                bs = atoi(eq);
                if (bs <= 0) {
                    fprintf(stderr, "invalid bs\n");
                    return EXIT_FAILURE;
                }
            } else if (!strcmp(argv[i], "count")) {
                count = atoi(eq);
            } else if (!strcmp(argv[i], "if")) {
                fdi = open(eq, O_RDONLY);
                if (fdi == -1) { 
                    perror(eq);
                    return EXIT_FAILURE;
                }
            } else if (!strcmp(argv[i], "of")) {
                fdo = open(eq, O_WRONLY|O_CREAT, 0666);
                if (fdo == -1) { 
                    perror(eq);
                    return EXIT_FAILURE;
                }
            } else if (!strcmp(argv[i], "seek")) {
                seek = atoi(eq);
            } else if (!strcmp(argv[i], "skip")) {
                skip = atoi(eq);
            }
        }
    }
    if (seek)
        lseek(fdo, seek * bs, SEEK_CUR);
    if (skip)
        lseek(fdi, skip * bs, SEEK_CUR);
    char * block = malloc(bs);
    if (!block)
        return EXIT_FAILURE;
    int total = 0, size;
    while ((size = read(fdi, block, bs)) > 0) {
        write(fdo, block, size);
        total += size;
        if (count) {
            count--;
            if (!count)
                break;
        }
    }
    fprintf(stderr, "%d bytes copied\n", total);
    free(block);
    close(fdi);
    close(fdo);
    return EXIT_SUCCESS;
}
