#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

#define st_atime st_atim.tv_sec
#define st_ctime st_ctim.tv_sec
#define st_mtime st_mtim.tv_sec

#define S_IFMT   0xF000
#define S_IFSOCK 0xC000  /* socket */
#define S_IFLNK  0xA000  /* symbolic link */
#define S_IFREG  0x8000  /* regular file */
#define S_IFBLK  0x6000  /* block device */
#define S_IFDIR  0x4000  /* directory */
#define S_IFCHR  0x2000  /* character device */
#define S_IFIFO  0x1000  /* fifo */

#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010
#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

int chmod(const char *, mode_t);
int fchmod(int, mode_t);
int fstat(int fildes, struct stat * buf);
int fstatat(int fd, const char * path, struct stat * buf, int flag);
int lstat(const char *, struct stat *);
int mkdir(const char *, mode_t);
int mkfifo(const char *, mode_t);
int mknod(const char *, mode_t, dev_t);
int stat(const char *, struct stat *);
mode_t umask(mode_t);

#ifdef __cplusplus
}
#endif

#endif /* SYS_STAT_H */
