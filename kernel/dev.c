#include "dev.h"
#include "utils.h"
#include <bsd/string.h>
#include <errno.h>
#include <string.h>

typedef struct DeviceFile DeviceFile;

struct DeviceFile {
    int inode;
    const char * name;
    const DeviceOperations * ops;
    int isatty;
    int (*getsize)(void * priv_data);
    const char * symlink;
    void * priv_data;
    DeviceFile * next;
};

static int dev_inode_next = DEV_INODE_MIN + 1;
static DeviceFile * dev_inodes = NULL;

static void add_dev_inode(int inode, const char * name, const DeviceOperations * ops, int isatty, int (*getsize)(void * priv_data), void * priv_data, const char * symlink)
{
    DeviceFile * d = kmalloc(sizeof(DeviceFile), "device-inode");
    d->inode = inode;
    d->name = name;
    d->priv_data = priv_data;
    d->ops = ops;
    d->isatty = isatty;
    d->getsize = getsize;
    d->symlink = symlink;
    d->next = NULL;

    DeviceFile ** dp;
    for (dp = &dev_inodes; *dp; dp = &(*dp)->next) ;
    *dp = d;
}

void dev_init()
{
    add_dev_inode(DEV_INODE_MIN, ".", &null_dio, 0, NULL, NULL, NULL);
    add_dev_inode(2, "..", NULL, 0, NULL, NULL, NULL);
}

void dev_register_device(const char * name, const DeviceOperations * ops, int isatty, int (*getsize)(void * priv_data), void * priv_data)
{
    add_dev_inode(dev_inode_next++, name, ops, isatty, getsize, priv_data, NULL);
}

void dev_register_symlink(const char * name, const char * symlink)
{
    add_dev_inode(dev_inode_next++, name, NULL, 0, NULL, NULL, symlink);
}

static int dev_getdents(FileDescriptor * fd, struct dirent * dent, size_t count)
{
    int i = 0;
    for (DeviceFile * d = dev_inodes; d; d = d->next, i++) {
        if (i == fd->pos) {
            dent[0].d_ino = d->inode;
            dent[0].d_off = 0;
            dent[0].d_reclen = sizeof(struct dirent);
            strlcpy(dent[0].d_name, d->name, sizeof(dent[0].d_name));
            fd->pos++;
            return 1;
        }
    }
    return 0;
}

static int dev_inode_range(void * priv_data, int * min_inode, int * max_inode)
{
    *min_inode = DEV_INODE_MIN;
    *max_inode = DEV_INODE_MAX;
    return 0;
}

static int dev_inode_resolve(void * priv_data, int dinode, const char * target)
{
    if (dinode != DEV_INODE_MIN)
        return -ENOENT;

    for (DeviceFile * d = dev_inodes; d; d = d->next)
        if (!strcmp(d->name, target))
            return d->inode;

    return -ENOENT;
}

static int dev_inode_symlink(void * priv_data, int inode, char * buf, size_t size)
{
    for (DeviceFile * d = dev_inodes; d; d = d->next)
        if (d->inode == inode && d->symlink)
            return strlcpy(buf, d->symlink, size);
    return -ENOENT;
}

static int dev_inode_stat(void * priv_data, int inode, struct stat * st)
{
    memset(st, 0, sizeof(struct stat));
    st->st_ino = inode;
    st->st_uid = 1;
    st->st_gid = 1;

    if (inode == DEV_INODE_MIN) {
        st->st_mode = S_IFDIR | 0666;
        return 0;
    }

    for (DeviceFile * d = dev_inodes; d; d = d->next) {
        if (d->inode == inode) {
            if (d->symlink) {
                st->st_mode = S_IFLNK | 0666;
                return 0;
            }
            st->st_mode = ((d->ops && d->isatty) ? S_IFCHR : S_IFREG) | 0666;
            if (d->ops && d->getsize)
                st->st_size = d->getsize(d->priv_data);
            return 0;
        }
    }
    return -ENOENT;
}

static int dev_open2(FileDescriptor * fd)
{
    fd->dirty = 0;
    fd->buf = NULL;
    fd->buf_size = 0;

    for (DeviceFile * d = dev_inodes; d; d = d->next)
        if (d->inode == fd->inode) {
            fd->isatty = d->isatty;
            fd->ops = d->ops;
            fd->priv_data = d->priv_data;
            return 0;
        }

    return -ENOENT;
}

static int dev_write(FileDescriptor * fd, const void * buf, int size)
{
    return fd->ops->write ? fd->ops->write(fd, buf, size) : -ENOSYS;
}

static int dev_write_available(const FileDescriptor * fd)
{
    return fd->ops->write_available ? fd->ops->write_available(fd) : 1;
}

static off_t dev_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    return fd->ops->lseek ? fd->ops->lseek(fd, offset, whence) : -ENOSYS;
}

static int dev_read(FileDescriptor * fd, void * buf, int size)
{
    return fd->ops->read ? fd->ops->read(fd, buf, size) : -ENOSYS;
}

static int dev_read_available(const FileDescriptor * fd)
{
    return fd->ops->read_available ? fd->ops->read_available(fd) : 1;
}

static int dev_ioctl(FileDescriptor * fd, int request, void * data)
{
    return fd->ops->ioctl ? fd->ops->ioctl(fd, request, data) : -ENOSYS;
}

static int dev_mmap(FileDescriptor * fd, struct os_mmap_request * req)
{
    return fd->ops->mmap ? fd->ops->mmap(fd, req) : -ENOSYS;
}

const FileOperations dev_io = {
    .name = "dev",
    .inode_range = dev_inode_range,
    .inode_resolve = dev_inode_resolve,
    .inode_symlink = dev_inode_symlink,
    .inode_stat = dev_inode_stat,
    .open2 = dev_open2,
    .file = {
        .write = dev_write,
        .write_available = dev_write_available,
        .lseek = dev_lseek,
        .read = dev_read,
        .read_available = dev_read_available,
        .ioctl = dev_ioctl,
        .mmap = dev_mmap,
    },
    .getdents = dev_getdents,
};
