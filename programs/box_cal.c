#include <stdio.h>
#include <langinfo.h>
#include <time.h>

static int is_common_year(int y)
{
    return (y % 4) || (!(y % 100) && (y % 400));
}

static int mdays(int y, int m)
{
    static const int dim[12] = {31,29,31,30,31,30,31,31,30,31,30,31};
    return (m == 1 && is_common_year(y)) ? 28 : dim[m - 1];
}

static int cal_main(int argc, char ** argv, char ** envp)
{
    time_t t;
    struct tm tm;
    t = time(NULL);
    localtime_r(&t, &tm);
    tm.tm_mday = 1;
    t = mktime(&tm);
    localtime_r(&t, &tm);
    printf("%s %d\n", nl_langinfo(MON_1 + tm.tm_mon), 1900 + tm.tm_year);
    printf("Su Mo Tu We Th Fr Sa\n");
    int wday = tm.tm_wday;
    for (int i = 0; i < wday; i++)
        printf("   ");
    for (int i = 1; i <= mdays(1900 + tm.tm_year, tm.tm_mon); i++) {
        printf("%2d ", i);
        wday++;
        if (wday >= 7) {wday = 0; printf("\n");}
    }
    if (wday < 7)
        printf("\n");
    return 0;
}
