#include <errno.h>
#include <stdint.h>
#include "pipe.h"
#include "utils.h"

PipeContext * pipe_create(int size)
{
    PipeContext * s = kmalloc(sizeof(PipeContext) + size, "pipe-cntx");
    if (!s)
        return NULL;
    ringbuffer_init(&s->rbuf, (char *)(s + 1), size);
    s->writer_open = 1;
    s->reader_open = 1;
    return s;
}

static int pipe_write_available(const FileDescriptor * fd)
{
    const PipeContext * s = fd->priv_data;
    return ringbuffer_write_available(&s->rbuf);
}

static int pipe_write(FileDescriptor * fd, const void * buf, int size)
{
    PipeContext * s = fd->priv_data;
    unsigned int i = ringbuffer_write(&s->rbuf, buf, size);
    return i ? i : ((s->writer_open > 0 && s->reader_open > 0) ? -EAGAIN : 0);
}

static int pipe_read_available(const FileDescriptor * fd)
{
    const PipeContext * s = fd->priv_data;
    return ringbuffer_read_available(&s->rbuf);
}

static int pipe_read(FileDescriptor * fd, void * buf, int size)
{
    PipeContext * s = fd->priv_data;
    unsigned int i = ringbuffer_read(&s->rbuf, buf, size);
    return i ? i : ((s->writer_open > 0 && s->reader_open > 0) ? -EAGAIN : 0);
}

static void pipe_refcount(FileDescriptor * fd, int delta)
{
    PipeContext * s = fd->priv_data;
    if (fd->ops == &pipe_writer_dio)
        s->writer_open += delta;
    else
        s->reader_open += delta;
}

static void pipe_close(FileDescriptor * fd)
{
    PipeContext * s = fd->priv_data;
    pipe_refcount(fd, -1);
    if (s->writer_open + s->reader_open == 0)
        kfree(s);
}

const DeviceOperations pipe_writer_dio = {
    .refcount = pipe_refcount,
    .write_available  = pipe_write_available,
    .write  = pipe_write,
    .close = pipe_close,
};

const DeviceOperations pipe_reader_dio = {
    .refcount = pipe_refcount,
    .read_available  = pipe_read_available,
    .read  = pipe_read,
    .close = pipe_close,
};
