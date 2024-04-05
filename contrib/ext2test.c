#include "vfs.h"
#include "dev.h"
#include "utils.h"
#include "ext2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

struct timespec tnow = {0,0};

int kprintf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}

void khexdump(const void * ptr, unsigned int size)
{
     const uint8_t * buf = ptr;
     for (unsigned i = 0 ; i < size; i += 16) {
         kprintf("%5x:", i);
         for (unsigned int j = i; j < size && j < i + 16; j++) {
             kprintf(" %02x", buf[j]);
         }
         kprintf(" | ");
         for (unsigned int j = i; j < size && j < i + 16; j++) {
             kprintf("%c", buf[j] >= 32 && buf[j] <= 127 ? buf[j] : '.');
         }
         kprintf("\n");
     }
}

void panic(const char * reason)
{
    printf("panic: %s\n", reason);
    exit(1);
}

void * kmalloc(uintptr_t size, const char * tag)
{
    return malloc(size);
}

void kfree(void * ptr)
{
    return free(ptr);
}

void * krealloc(void * buf, unsigned int size)
{
    return realloc(buf, size);
}

void get_absolute_path(char * abspath, int size, const char * path)
{
    strlcpy(abspath, path, size);
}

typedef struct {
    int fd;
} FileContext;

void * file_init(const char * path)
{
    FileContext * cntx = kmalloc(sizeof(FileContext), "mem-cntx");
    if (!cntx)
        return NULL;
    cntx->fd = open(path, O_RDWR);
    printf("file_read[%d]: init\n", cntx->fd);
    if (cntx->fd < 0)
        panic("open failed\n");
    return cntx;
}

static int file_write(FileDescriptor * fd, const void * buf, int size)
{
    FileContext * cntx = fd->priv_data;
    return write(cntx->fd, buf, size);
}

static int file_read(FileDescriptor * fd, void * buf, int size)
{
    FileContext * cntx = fd->priv_data;
    return read(cntx->fd, buf, size);
}

static off_t file_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    FileContext * cntx = fd->priv_data;
    return lseek(cntx->fd, offset, whence);
}

static DeviceOperations file_dio = {
    .write = file_write,
    .read  = file_read,
    .lseek = file_lseek,
};

int main(int argc, char ** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        return -1;
    }

    dev_init();
    vfs_register_mount_point2(2, "dev", &dev_io, NULL, DEV_INODE_MIN);

    void * file = file_init(argv[1]);
    if (!file)
        panic("file_init failed");
    dev_register_device("module", &file_dio, 0, NULL, file);

    void * ext2 = ext2_init("/dev/module");
    vfs_register_mount_point2(1, "", &ext2_io, ext2, 2);

#if 1
    if (vfs_unlink("/README.md", 0) < 0)
        fprintf(stderr, "warning: unlink failed\n");
#endif

#if 1
    FileDescriptor * fd;
    if (!(fd = vfs_open("/README.txt", O_CREAT, 0))) {
        printf("vfs_open failed\n");
        return -1;
    }
    vfs_write(fd, "Hello\n", 6);
    vfs_close(fd);
#endif

#if 1
    vfs_mkdir("/test", 0);
#endif

    printf("ok\n");

    return 0;
}
