#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

static int wc_main(int argc, char ** argv, char ** envp)
{
    int fd;
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror(argv[1]);
            return -1;
        }
    } else
        fd = STDIN_FILENO;

    int bytes = 0, words = 0, lines = 0, in_word = 0;
    char c;
    while (read(fd, &c, sizeof(c))) {
        bytes++;
        if (in_word) {
            if (isspace(c))
                in_word = 0;
        } else {
            if (!isspace(c)) {
                in_word = 1;
                words++;
            }
        }
        if (c == '\n')
            lines++;
    }
    close(fd);
    printf("%d %d %d %s\n", lines, words, bytes, argc == 2 ? argv[1] : "-");
    return 0;
}
