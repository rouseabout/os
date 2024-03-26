#include <stdio.h>

int main()
{
    printf("\33[2J\033[1;1H");
    fflush(stdout);
    return 0;
}
