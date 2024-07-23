#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int tail_main(int argc, char ** argv, char ** envp)
{
    int n = 10;
    int opt;
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
        case 'n':
            n = atoi(optarg);
            break;
        default:
            fprintf(stderr, "usage: %s [-n NUMBER] [FILE]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    const char * path = optind < argc ? argv[optind] : "-";
    READ_FILE(uint8_t, buf, size, path)
    if (buf[size - 1] != '\n') {
        buf = realloc(buf, size + 1);
        if (!buf) {
            perror("maloc");
            return EXIT_FAILURE;
        }
        buf[size++] = '\n';
    }

    int count = 0;
    for (size_t i = 0; i < size; i++)
        if (buf[i] == '\n')
            count++;

    count -= n;

    size_t i = 0;
    while(count > 0) {
        if (buf[i] == '\n')
            count--;
        i++;
    }
    printf("%.*s", (int)(size - i), buf + i);
    free(buf);
    return EXIT_SUCCESS;
}
