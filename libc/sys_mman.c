#include <sys/mman.h>
#include <os/syscall.h>
#include <stdlib.h>
#include <syslog.h>

#include <unistd.h>
void * mmap(void * addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    if (fildes < 0) {
        void * ptr;
        if (posix_memalign(&ptr, 4096, len) < 0)
            return MAP_FAILED;
        return ptr;
    }

    void * ret;
#if defined(ARCH_i686)
    struct os_mmap_request req = {addr, len, prot, flags, fildes, off};
    os_syscall1(ret, OS_MMAP, &req);
#else
    os_syscall6(ret, OS_MMAP, addr, len, prot, flags, fildes, off);
#endif

    /* user space mmap */
    if (ret == MAP_FAILED) {
        off_t pos = lseek(fildes, 0, SEEK_CUR);
        lseek(fildes, off, SEEK_SET);
        ret = malloc(len);
        if (!ret)
            return MAP_FAILED;
        read(fildes, ret, len);
        lseek(fildes, pos, SEEK_SET);
    }

    return ret;
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
static uintptr_t brk(uintptr_t addr)
{
    uintptr_t ret;
    os_syscall1(ret, OS_BRK, addr);
    return ret;
}

extern char end;
int munmap(void * addr, size_t len)
{
    /* user space mmap */
    uintptr_t cur = brk(0);
    if (addr >= (void *)&end && (uintptr_t)addr <= cur) {
        free(addr);
        return 0;
    }

    errno = ENOSYS;
    return -1;
}
