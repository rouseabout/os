#include <stddef.h>
#include <string.h>

enum {
    RB_READ = 0,
    RB_WRITE,
    RB_NB_MODES
};

typedef struct {
    char * buffer;
    unsigned int size;
    unsigned int position[RB_NB_MODES];
    unsigned int mask;
    unsigned int mask2;
} RingBuffer;

static inline void ringbuffer_init(RingBuffer * rb, char * buffer, unsigned int size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->position[RB_READ] = rb->position[RB_WRITE] = 0;
    rb->mask = size - 1;
    rb->mask2 = size * 2 - 1;
}

static inline unsigned int ringbuffer_read_available(const RingBuffer * rb)
{
    return (rb->position[RB_WRITE] - rb->position[RB_READ]) & rb->mask2;
}

static inline unsigned int ringbuffer_write_available(const RingBuffer * rb)
{
    return rb->size - ringbuffer_read_available(rb);
}

static inline unsigned int ringbuffer_available(const RingBuffer * rb, int mode)
{
    return mode == RB_READ ? ringbuffer_read_available(rb) : ringbuffer_write_available(rb);
}

static inline char * ringbuffer_where1(RingBuffer * rb, int mode)
{
    unsigned int available = ringbuffer_available(rb, mode);
    if (!available)
        return NULL;
    unsigned int position = rb->position[mode] & rb->mask;
    return rb->buffer + position;
}

static inline int ringbuffer_where(RingBuffer * rb, int mode, int size, char ** data1, unsigned int * size1, char ** data2, unsigned int * size2)
{
    unsigned int available = ringbuffer_available(rb, mode);
    if (size > available)
        size = available;
    unsigned int pos = rb->position[mode] & rb->mask;
    *data1 = rb->buffer + pos;
    if (pos + size <= rb->size) {
        *size1 = size;
        *data2 = NULL;
        *size2 = 0;
    } else {
        *size1 = rb->size - pos;
        *data2 = rb->buffer;
        *size2 = size - *size1;
    }
    return size;
}

static inline void ringbuffer_advance(RingBuffer * rb, int mode, unsigned int size)
{
    rb->position[mode] = (rb->position[mode] + size) & rb->mask2;
}

static inline void ringbuffer_write1(RingBuffer * rb, int c)
{
    char * ptr = ringbuffer_where1(rb, RB_WRITE);
    if (!ptr)
        return;
    *ptr = c;
    ringbuffer_advance(rb, RB_WRITE, 1);
}

static inline unsigned int ringbuffer_write(RingBuffer * rb, const char * buf, unsigned int size)
{
    char * data1, * data2;
    unsigned int size1, size2;
    size = ringbuffer_where(rb, RB_WRITE, size, &data1, &size1, &data2, &size2);
    memcpy(data1, buf, size1);
    if (size2)
        memcpy(data2, buf + size1, size2);
    ringbuffer_advance(rb, RB_WRITE, size);
    return size;
}

static inline int ringbuffer_read1(RingBuffer * rb)
{
    char * ptr = ringbuffer_where1(rb, RB_READ);
    if (!ptr)
        return -1;
    ringbuffer_advance(rb, RB_READ, 1);
    return *ptr;
}

static inline unsigned int ringbuffer_read(RingBuffer * rb, char * buf, unsigned int size)
{
    char * data1, * data2;
    unsigned int size1, size2;
    size = ringbuffer_where(rb, RB_READ, size, &data1, &size1, &data2, &size2);
    memcpy(buf, data1, size1);
    if (size2)
        memcpy(buf + size1, data2, size2);
    ringbuffer_advance(rb, RB_READ, size);
    return size;
}
