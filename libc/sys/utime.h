#ifndef SYS_UTIME_H
#define SYS_UTIME_H

#include <sys/types.h>

struct utimbuf {
    time_t actime;
    time_t modtime;
};

#endif /* SYS_UTIME_H */
