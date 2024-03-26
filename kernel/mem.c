#include <errno.h>
#include <string.h>
#include "mem.h"
#include <unistd.h>
#include "utils.h"

typedef struct {
    char * start;
    ssize_t size;
} MemContext;

void * mem_init(void * start, size_t size)
{
    MemContext * cntx = kmalloc(sizeof(MemContext), "mem-cntx");
    if (!cntx)
        return NULL;
    cntx->start = start;
    cntx->size = size;
    return cntx;
}

static int mem_write_dio(FileDescriptor * fd, const void * buf, int size)
{
    MemContext * cntx = fd->priv_data;

    size_t sz = MIN(cntx->size - fd->pos, size);
    memcpy(cntx->start + fd->pos, buf, sz);

    fd->pos += sz;
    return sz;
}

static int mem_read_dio(FileDescriptor * fd, void * buf, int size)
{
    MemContext * cntx = fd->priv_data;

    size_t sz = MIN(cntx->size - fd->pos, size);
    memcpy(buf, cntx->start + fd->pos, sz);

    fd->pos += sz;
    return sz;
}

static off_t mem_lseek_dio(FileDescriptor * fd, off_t offset, int whence)
{
    MemContext * cntx = fd->priv_data;

    if (whence == SEEK_END)
        fd->pos = cntx->size;
    else if (whence == SEEK_CUR)
        fd->pos = MIN(cntx->size, MAX(0, fd->pos + offset));
    else if (whence == SEEK_SET)
        fd->pos = MIN(cntx->size, MAX(0, offset));

    return fd->pos;
}

int mem_getsize(void * priv_data)
{
    MemContext * cntx = priv_data;
    return cntx->size;
}

DeviceOperations mem_dio = {
    .write = mem_write_dio,
    .read  = mem_read_dio,
    .lseek = mem_lseek_dio,
};
