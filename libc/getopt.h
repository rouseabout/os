#ifndef GETOPT_H
#define GETOPT_H

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct option { /* not posix */
    const char * name;
    int has_arg;
    int * flag;
    int val;
};

int getopt_long(int, char * const [], const char *, const struct option *, int *);

#define no_argument 0
#define required_argument 1
#define optional_argument 2

#ifdef __cplusplus
}
#endif

#endif /* GETOPT_H */
