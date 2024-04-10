#include <stdio.h>

static int clear_main(int argc, char ** argv, char ** envp)
{
    printf("\33[2J\033[1;1H");
    fflush(stdout);
    return 0;
}
