#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static int strings_main(int argc, char ** argv, char ** envp)
{
    READ_FILE(char, buf, size, argc == 1 ? "-" : argv[1]);
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
