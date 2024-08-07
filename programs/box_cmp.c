#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int cmp_main(int argc, char ** argv, char ** envp)
{
    int fda, fdb;
    if (argc != 3) {
        printf("cmp file1 file2\n");
        return EXIT_FAILURE;
    }

    fda = open(argv[1], O_RDONLY);
    if (fda < 0) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    fdb = open(argv[2], O_RDONLY);
    if (fdb < 0) {
        perror(argv[2]);
        return EXIT_FAILURE;
    }

    int pos = 1;
    int line = 1;
    do {
        char a, b;
        int reta = read(fda, &a, sizeof(a));
        int retb = read(fdb, &b, sizeof(b));

        if (reta < 0 || retb < 0) {
            printf("i/o error\n");
            return EXIT_FAILURE;
        }

        if (reta == 0 && retb == 0)
            break;

        if (a != b) {
            printf("%s %s differ: char %d, line %d\n", argv[1], argv[2], pos, line);
            return EXIT_FAILURE;
        }

        if (a == '\n')
            line++;
        pos++;
    } while(1);

    close(fda);
    close(fdb);

    return EXIT_SUCCESS;
}
