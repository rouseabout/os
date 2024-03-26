#ifndef SYS_TIMES_H
#define SYS_TIMES_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};

clock_t times(struct tms *);

#ifdef __cplusplus
}
#endif

#endif /* SYS_TIMES_H */
