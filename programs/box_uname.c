#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

static int uname_main(int argc, char ** argv, char ** envp)
{
    int m = 0, n = 0, r = 0, s = 0, v = 0;
    int opt;
    while ((opt = getopt(argc, argv, "amnrsv")) != -1) {
        switch (opt) {
        case 'a': m = n = r = s = v = 1; break;
        case 'm': m = 1; break;
        case 'n': n = 1; break;
        case 'r': r = 1; break;
        case 's': s = 1; break;
        case 'v': v = 1; break;
        default:
            fprintf(stderr, "usage: %s [-amnrsv]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!m && !n && !r && !s && !v)
        s = 1;

    struct utsname un;
    uname(&un);
    if (s)
        printf("%s ", un.sysname);
    if (n)
        printf("%s ", un.nodename);
    if (r)
        printf("%s ", un.release);
    if (v)
        printf("%s ", un.version);
    if (m)
        printf("%s ", un.machine);
    printf("\n");

    return 0;
}
