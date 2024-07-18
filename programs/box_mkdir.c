#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static int mkdir_main(int argc, char ** argv, char ** envp)
{
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0666) == -1) {
            perror(argv[i]);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
