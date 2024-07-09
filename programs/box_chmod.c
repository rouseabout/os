#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static int chmod_main(int argc, char ** argv, char ** envp)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s MODE FILE...\n", argv[0]);
        return EXIT_FAILURE;
    }
    unsigned int mode;
    if (sscanf(argv[1], "%o", &mode) != 1) {
        fprintf(stderr, "error\n");
        return EXIT_FAILURE;
    }
    for (int i = 2; i < argc; i++)
        if (chmod(argv[i], mode))
            perror(argv[i]);
    return EXIT_SUCCESS;
}
