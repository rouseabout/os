#include <stdio.h>

static int echo_main(int argc, char ** argv, char ** envp)
{
    for (int i = 1; i < argc; i++)
        printf(i > 1 ? " %s" : "%s", argv[i]);
    printf("\n");
    return 0;
}
