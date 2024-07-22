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

static int grep(const char * pat, const char * path)
{
    READ_FILE(char, buf, size, path)
    int found = 0;
    char * s = buf, * p;
    while ((p = memchr(s, '\n', size))) {
        int len = p - s;
        if (!memfind(s, len, pat, strlen(pat))) {
            printf("%.*s\n", len, s);
            found = 1;
        }
        s += len + 1;
        size -= len + 1;
    }
    free(buf);
    return found;
}

static int grep_main(int argc, char ** argv, char ** envp)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s PATTERN FILE...\n", argv[0]);
        return EXIT_FAILURE;
    }
    int found = 0;
    if (argc == 1)
        found = grep(argv[1], "-");
    else
        for (int i = 2; i < argc; i++)
            found |= grep(argv[1], argv[i]);
    return found ? EXIT_SUCCESS : EXIT_FAILURE;
}
