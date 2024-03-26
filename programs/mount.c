#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

int main()
{
    struct statvfs * s;
    int size = getmntinfo(&s, MNT_NOWAIT);
    if (size < 0) {
        perror("getmntinfo");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < size; i++)
        printf("%s %s\n", s[i].f_fstypename, s[i].f_mntonname);

    return EXIT_SUCCESS;
}
