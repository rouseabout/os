#include <sys/stat.h>
#include <os/syscall.h>
#include <syslog.h>

int chmod(const char *path, mode_t mode)
{
    syslog(LOG_DEBUG, "libc: chmod");
    return 0;
}

int fchmod(int fildes, mode_t mode)
{
    syslog(LOG_DEBUG, "libc: fchmod");
    errno = ENOSYS;
    return -1;
}

MK_SYSCALL2(int, fstat, OS_FSTAT, int, struct stat *)

MK_SYSCALL4(int, fstatat, OS_FSTATAT, int, const char *, struct stat *, int)

MK_SYSCALL2(int, lstat, OS_LSTAT, const char *, struct stat *)

MK_SYSCALL2(int, mkdir, OS_MKDIR, const char *, mode_t)

int mkfifo(const char *path, mode_t mode)
{
    syslog(LOG_DEBUG, "libc: mkfifo path='%s'", path);
    errno = ENOSYS;
    return -1; //FIXME:
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    syslog(LOG_DEBUG, "libc: mknod");
    errno = ENOSYS;
    return -1; //FIXME:
}

MK_SYSCALL2(int, stat, OS_STAT, const char *, struct stat *)

mode_t umask(mode_t cmask)
{
    syslog(LOG_DEBUG, "libc: umask");
    return cmask; //FIXME:
}
