#include <stdlib.h>
#include <stdio.h>

static int env_main(int argc, char ** argv, char ** envp)
{
    for (unsigned int i = 0; environ[i]; i++)
        printf("%s\n", environ[i]);

    return 0;
}
