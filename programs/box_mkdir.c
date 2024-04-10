#include <stdio.h>
#include <sys/stat.h>

static int mkdir_main(int argc, char ** argv, char ** envp)
{
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0666) < 0) {
            printf("cannot create directory '%s'\n", argv[i]);
            return -1;
        }
    }
    return 0;
}
