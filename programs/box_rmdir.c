#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int rmdir_main(int argc, char ** argv, char ** envp)
{
    for (int i = 1; i < argc; i++) {
        if (rmdir(argv[i]) == -1) {
            perror(argv[i]);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
