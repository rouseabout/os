#include <stdio.h>
#include <stdlib.h>

static int mktemp_main(int argc, char ** argv, char ** envp)
{
    char * template;

    if (argc > 2) {
        fprintf(stderr, "usage: %s template\n", argv[0]);
        return EXIT_FAILURE;
    }

    template = argc == 2 ? argv[1] : "/tmp/tmp.XXXXXX";
    if (mkstemp(template) == -1) {
        perror("mktemp");
        return EXIT_FAILURE;
    }

    printf("%s\n", template);
    return EXIT_SUCCESS;
}
