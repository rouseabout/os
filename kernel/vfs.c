#include "vfs.h"
#include "utils.h"
#include <stddef.h>
#include <string.h>
#include <bsd/string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include "pipe.h"

#ifdef TEST
static char * basename_safe(char * s) { return basename(strdup(s)); }
static char * dirname_safe(char * s) { return dirname(strdup(s)); }
#ifdef basename
#undef basename
#endif
#define basename(x) basename_safe(x)
#ifdef dirname
#undef dirname
#endif
#define dirname(x) dirname_safe(x)
#endif

static Mount * mount_points = NULL;

static Mount * find_mount_inode(int inode)
{
    for (Mount *d = mount_points; d; d = d->next) {
        int min_inode, max_inode;
        if (d->ops->inode_range && d->ops->inode_range(d->priv_data, &min_inode, &max_inode) >= 0 && inode >= min_inode && inode <= max_inode)
            return d;
    }
    return NULL;
}

void vfs_register_mount_point2(int dinode, const char * name, const FileOperations * ops, void * priv_data, int root_inode)
{
    Mount * m = kmalloc(sizeof(Mount), "mountpoint2");
    m->dinode = dinode;
    strlcpy(m->name, name, sizeof(m->name));
    m->ops = ops;
    m->priv_data = priv_data;
    m->root_inode = root_inode;
    m->next = NULL;

    // add to tail
    Mount ** mp;
    for (mp = &mount_points; *mp; mp = &(*mp)->next) ;
    *mp = m;
}

static int find_mount_point_at(int dinode, const char * name)
{
    for (const Mount * m = mount_points; m; m = m->next)
        if (m->dinode == dinode && !strcmp(m->name, name))
            return m->root_inode;
    return -ENOENT;
}

static int resolve_inode2(int initial_inode, int make_abs, const char * path, int resolve_basename, Mount ** mount_ptr, int * dinode_ptr, char * rpath)
{
    if (!strlen(path))
        return -ENOENT;

    char abspath[PATH_MAX];
    if (make_abs)
        get_absolute_path(abspath, sizeof(abspath), path);
    else
        strlcpy(abspath, path, sizeof(abspath));
    char * p = abspath;
    int run, inode = initial_inode;

    // trim any trailing slashes
    for (char *q = p + strlen(p) - 1; *q == '/'; q--) *q = 0;

    do {
        while (*p && *p == '/') p++;
        char *q = p;
        while (*q && *q != '/') q++;
        run = *q;
        *q = 0;

        if (!*p) { // root
            *mount_ptr = find_mount_inode(inode);
            if (!*mount_ptr)
                return -ENODEV;
            if (dinode_ptr)
                *dinode_ptr = 1;
            return inode;
        }

        int new_inode;

        new_inode = find_mount_point_at(inode, p);
        if (new_inode < 0) {
            *mount_ptr = find_mount_inode(inode);
            if (!*mount_ptr)
                return -ENODEV;
            new_inode = (*mount_ptr)->ops->inode_resolve((*mount_ptr)->priv_data, inode, p);
            if (new_inode < 0)
                return new_inode;
        }

        *mount_ptr = find_mount_inode(new_inode);
        if (!*mount_ptr)
            return -ENODEV;

        char path2[PATH_MAX];
        if ((run || resolve_basename) && (*mount_ptr)->ops->inode_symlink && (*mount_ptr)->ops->inode_symlink((*mount_ptr)->priv_data, new_inode, path2, sizeof(path2)) > 0) {
            if (run) {
                int size = strlen(path2);
                path2[size] = '/';
                strlcpy(path2 + size + 1, q + 1, PATH_MAX - size - 1);
            }
            memcpy(abspath, path2, PATH_MAX);
            p = abspath;
            run = 1;
            if (abspath[0] == '/') {
                if (rpath)
                    rpath[0] = 0;
                inode = 2;
            }
            continue;
        }

        if (rpath) {
            strlcat(rpath, "/", PATH_MAX);
            strlcat(rpath, p, PATH_MAX);
        }
        if (dinode_ptr)
            *dinode_ptr = inode;
        inode = new_inode;
        q++;
        p = q;
    } while(run);

    return inode;
}

static int resolve_inode(const char * path, int resolve_basename, Mount ** mount_ptr, int * dinode_ptr, char * rpath)
{
    return resolve_inode2(2, 1, path, resolve_basename, mount_ptr, dinode_ptr, rpath);
}

static int vfs_inode_creat(const char * path, mode_t mode, const char * symbolic_link, Mount ** mount_ptr, int * dinode_ptr, char * rpath)
{
    char * dir = dirname((char *)path); //our dirname/basename functions are not destructive
    char * base = basename((char *)path);
    kprintf("vfs_creat: [%s], dir='%s', base='%s'\n", path, dir, base);

    Mount * mount = NULL;
    int dinode = resolve_inode(dir, 1, &mount, NULL, rpath);
    if (dinode < 0)
        return -ENOENT;

    if (!mount->ops->inode_creat)
        return -ENOSYS;

    int inode = mount->ops->inode_creat(mount->priv_data, dinode, base, mode, symbolic_link);
    if (inode < 0)
        return inode;

    if (mount_ptr) *mount_ptr = mount;
    if (dinode_ptr) *dinode_ptr = dinode;

    return inode;
}

static int flags2fmt(int flags)
{
    if ((flags & O_DIRECTORY))
        return S_IFDIR;
    else
        return S_IFREG;
}

FileDescriptor * vfs_open(const char * path, int flags, mode_t mode)
{
    Mount * mount = NULL;
    char rpath[PATH_MAX];
    rpath[0] = 0;
    int inode = resolve_inode(path, 1, &mount, NULL, rpath);
    if (inode < 0) {
        if (!(flags & O_CREAT))
            return NULL;
        kprintf("vfs_creat: [%s]\n", path);
        rpath[0] = 0;
        inode = vfs_inode_creat(path, flags2fmt(flags) | mode, NULL, NULL, NULL, rpath);
        if (inode < 0)
            return NULL;
    }

    struct stat st;
    if (mount->ops->inode_stat && mount->ops->inode_stat(mount->priv_data, inode, &st) == 0 && !!(st.st_mode & S_IFDIR) != !!(flags & O_DIRECTORY))
        return NULL;

    FileDescriptor * fd = kmalloc(sizeof(FileDescriptor), "fd");
    fd->refcount = 1;
    fd->mount = mount;
    fd->ops = &mount->ops->file;
    fd->priv_data = mount->priv_data;
    fd->inode = inode;
    fd->pos = 0;
    fd->mount_pos = -1;
    fd->flags = flags;
    fd->fd_flags = 0;
    memcpy(fd->path, rpath, PATH_MAX);

    if (mount->ops->open2)
        if (mount->ops->open2(fd) < 0) {
            kfree(fd);
            return NULL;
        }

    if ((flags & O_APPEND) && fd->ops->lseek)
        fd->ops->lseek(fd, 0, SEEK_END);

    return fd;
}

void vfs_close(FileDescriptor * fd)
{
    if (fd->ops->close)
        fd->ops->close(fd);

    fd->refcount--;
    if (!fd->refcount)
        kfree(fd);
}

int vfs_write(FileDescriptor * fd, const void * buf, int size)
{
    if (!fd->ops->write)
        return 0;
    return fd->ops->write(fd, buf, size);
}

int vfs_read(FileDescriptor * fd, void * buf, int size)
{
    if (!fd->ops->read)
        return 0;
    return fd->ops->read(fd, buf, size);
}

int vfs_dup_fds(FileDescriptor ** dst, FileDescriptor ** src, int count)
{
    for (int i = 0; i < count; i++) {
        if (src[i]) {
            src[i]->refcount++;
            if (src[i]->ops->refcount)
                src[i]->ops->refcount(src[i], 1);
        }
        dst[i] = src[i];
    }
    return 0;
}

off_t vfs_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    if (!fd->ops->lseek)
        return -ENOSYS;

    return fd->ops->lseek(fd, offset, whence);
}

int vfs_getdents(FileDescriptor * fd, struct dirent * dent, size_t count)
{
    if (fd->mount_pos == -1) {
        if (fd->mount && fd->mount->ops->getdents) {
            int ret = fd->mount->ops->getdents(fd, dent, count);
            if (ret)
                return ret;
        }
        fd->mount_pos++;
    }

    int i = 0;
    int mcount = 0;
    for (Mount * m = mount_points; m && i < count; m = m->next) {
        if (m->dinode == fd->inode) {
            if (mcount < fd->mount_pos) {
                mcount++;
                continue;
            }
            dent[i].d_ino = m->root_inode;
            strlcpy(dent[i].d_name, m->name, sizeof(dent[i].d_name));
            i++;
            mcount++;
        }
    }

    fd->mount_pos += i;
    return i;
}

int vfs_stat(FileDescriptor * fd, const char * path, struct stat * st, int resolve_link)
{
    Mount * mount;
    int inode = resolve_inode2(fd ? fd->inode : 2, !fd, path, resolve_link, &mount, NULL, NULL);
    if (inode < 0)
        return -ENOENT;
    return mount->ops->inode_stat ? mount->ops->inode_stat(mount->priv_data, inode, st) : -ENOSYS;
}

int vfs_unlink(const char * path, int is_dir)
{
    Mount * mount;
    int dinode;
    int inode = resolve_inode(path, 0, &mount, &dinode, NULL);
    if (inode < 0)
        return inode;
    return mount->ops->inode_unpopulate_dir ? mount->ops->inode_unpopulate_dir(mount->priv_data, dinode, inode, is_dir, -1) : -ENOSYS;
}

int vfs_mkdir(const char *path, mode_t mode)
{
    Mount * mount;
    int inode = resolve_inode(path, 1, &mount, NULL, NULL);
    if (inode >= 0)
        return -EEXIST;

    int dinode;
    inode = vfs_inode_creat(path, S_IFDIR | mode, NULL, &mount, &dinode, NULL);
    if (inode < 0)
        return -EIO;

    return mount->ops->inode_populate_dir(mount->priv_data, dinode, inode);
}

int vfs_ioctl(FileDescriptor * fd, int request, void * data)
{
    if (!fd->ops->ioctl)
        return -ENOSYS;

    return fd->ops->ioctl(fd, request, data);
}

int vfs_mmap(FileDescriptor * fd, struct os_mmap_request * req)
{
    if (!fd->ops->mmap)
        return -ENOSYS;

    return fd->ops->mmap(fd, req);
}

int vfs_fstat(FileDescriptor * fd, struct stat * st)
{
    if (!fd->mount || !fd->mount->ops->inode_stat) {
        memset(st, 0, sizeof(*st)); /* pipe, socket */
        return 0;
    }

    return fd->mount->ops->inode_stat(fd->priv_data, fd->inode, st);
}

int vfs_read_available(const FileDescriptor * fd)
{
    if (!fd->ops->read_available)
        return 1; /* always available */

    return fd->ops->read_available(fd);
}

int vfs_write_available(const FileDescriptor * fd)
{
    if (!fd->ops->write_available)
        return 1; /* always available */

    return fd->ops->write_available(fd);
}

int vfs_pipe(FileDescriptor ** rfd_ptr, FileDescriptor ** wfd_ptr)
{
    PipeContext * pipe = pipe_create(2048);

    FileDescriptor * rfd = kmalloc(sizeof(FileDescriptor), "fd-r-pipe");
    rfd->refcount = 1;
    rfd->mount = NULL;
    rfd->ops = &pipe_reader_dio;
    rfd->priv_data = pipe;
    rfd->inode = 0;
    rfd->pos = 0;
    rfd->flags = 0;
    rfd->fd_flags = 0;
    rfd->isatty = 0;
    snprintf(rfd->path, sizeof(rfd->path), "(pipe-%p-read)", pipe);

    FileDescriptor * wfd = kmalloc(sizeof(FileDescriptor), "fd-w-pipe");
    wfd->refcount = 1;
    wfd->mount = NULL;
    wfd->ops = &pipe_writer_dio;
    wfd->priv_data = pipe;
    wfd->inode = 0;
    wfd->pos = 0;
    wfd->flags = 0;
    wfd->fd_flags = 0;
    wfd->isatty = 0;
    snprintf(wfd->path, sizeof(wfd->path), "(pipe-%p-write)", pipe);

    *rfd_ptr = rfd;
    *wfd_ptr = wfd;
    return 0;
}

int vfs_direct(const FileDescriptor * fd)
{
    return fd->isatty || fd->ops == &pipe_reader_dio;
}

int vfs_utime(const char * path, const struct utimbuf * times)
{
    Mount * mount;
    int inode = resolve_inode(path, 1, &mount, NULL, NULL);
    if (inode < 0)
        return inode;

    return mount->ops->inode_utime ? mount->ops->inode_utime(mount->priv_data, inode, times) : -ENOSYS;
}

int vfs_chmod(const char * path, mode_t mode)
{
    Mount * mount;
    int inode = resolve_inode(path, 1, &mount, NULL, NULL);
    if (inode < 0)
        return inode;
    return mount->ops->inode_chmod ? mount->ops->inode_chmod(mount->priv_data, inode, mode) : -ENOSYS;
}

static int unlink_if_file(Mount * mount, int inode, const char * path)
{
    int ret;
    struct stat st;

    if (!mount->ops->inode_stat)
        return -ENOSYS;

    ret = mount->ops->inode_stat(mount->priv_data, inode, &st);
    if (ret < 0)
        return ret;

    int is_dir = (st.st_mode & S_IFMT) == S_IFDIR;
    if (is_dir)
        return -EEXIST;

    return vfs_unlink(path, 0);
}

int vfs_rename(const char * old, const char * new)
{
    if (!strlen(old))
        return -ENOENT;

    Mount * oldmount;
    int olddinode;
    int inode = resolve_inode(old, 1, &oldmount, &olddinode, NULL);
    if (inode < 0)
        return -ENOENT;

    Mount * tmpmount;
    int tmpinode = resolve_inode(new, 1, &tmpmount, NULL, NULL);
    if (tmpinode >= 0) {
        // FIXME: if new is dir, move old inode into it
        int ret = unlink_if_file(tmpmount, tmpinode, new);
        if (ret < 0)
            return ret;
    }

    Mount * newmount;
    char * newdir = dirname((char *)new); //our dirname/basename functions are not destructive
    char * newbase = basename((char *)new);
    int newdinode = resolve_inode(newdir, 1, &newmount, NULL, NULL);
    if (newdinode < 0)
        return -ENOENT;

    if (oldmount != newmount)
        return -EINVAL; //FIXME: support move across mount points

    if (!oldmount->ops->inode_stat || !oldmount->ops->inode_populate_dir || !oldmount->ops->inode_unpopulate_dir)
        return -ENOSYS;

    int ret;
    struct stat st;
    ret = oldmount->ops->inode_stat(oldmount->priv_data, inode, &st);
    if (ret < 0)
        return ret;

    int is_dir = (st.st_mode & S_IFMT) == S_IFDIR;

    ret = oldmount->ops->inode_unpopulate_dir(oldmount->priv_data, olddinode, inode, is_dir, 0);
    if (ret < 0)
        return -EIO; //FIXME: no rollback

    ret = oldmount->ops->inode_append_dir(oldmount->priv_data, newdinode, inode, newbase, is_dir);
    if (ret < 0)
        return -EIO; //FIXME: no rollback

    return 0;
}

int vfs_getmntinfo(struct statvfs * mntbufp, int size)
{
#if TEST
    return -ENOSYS;
#else
    int i = 0;
    for (Mount *d = mount_points; d && (!mntbufp || i < size); d = d->next, i++) {
        if (mntbufp) {
            memset(&mntbufp[i], 0, sizeof(struct statvfs));
            strlcpy(mntbufp[i].f_fstypename, d->ops->name, sizeof(mntbufp[i].f_fstypename));
            strlcpy(mntbufp[i].f_mntfromname, d->ops->name, sizeof(mntbufp[i].f_mntfromname));
            snprintf(mntbufp[i].f_mntonname, sizeof(mntbufp[i].f_mntonname), "/%s", d->name);
            if (d->ops->usage) {
                d->ops->usage(d->priv_data, &mntbufp[i].f_bsize, &mntbufp[i].f_blocks, &mntbufp[i].f_bfree);
            } else {
                mntbufp[i].f_bsize = 0;
                mntbufp[i].f_blocks = 0;
                mntbufp[i].f_bfree = 0;
            }
        }
    }
    return i;
#endif
}

ssize_t vfs_readlink(const char * path, char * buf, size_t bufsize)
{
    Mount * mount;
    int inode = resolve_inode(path, 0, &mount, NULL, NULL);
    if (inode < 0)
        return -ENOENT;

    return mount->ops->inode_symlink ? mount->ops->inode_symlink(mount->priv_data, inode, buf, bufsize) : -ENOSYS;
}

int vfs_link(const char * srcpath, const char * dstpath, int symbolic)
{
    Mount * srcmount;
    int inode = resolve_inode(srcpath, 0, &srcmount, NULL, NULL);
    if (inode < 0)
        return -ENOENT;

    if (!srcmount->ops->inode_stat)
        return -ENOSYS;

    struct stat st;
    int ret = srcmount->ops->inode_stat(srcmount->priv_data, inode, &st);
    if (ret < 0)
        return ret;

    if (symbolic) {
        int dstinode = vfs_inode_creat(dstpath, (st.st_mode & ~S_IFMT) | S_IFLNK, srcpath, NULL, NULL, NULL);
        if (dstinode < 0)
            return dstinode;
    } else {
        char absdst[PATH_MAX];
        get_absolute_path(absdst, sizeof(absdst), dstpath);

        char * dstdir = dirname((char *)absdst); //our dirname/basename functions are not destructive
        char * dstbase = basename((char *)absdst);

        Mount * dstmount;
        int dinode = resolve_inode(dstdir, 0, &dstmount, NULL, NULL);
        if (dinode < 0)
            return -ENOENT;

        if (srcmount != dstmount)
            return -EINVAL;

        if (!srcmount->ops->inode_append_dir || !srcmount->ops->inode_increment_links_count)
            return -ENOSYS;

        ret = srcmount->ops->inode_append_dir(srcmount->priv_data, dinode, inode, dstbase, (st.st_mode & S_IFMT) == S_IFDIR);
        if (ret < 0)
            return ret;

        ret = srcmount->ops->inode_increment_links_count(srcmount->priv_data, inode);
        if (ret < 0)
            return ret;
    }

    return 0;
}

int vfs_truncate(FileDescriptor * fd, off_t length)
{
    if (!fd->ops->truncate)
        return -ENOSYS;

    return fd->ops->truncate(fd, length);
}

enum {
    SOCKET_STATE_FREE = 0,
    SOCKET_STATE_BUSY,
    SOCKET_STATE_ACCEPTING,
    SOCKET_STATE_CONNECTED,
    NB_SOCKET_STATES
};

static struct {
    int state;
    char bind_name[256]; /* matches sys/un.h sun_path */
    int other_idx;
    FileDescriptor * fd;
} sockets[32] = {0};

typedef struct {
    char buffer[4096];
    RingBuffer rb;
} SocketContext;

FileDescriptor * vfs_socket_get_other(FileDescriptor * fd)
{
    int other_idx = sockets[fd->inode].other_idx;
    if (other_idx < 0)
        return NULL;
    return sockets[other_idx].fd;
}

static int socket_write(FileDescriptor * fd, const void * buf, int size)
{
    FileDescriptor * other_fd = vfs_socket_get_other(fd);
    if (!other_fd)
        return -ENOTCONN;
    SocketContext * other_cntx = other_fd->priv_data;
    return ringbuffer_write(&other_cntx->rb, buf, size);
}

static int socket_read(FileDescriptor * fd, void * buf, int size)
{
    SocketContext * cntx = fd->priv_data;
    int ret = ringbuffer_read(&cntx->rb, buf, size);
    return ret ? ret : (sockets[fd->inode].other_idx < 0 ? 0 : -EAGAIN);
}

static void socket_close(FileDescriptor * fd)
{
    SocketContext * cntx = fd->priv_data;
    kfree(cntx);
    sockets[fd->inode].state = SOCKET_STATE_FREE;
    sockets[fd->inode].bind_name[0] = 0;
    sockets[sockets[fd->inode].other_idx].other_idx = -1;
}

const DeviceOperations socket_dio = {
    .write = socket_write,
    .read = socket_read,
    .close = socket_close,
};

FileDescriptor * vfs_socket(FileDescriptor * other_fd)
{
    for (int i = 0; i < NB_ELEMS(sockets); i++)
        if (sockets[i].state == SOCKET_STATE_FREE) {
            FileDescriptor * fd = kmalloc(sizeof(FileDescriptor), "fd-socket");
            fd->refcount = 1;
            fd->mount = NULL;
            fd->ops = &socket_dio;
            fd->priv_data = kmalloc(sizeof(SocketContext), "socket-cntx");
            fd->inode = i;
            fd->pos = 0;
            fd->flags = 0;
            fd->fd_flags = 0;
            fd->isatty = 1;
            snprintf(fd->path, sizeof(fd->path), "(socket-%d)", i);

            SocketContext * cntx = fd->priv_data;
            ringbuffer_init(&cntx->rb, cntx->buffer, sizeof(cntx->buffer));

            sockets[i].state = SOCKET_STATE_BUSY;
            sockets[i].fd = fd;
            if (other_fd) {
                sockets[i].other_idx = other_fd->inode;
                sockets[other_fd->inode].other_idx = i;
            } else {
                sockets[i].other_idx = -1;
            }
            return fd;
        }
    return NULL;
}

int vfs_bind(FileDescriptor * fd, const char * name)
{
    if (fd->ops != &socket_dio)
        return -ENOTSOCK;
    for (int i = 0; i < NB_ELEMS(sockets); i++)
        if (!strcmp(sockets[i].bind_name, name))
            return -EADDRINUSE;
    if (sockets[fd->inode].bind_name[0])
        return -EINVAL; /* already bound */
    strlcpy(sockets[fd->inode].bind_name, name, sizeof(sockets[0].bind_name));
    return 0;
}

int vfs_accept(FileDescriptor * fd)
{
    if (fd->ops != &socket_dio)
        return -ENOTSOCK;
    sockets[fd->inode].state = SOCKET_STATE_ACCEPTING;
    return 0;
}

int vfs_is_connected(FileDescriptor * fd)
{
    return sockets[fd->inode].state == SOCKET_STATE_CONNECTED;
}

int vfs_connect(FileDescriptor * fd, const char * name)
{
    if (fd->ops != &socket_dio)
        return -ENOTSOCK;
    for (int i = 0; i < NB_ELEMS(sockets); i++)
        if (sockets[i].state == SOCKET_STATE_ACCEPTING && !strcmp(sockets[i].bind_name, name)) {
            sockets[i].state = SOCKET_STATE_CONNECTED;
            sockets[i].other_idx = fd->inode;
            sockets[fd->inode].state = SOCKET_STATE_CONNECTED;
            return 0;
        }
    return -ECONNREFUSED;
}

void vfs_dump_sockets(void)
{
    for (int i = 0; i < NB_ELEMS(sockets); i++)
        if (sockets[i].state != SOCKET_STATE_FREE) {
            kprintf("socket[%d] state=%d bind_name:'%s'\n", i, sockets[i].state, sockets[i].bind_name);
        }
    kprintf("\n");
}

/* misc devices */

const DeviceOperations null_dio = {0};

static int zero_read(FileDescriptor * fd, void * buf, int size)
{
    memset(buf, 0, size);
    return size;
}

const DeviceOperations zero_dio = {.read = zero_read};

static unsigned int g_seed = 123456789;

static long random()
{
    g_seed = 1103515245 * g_seed + 12345;
    return g_seed & 0x7fffffff;
}

static int urandom_read(FileDescriptor * fd, void * buf_, int size)
{
    uint8_t * buf = buf_;
    for (int i = 0; i < size; i++)
        buf[i] = random() % (i + 256);
    return size;
}

const DeviceOperations urandom_dio = {.read = urandom_read};
