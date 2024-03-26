#ifndef SYS_STATVFS_H
#define SYS_STATVFS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MNT_WAIT 1 /* not posix */
#define MNT_NOWAIT 2 /* not posix */

struct statvfs {
    unsigned long f_bsize;
    unsigned long f_frsize;
    fsblkcnt_t f_blocks;
    fsblkcnt_t f_bfree;
    fsblkcnt_t f_bavail;
    fsfilcnt_t f_files;
    fsfilcnt_t f_ffree;
    fsfilcnt_t f_favail;
    unsigned long f_fsid;
    unsigned long f_flag;
    unsigned long f_namemax;
    char f_mntfromname[256]; /* not posix */
    char f_mntonname[256]; /* not posix */
    char f_fstypename[64]; /* not posix */
};

int fstatvfs(int, struct statvfs *);
int getmntinfo(struct statvfs **, int); /* not posix */
int statvfs(const char *, struct statvfs *);

#ifdef __cplusplus
}
#endif

#endif /* SYS_STATVFS_H */
