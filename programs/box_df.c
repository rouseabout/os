#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

static int df_main(int argc, char ** argv, char ** envp)
{
    struct statvfs * s;
    int size = getmntinfo(&s, MNT_NOWAIT);
    if (size < 0) {
        perror("getmntinfo");
        return EXIT_FAILURE;
    }

    printf("fstype mnt bsize blocks bfree\n");
    for (int i = 0; i < size; i++)
        printf("%s %s %ld %ld %ld\n", s[i].f_fstypename, s[i].f_mntonname, s[i].f_bsize, s[i].f_blocks, s[i].f_bfree);

    return EXIT_SUCCESS;
}
