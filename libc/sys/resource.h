#ifndef SYS_RESOURCE_H
#define SYS_RESOURCE_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int rlim_t;

#define RLIM_INFINITY 0

#define RLIMIT_CORE 0
#define RLIMIT_CPU 1
#define RLIMIT_DATA 2
#define RLIMIT_NOFILE 3
#define RLIMIT_STACK 4
#define RLIMIT_FSIZE 5

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN 1
struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
};

int getrlimit(int, struct rlimit *);
int getrusage(int, struct rusage *);
int setrlimit(int, const struct rlimit *);

#ifdef __cplusplus
}
#endif

#endif /* SYS_RESOURCE_H */
