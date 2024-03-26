#include "ringbuffer.h"
#include "vfs.h"

typedef struct {
    RingBuffer rbuf;
    int writer_open;
    int reader_open;
} PipeContext;

PipeContext * pipe_create(int size);

extern const DeviceOperations pipe_writer_dio;
extern const DeviceOperations pipe_reader_dio;
