#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static int strings(const char * path)
{
    READ_FILE(char, buf, size, path);
    int state = 0, start;
    for (int i = 0; i < size; i++) {
        if (buf[i] >= 32 && buf[i] < 127) {
            if (state == 0)
                start = i;
            state++;
        } else {
            if (state >= 5)
                printf("%.*s\n", i - start, buf + start);
            state = 0;
        }
    }
    if (state >= 5)
        printf("%.*s\n", size - start, buf + start);
    return EXIT_SUCCESS;
}

static int strings_main(int argc, char ** argv, char ** envp)
{
    if (argc == 1)
        strings("-");
    else
        for (int i = 1; i < argc; i++)
            strings(argv[i]);
    return EXIT_SUCCESS;
}
