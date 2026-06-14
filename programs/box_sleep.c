#include <math.h>
#include <stdlib.h>
#include <time.h>

static int sleep_main(int argc, char ** argv, char ** envp)
{
    if (argc != 2)
        return -1;
    double ti, tf;
    tf = modf(atof(argv[1]), &ti);
    nanosleep(&(struct timespec){.tv_sec=ti, .tv_nsec=tf*1000000000}, NULL);
    return 0;
}
