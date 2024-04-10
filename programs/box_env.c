#include <stdio.h>

static int env_main(int argc, char ** argv, char ** envp)
{
    for (unsigned int i = 0; envp[i]; i++)
        printf("%s\n", envp[i]);

    return 0;
}
