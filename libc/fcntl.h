#ifndef FCNTL_H
#define FCNTL_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define F_GETFL 1
#define F_SETFL 2
#define F_GETFD 3
#define F_SETFD 4
#define F_DUPFD 5
#define F_SETLK 6
#define F_SETLKW 7
#define F_GETLK 8

#define FD_CLOEXEC 1

#define O_ACCMODE 0x7 /* mask */
#define O_RDONLY 0x0
#define O_WRONLY 0x1
#define O_RDWR   0x2

#define O_DIRECTORY 0x10000
#define O_CREAT 0x40
#define O_EXCL 0x80
#define O_APPEND 0x400
#define O_NONBLOCK 0x800
#define O_TRUNC 0x200
#define O_NOFOLLOW 0x20000
#define O_NOCTTY 0x100
#define O_SYNC 0x101000
#define O_NDELAY O_NONBLOCK

#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2

#define AT_FDCWD -100

#define AT_SYMLINK_NOFOLLOW 0x1

struct flock {
    short l_type;
    short l_whence;
    off_t l_start;
    off_t l_len;
    pid_t l_pid;
};

int creat(const char *, mode_t);
int open(const char *, int, ...);
int fcntl(int, int, ...);

#ifdef __cplusplus
}
#endif

#endif /* FCNTL_H */
