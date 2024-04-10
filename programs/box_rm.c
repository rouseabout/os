#include <stdio.h>
#include <unistd.h>

static int rm_main(int argc, char ** argv, char ** envp)
{
    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            printf("cannot remove '%s'\n", argv[i]);
            return -1;
        }
    }
    return 0;
}
