#include <stdio.h>
#include <unistd.h>

int main(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            printf("cannot remove '%s'\n", argv[i]);
            return -1;
        }
    }
    return 0;
}
