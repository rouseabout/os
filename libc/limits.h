#ifndef LIMITS_H
#define LIMITS_H

#if __GNUC__ < 13
#include "../include-fixed/limits.h"  //gcc supplied limits
#else
#ifndef _GCC_NEXT_LIMITS_H
#define _LIBC_LIMITS_H_
#include "../install-tools/include/limits.h"  //gcc supplied limits
#endif
#endif

#define SSIZE_MAX LONG_MAX

#define _POSIX_ARG_MAX 4096
#define _POSIX_NAME_MAX 14
#define _POSIX_NGROUPS_MAX 8
#define _POSIX_OPEN_MAX 20
#define _POSIX_PATH_MAX 256

#define ARG_MAX _POSIX_ARG_MAX
#define NAME_MAX 64
#define NGROUPS_MAX _POSIX_NGROUPS_MAX
#define OPEN_MAX _POSIX_OPEN_MAX
#define PATH_MAX _POSIX_PATH_MAX

#endif /* LIMITS_H */
