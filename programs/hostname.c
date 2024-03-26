#include <stdio.h>
#include <unistd.h>

int main()
{
    char hostname[64];
    gethostname(hostname, sizeof(hostname));
    printf("%s\n", hostname);
    return 0;
}
