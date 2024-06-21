#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: %s bytes\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    void *p = malloc(n);
    printf("Allocated %d bytes at %p\n", n, p);
    if (p) {
        memset(p, 0xCC, n);
        printf("Press any key to free\n");
        getchar();
        free(p);
        printf("Free'd\n");
    }

    return EXIT_SUCCESS;
}
