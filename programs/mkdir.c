#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0666) < 0) {
            printf("cannot create directory '%s'\n", argv[i]);
            return -1;
        }
    }
    return 0;
}
