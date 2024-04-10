#include <stdio.h>
#include <stdlib.h>

static int mv_main(int argc, char ** argv, char ** envp)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s old new\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (rename(argv[1], argv[2]) < 0) {
        perror("mv");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
