#include <time.h>
#include <stdio.h>

static int date_main(int argc, char ** argv, char ** envp)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("%s\n", ctime(&ts.tv_sec));
    return 0;
}
