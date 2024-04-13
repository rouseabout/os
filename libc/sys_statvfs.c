#include <stdlib.h>
#include <string.h>
#include <os/syscall.h>
#include <sys/statvfs.h>

static struct statvfs * mntbuf = NULL;

#include <syslog.h>
int fstatvfs(int fildes, struct statvfs * buf)
{
    syslog(LOG_DEBUG, "libc: fstatvfs: fd=%d", fildes);
    memset(buf, 0, sizeof(*buf));
    return 0;
}

static MK_SYSCALL2(int, sys_getmntinfo, OS_GETMNTINFO, struct statvfs *, int)
int getmntinfo(struct statvfs ** mntbufp, int flags)
{
    int size = sys_getmntinfo(NULL, 0);
    if (size < 0) {
        return 0;
    }

    if (size == 0) {
        errno = ENOENT;
        return 0;
    }

    struct statvfs * tmp = realloc(mntbuf, sizeof(struct statvfs) * size);
    if (!tmp) {
        errno = ENOMEM;
        return -1;
    }
    mntbuf = tmp;

    size = sys_getmntinfo(mntbuf, size);
    *mntbufp = mntbuf;
    return size;
}

int statvfs(const char * path, struct statvfs * buf)
{
    struct statvfs * s;
    int size = getmntinfo(&s, MNT_NOWAIT);
    for (int i = 0; i < size; i++) {
        if (!strcmp(s[i].f_mntonname, path)) {
            *buf = s[i];
            return 0;
        }
    }

    errno = ENOENT;
    return -1;
}
