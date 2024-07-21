#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int compar(const void * a, const void * b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

static int sort_main(int argc, char ** argv, char ** envp)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    READ_FILE(char, buf, size, argv[1]);
    if (!size)
        return EXIT_SUCCESS;

    if (buf[size - 1] != '\n') {
        buf = realloc(buf, size + 1);
        if (!buf) {
            perror("maloc");
            return EXIT_FAILURE;
        }
        buf[size++] = '\n';
    }

    int count = 0;
    for (int i = 0; i < size; i++) {
        if (buf[i] == '\n')
            count++;
    }

    char ** list = malloc(count * sizeof(char *));
    if (!list) {
        perror("maloc");
        return EXIT_FAILURE;
    }

    int n = 0;
    list[n] = buf;
    for (int i = 0; i < size; i++) {
        if (buf[i] == '\n') {
            buf[i] = 0;
            n++;
            if (n >= count)
                break;
            list[n] = buf + i + 1;
        }
    }

    qsort(list, count, sizeof(char *), compar);
    for (int i = 0; i < count; i++)
        printf("%s\n", list[i]);
    free(list);
    free(buf);
    return EXIT_SUCCESS;
}
