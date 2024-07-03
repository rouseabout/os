#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static int memfind(const char * hay, int haysize, const char * needle, int needlesize)
{
    for (int i = 0; i < haysize - needlesize; i++)
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
    int fd = open(argv[2], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    size_t size = lseek(fd, 0, SEEK_END);
    char * buf = malloc(size + 1);
    if (!buf) {
        perror("malloc");
        return EXIT_FAILURE;
    }   
    lseek(fd, 0, SEEK_SET);
    size = read(fd, buf, size);
    if (size == -1) {
        perror("read");
        return EXIT_FAILURE;
    }
    close(fd);
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
