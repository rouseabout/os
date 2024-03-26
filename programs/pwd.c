#include <stdio.h>
#include <unistd.h>

int main()
{
    char cwd[256];
    printf("%s\n", getcwd(cwd, sizeof(cwd)));
    return 0;
}
