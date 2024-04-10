#include <unistd.h>
#include <stdlib.h>

static int sleep_main(int argc, char ** argv, char ** envp)
{
    if (argc != 2)
        return -1;
    sleep(atoi(argv[1]));
    return 0;
}
