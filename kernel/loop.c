#include <errno.h>
#include <string.h>
#include "loop.h"
#include <unistd.h>
#include <fcntl.h>
#include "vfs.h"
#include "utils.h"

typedef struct {
    FileDescriptor * fd;
    int offset;
} LoopContext;

void * loop_init(const char * path, int offset)
{
    LoopContext * s = kmalloc(sizeof(LoopContext), "loop-cntx");
    if (!s)
        return NULL;
    s->fd = vfs_open(path, O_RDONLY, 0);
    if (!s->fd) {
        kfree(s);
        return NULL;
    }
    s->offset = offset;
    vfs_lseek(s->fd, offset, SEEK_SET);
    return s;
}

static int loop_write(FileDescriptor * fd, const void * buf, int size)
{
    LoopContext * s = fd->priv_data;
    return vfs_write(s->fd, buf, size);
}

static int loop_read(FileDescriptor * fd, void * buf, int size)
{
    LoopContext * s = fd->priv_data;
    return vfs_read(s->fd, buf, size);
}

static off_t loop_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    LoopContext * s = fd->priv_data;
    int ret = vfs_lseek(s->fd, offset + s->offset, whence);
    if (ret <= 0)
        return ret;
    return ret - offset;
}

DeviceOperations loop_dio = {
    .write = loop_write,
    .read  = loop_read,
    .lseek = loop_lseek,
};
