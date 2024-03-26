#include <stdio.h>

int main()
{
    printf("%c", 7);
    fflush(stdout);
#if 0
    for (int i = 0; i < 256; i++) {
        printf("%c", i);
        fflush(stdout);
    }
#endif
}
