#ifndef UTIME_H
#define UTIME_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf {
    time_t actime;
    time_t modtime;
};

int utime(const char *, const struct utimbuf *);

#ifdef __cplusplus
}
#endif

#endif /* UTIME_H */
