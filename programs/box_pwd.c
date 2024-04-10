#include <stdio.h>
#include <unistd.h>

static int pwd_main(int argc, char ** argv, char ** envp)
{
    char cwd[256];
    printf("%s\n", getcwd(cwd, sizeof(cwd)));
    return 0;
}
