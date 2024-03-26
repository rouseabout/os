#ifndef TIME_H
#define TIME_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCKS_PER_SEC 1000000

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

enum {
    CLOCK_REALTIME = 0,
    CLOCK_MONOTONIC,
};

extern int daylight;
extern long timezone;
extern char *tzname[2];

char * asctime(const struct tm *);
char * asctime_r(const struct tm *, char * buf);
clock_t clock(void);
int clock_getres(clockid_t, struct timespec *);
int clock_gettime(clockid_t, struct timespec *);
char * ctime(const time_t *);
char * ctime_r(const time_t *, char * buf);
double difftime(time_t, time_t);
struct tm *gmtime(const time_t *);
struct tm * gmtime_r(const time_t *, struct tm *);
struct tm *localtime(const time_t *);
struct tm * localtime_r(const time_t *, struct tm *);
time_t mktime(struct tm *);
int nanosleep(const struct timespec *, struct timespec *);
size_t strftime(char *, size_t, const char *, const struct tm *);
time_t time(time_t *);
void tzset(void);

#ifdef __cplusplus
}
#endif

#endif /* TIME_H */
