#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tr_main(int argc, char ** argv, char ** envp)
{
    if (argc != 3 || strlen(argv[1]) != strlen(argv[2])) {
        fprintf(stderr, "usage: %s FROM TO\n", argv[0]);
        return EXIT_FAILURE;
    }
    char lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = i;
    for (int i = 0; i < strlen(argv[1]); i++)
        lut[(int)argv[1][i]] = argv[2][i];
    int c;
    while ((c = getchar()) != EOF)
        putchar(lut[c]);
    return EXIT_SUCCESS;
}
