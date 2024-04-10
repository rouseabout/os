#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int ln_main(int argc, char ** argv, char ** envp)
{
    int ret;
    if (argc == 3) {
        ret = link(argv[1], argv[2]);
    } else if (argc == 4 && !strcmp(argv[1], "-s")) {
        ret = symlink(argv[2], argv[3]);
    } else {
       printf("usage: %s [-s] src dst\n", argv[0]);
       return EXIT_FAILURE;
    }

    if (ret == -1) {
       perror(argv[0]);
       return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
