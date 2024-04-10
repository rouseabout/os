#include <stdio.h>

static int reset_main(int argc, char ** argv, char ** envp)
{
    printf("\33c");
    fflush(stdout);
    return 0;
}
