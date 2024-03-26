#include <signal.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
    if (argc < 2)
        return -1;
    int signum = SIGTERM;
    for (int i = 1; i < argc; i++) {
        int v = atoi(argv[i]);
        if (v < 0)
            signum = -v;
        else
            kill(v, signum);
    }
    return 0;
}
