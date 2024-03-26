#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stddef.h>

typedef int blkcnt_t;
typedef int blksize_t;
typedef int clock_t;
typedef int clockid_t;
typedef int dev_t;
typedef int ino_t;
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
typedef int gid_t;
typedef int mode_t;
typedef int nlink_t;
typedef int off_t;
typedef int pid_t;
typedef int pthread_attr_t;
typedef int pthread_cond_t;
typedef int pthread_condattr_t;
typedef int pthread_key_t;
typedef int pthread_once_t;
typedef int pthread_mutex_t;
typedef int pthread_mutexattr_t;
typedef int pthread_t;
typedef int ssize_t;
typedef int suseconds_t;
typedef long time_t;
typedef int uid_t;
typedef int useconds_t;

typedef unsigned char u_char; /* not posix */
typedef unsigned short u_short; /* not posix */
typedef unsigned int u_int; /* not posix */
typedef unsigned long u_long; /* not posix */

#endif /* SYS_TYPES_H */
