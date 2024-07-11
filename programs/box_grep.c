#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static int memfind(const char * hay, int haysize, const char * needle, int needlesize)
{
    for (int i = 0; i < haysize - needlesize + 1; i++)
        if (!memcmp(hay + i, needle, needlesize))
            return 0;
    return 1;
}

static int grep_main(int argc, char ** argv, char ** envp)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s PATTERN FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    READ_FILE(char, buf, size, argv[2])
    int found = 0;
    char * s = buf, * p;
    while ((p = memchr(s, '\n', size))) {
        int len = p - s;
        if (!memfind(s, len, argv[1], strlen(argv[1]))) {
            printf("%.*s\n", len, s);
            found = 1;
        }
        s += len + 1;
        size -= len + 1;
    }
    free(buf);
    return found ? EXIT_SUCCESS : EXIT_FAILURE;
}
