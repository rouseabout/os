#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ITIMER_REAL 1

struct itimerval {
    struct timeval it_interval;
    struct timeval it_value;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int getitimer(int, struct itimerval *);
int gettimeofday(struct timeval *, void *);
int setitimer(int, const struct itimerval *, struct itimerval *);
int utimes(const char *, const struct timeval[2]);

#ifdef __cplusplus
}
#endif

#endif /* SYS_TIME_H */
