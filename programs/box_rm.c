#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int rm_main(int argc, char ** argv, char ** envp)
{
    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) == -1) {
            perror(argv[i]);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
