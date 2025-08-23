#include "proc.h"
#include "utils.h"
#include <bsd/string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PROC_INODE_FD_BASE   (PROC_INODE_MIN + 100)

typedef struct ProcFile ProcFile;

struct ProcFile {
    int dinode;
    int inode;
    const char * name;
    void (*cb)(FileDescriptor * fd);
    ProcFile * next;
};

static ProcFile * proc_inodes = NULL;
static int proc_inode_next = PROC_INODE_MIN + 3;

static void add_proc_inode(int dinode, int inode, const char * name, void (*cb)(FileDescriptor * fd))
{
    ProcFile * d = kmalloc(sizeof(ProcFile), "proc-file");
    d->dinode = dinode;
    d->inode = inode;
    d->name = name;
    d->cb = cb;
    d->next = NULL;

    ProcFile ** dp;
    for (dp = &proc_inodes; *dp; dp = &(*dp)->next) ;
    *dp = d;
}

void proc_init()
{
    add_proc_inode(PROC_INODE_MIN, PROC_INODE_MIN, ".", NULL);
    add_proc_inode(PROC_INODE_MIN, 2, "..", NULL);
    add_proc_inode(PROC_INODE_MIN, PROC_INODE_MIN + 1, "self", NULL);

    add_proc_inode(PROC_INODE_MIN + 1, PROC_INODE_MIN + 1, ".", NULL);
    add_proc_inode(PROC_INODE_MIN + 1, PROC_INODE_MIN, "..", NULL);
    add_proc_inode(PROC_INODE_MIN + 1, PROC_INODE_MIN + 2, "fd", NULL);

    add_proc_inode(PROC_INODE_MIN + 2, PROC_INODE_MIN + 2, ".", NULL);
    add_proc_inode(PROC_INODE_MIN + 2, PROC_INODE_MIN + 1, "..", NULL);
}

void proc_register_file(const char * name, void (*cb)(FileDescriptor * fd))
{
    add_proc_inode(PROC_INODE_MIN, proc_inode_next++, name, cb);
}

FileDescriptor ** get_current_task_fds(void);

static int proc_getdents(FileDescriptor * fd, struct dirent * dent, size_t count)
{
    int i = 0;
    for (ProcFile * d = proc_inodes; d; d = d->next) {
        if (d->dinode != fd->inode)
            continue;
        if (i == fd->pos) {
            dent[0].d_ino = d->inode;
            dent[0].d_off = 0;
            dent[0].d_reclen = sizeof(struct dirent);
            strlcpy(dent[0].d_name, d->name, sizeof(dent[0].d_name));
            fd->pos++;
            return 1;
        }
        i++;
    }

    FileDescriptor ** fds = get_current_task_fds();

    if (fd->inode == PROC_INODE_MIN + 2) {
        for (int j = 0; j < OPEN_MAX; j++) {
            if (!fds[j])
                continue;
            if (i - 2 == fd->pos) {
                dent[0].d_ino = PROC_INODE_FD_BASE + j;
                snprintf(dent[0].d_name, sizeof(dent[0].d_name), "%d", j);
                fd->pos++;
                return 1;
            }
            i++;
        }
    }
    return 0;
}

static int proc_inode_range(void * priv_data, int * min_inode, int * max_inode)
{
    *min_inode = PROC_INODE_MIN;
    *max_inode = PROC_INODE_MAX;
    return 0;
}

static int proc_inode_resolve(void * priv_data, int dinode, const char * target)
{
    int fd;
    if (dinode == PROC_INODE_MIN + 2 && sscanf(target, "%d", &fd) == 1) {
        return PROC_INODE_FD_BASE + fd;
    }

    for (ProcFile * d = proc_inodes; d; d = d->next)
        if (d->dinode == dinode && !strcmp(d->name, target))
            return d->inode;

    return -ENOENT;
}

static int proc_inode_stat(void * priv_data, int inode, struct stat * st)
{
    memset(st, 0, sizeof(struct stat));
    st->st_ino = inode;
    st->st_uid = 1;
    st->st_gid = 1;

    if (inode >= PROC_INODE_FD_BASE && inode < PROC_INODE_FD_BASE + OPEN_MAX) {
        st->st_mode = S_IFLNK | 0666;
        return 0;
    }

    for (ProcFile * d = proc_inodes; d; d = d->next)
        if (d->inode == inode) {
            st->st_mode = (d->cb ? S_IFCHR : S_IFDIR) | 0666;
            return 0;
        }

    return -ENOENT;
}

static int proc_inode_symlink(void * priv_data, int inode, char * buf, size_t size)
{
    if (inode >= PROC_INODE_FD_BASE && inode < PROC_INODE_FD_BASE + OPEN_MAX) {
        FileDescriptor * fd = get_current_task_fds()[inode - PROC_INODE_FD_BASE];
        if (fd)
            return strlcpy(buf, fd->path, size);
    }

    return 0;
}

static int proc_open2(FileDescriptor * fd)
{
    fd->dirty = 0;
    fd->buf = NULL;
    fd->buf_size = 0;

    kprintf("proc_open2: inode=%d\n", fd->inode);

    for (ProcFile * d = proc_inodes; d; d = d->next)
        if (d->inode == fd->inode) {
            if (d->cb)
                d->cb(fd);
            return 0;
        }

    return -ENOENT;
}

static int proc_read(FileDescriptor * fd, void * buf, int size)
{
    size = MAX(MIN(size, fd->buf_size - fd->pos), 0);
    memcpy(buf, fd->buf + fd->pos, size);
    fd->pos += size;
    return size;
}

static off_t proc_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    if (whence == SEEK_END)
        fd->pos = MAX(0, fd->buf_size + offset);
    else if (whence == SEEK_CUR)
        fd->pos = MAX(0, fd->pos + offset);
    else if (whence == SEEK_SET)
        fd->pos = MAX(0, offset);
    else
        return -EINVAL;
    return fd->pos;
}

const FileOperations proc_io = {
    .name = "proc",
    .inode_range = proc_inode_range,
    .inode_resolve = proc_inode_resolve,
    .inode_stat = proc_inode_stat,
    .inode_symlink = proc_inode_symlink,
    .open2 = proc_open2,
    .file = {
        .read = proc_read,
        .lseek = proc_lseek,
    },
    .getdents = proc_getdents,
};
