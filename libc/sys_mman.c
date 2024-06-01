#include <sys/mman.h>
#include <os/syscall.h>
#include <stdlib.h>
#include <syslog.h>

#include <unistd.h>
static MK_SYSCALL1(int, sys_mmap, OS_MMAP, struct os_mmap_request *)
void * mmap(void * addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    if (fildes < 0) {
        void * ptr;
        if (posix_memalign(&ptr, 4096, len) < 0)
            return MAP_FAILED;
        return ptr;
    }

    struct os_mmap_request req = {addr, len, prot, flags, fildes, off};
    int ret = sys_mmap(&req);

    /* user space mmap */
    if (ret < 0) {
        off_t pos = lseek(fildes, 0, SEEK_CUR);
        lseek(fildes, off, SEEK_SET);
        req.addr = malloc(len);
        if (!req.addr)
            return MAP_FAILED;
        read(fildes, req.addr, len);
        lseek(fildes, pos, SEEK_SET);
    }
    return req.addr;
}

int mprotect(void * addr, size_t len, int prot)
{
    syslog(LOG_DEBUG, "libc: mprotect: addr=%p, len=0x%x, prot=0x%x\n", addr, len, prot);
    errno = ENOSYS;
    return -1;
}

int msync(void * addr, size_t len, int flags)
{
    errno = ENOSYS;
    return -1;
}

#include <os/syscall.h>
static MK_SYSCALL2(int, brk, OS_BRK, void *, void **)
extern char end;
int munmap(void * addr, size_t len)
{
    /* user space mmap */
    void *cur;
    brk(NULL, &cur);
    if (addr >= (void *)&end && addr <= cur) {
        free(addr);
        return 0;
    }

    errno = ENOSYS;
    return -1;
}
