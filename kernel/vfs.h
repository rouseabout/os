#ifndef VFS_H
#define VFS_H

#include <dirent.h>
#include <limits.h>
#include <stddef.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#if TEST
struct os_mmap_request {
int foo;
};
#else
#include <os/syscall.h>
#endif
#include <termios.h>

typedef struct FileDescriptor FileDescriptor;

typedef struct {
    void (*refcount)(FileDescriptor *, int delta);
    void (*close)(FileDescriptor *);
    int (*read)(FileDescriptor *, void *, int);
    int (*read_available)(const FileDescriptor * fd);
    int (*write)(FileDescriptor *, const void *, int);
    int (*write_available)(const FileDescriptor * fd);
    off_t (*lseek)(FileDescriptor *, off_t, int);
    pid_t (*tcgetpgrp)(FileDescriptor * fd);
    int (*tcsetpgrp)(FileDescriptor * fd, pid_t pgrp);
    int (*truncate)(FileDescriptor * fd, off_t length);
    int (*ioctl)(FileDescriptor * fd, int request, void * data);
    int (*mmap)(FileDescriptor * fd, struct os_mmap_request * req);
} DeviceOperations; //these are really file operations

typedef struct {
    const char * name;
    void (*usage)(void * priv_data, unsigned long * block_size_ptr, unsigned long * total_ptr, unsigned long * free_ptr);
    int (*inode_range)(void * priv_data, int * min_inode, int * max_inode);
    int (*inode_resolve)(void * priv_data, int dinode, const char * target);
    int (*inode_info)(void * priv_data, int inode);
    int (*inode_symlink)(void * priv_data, int inode, char * buf, size_t size);
    int (*inode_creat)(void * priv_data, int dinode, const char * base, int mode, const char * symbolic_link);
    int (*inode_stat)(void * priv_data, int inode, struct stat * st);
    int (*inode_populate_dir)(void * priv_data, int dinode, int inode);
    int (*inode_unpopulate_dir)(void * priv_data, int dinode, int inode, int is_dir, int links_adjustment);
    int (*inode_increment_links_count)(void * priv_data, int inode);
    int (*inode_append_dir)(void * priv_data, int dinode, int inode, const char * new_name, int is_dir);
    int (*inode_utime)(void * priv_data, int inode, const struct utimbuf * times);
    int (*inode_chmod)(void * priv_data, int inode, mode_t mode);

    int (*open2)(FileDescriptor *);
    DeviceOperations file;
    int (*getdents)(FileDescriptor * fd, struct dirent * dent, size_t count);
} FileOperations; //filesystem operations

typedef struct Mount Mount;

struct Mount {
    int dinode;
    char name[PATH_MAX];
    const FileOperations * ops;
    void * priv_data;
    int root_inode;
    Mount * next;
};

struct FileDescriptor {
    unsigned int refcount;
    Mount * mount; /* optional */
    const DeviceOperations * ops;
    void * priv_data;

    int inode;
    int flags;
    int fd_flags;
    int isatty;
    ssize_t pos;

    ssize_t mount_pos;

    int dirty;
    char * buf;
    ssize_t buf_size;

    char path[PATH_MAX];
};

FileDescriptor * vfs_open(const char * path, int flags, mode_t mode);
void vfs_close(FileDescriptor * fd);
int vfs_write(FileDescriptor * fd, const void * buf, int size);
int vfs_read(FileDescriptor * fd, void * buf, int size);
int vfs_dup_fds(FileDescriptor ** dst, FileDescriptor ** src, int count);
off_t vfs_lseek(FileDescriptor * fd, off_t offset, int whence);
int vfs_getdents(FileDescriptor * fd, struct dirent * dent, size_t count);
void vfs_register_mount_point(const char * path, const FileOperations * ops, void * priv_data);
void vfs_register_mount_point2(int dinode, const char * name, const FileOperations * ops, void * priv_data, int root_inode);
int vfs_stat(FileDescriptor * fd, const char * path, struct stat * st, int resolve_link);
pid_t vfs_tcgetpgrp(FileDescriptor * fd);
int vfs_tcsetpgrp(FileDescriptor * fd, pid_t pgrp);
int vfs_unlink(const char * path, int dir);
int vfs_mkdir(const char *path, mode_t mode);
int vfs_ioctl(FileDescriptor * fd, int request, void * data);
int vfs_mmap(FileDescriptor * fd, struct os_mmap_request * req);
int vfs_fstat(FileDescriptor * fd, struct stat * st);
int vfs_read_available(const FileDescriptor * fd);
int vfs_write_available(const FileDescriptor * fd);
int vfs_pipe(FileDescriptor ** rfd_ptr, FileDescriptor ** wfd_ptr);
int vfs_isatty(const FileDescriptor * fd);
int vfs_direct(const FileDescriptor * fd);
int vfs_utime(const char * path, const struct utimbuf * times);
int vfs_chmod(const char * path, mode_t mode);
int vfs_rename(const char * old, const char * new);
int vfs_getmntinfo(struct statvfs * mntbufp, int size);
ssize_t vfs_readlink(const char * path, char * buf, size_t bufsize);
int vfs_link(const char * path1, const char * path2, int symbolic);
int vfs_truncate(FileDescriptor * fd, off_t length);
FileDescriptor * vfs_socket_get_other(FileDescriptor * fd);
FileDescriptor * vfs_socket(FileDescriptor * other_fd);
int vfs_bind(FileDescriptor * fd, const char * name);
int vfs_accept(FileDescriptor * fd);
int vfs_is_connected(FileDescriptor * fd);
int vfs_connect(FileDescriptor * fd, const char * name);
void vfs_dump_sockets(void);

extern const DeviceOperations null_dio;
extern const DeviceOperations urandom_dio;
extern const DeviceOperations power_dio;
extern const DeviceOperations reboot_dio;
extern const DeviceOperations zero_dio;

#endif /* VFS_H */
