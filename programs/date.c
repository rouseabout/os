#include <time.h>
#include <stdio.h>

int main()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("%s\n", ctime(&ts.tv_sec));
    return 0;
}
