#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define USAGE fprintf(stderr, "usage: %s [-s SIZE] [path]\n", argv[0]);

int main(int argc, char ** argv)
{
    off_t size = -1;
    int opt;
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
        case 's':
           size = atoi(optarg);
           break;
        default:
           USAGE
           return EXIT_FAILURE;
        }
    }

    if (size < 0 || optind >= argc) {
        USAGE
        return EXIT_FAILURE;
    }

    if (truncate(argv[optind], size) == -1) {
        perror(argv[optind]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
