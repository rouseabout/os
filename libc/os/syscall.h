#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#include <errno.h>

#if defined(ARCH_i686)
#define OS_SYSCALL_NR(NAME, i386, x86_64) OS_##NAME = i386
#elif defined(ARCH_x86_64)
#define OS_SYSCALL_NR(NAME, i386, x86_64) OS_##NAME = x86_64
#else
#error unsupported architecture
#endif

enum {
    OS_SYSCALL_NR(EXIT, 1, 60),
    OS_SYSCALL_NR(FORK, 2, 57),
    OS_SYSCALL_NR(READ, 3, 0),
    OS_SYSCALL_NR(WRITE, 4, 1),
    OS_SYSCALL_NR(OPEN, 5, 2),
    OS_SYSCALL_NR(CLOSE, 6, 3),
    OS_SYSCALL_NR(WAITPID, 7, 61), /* x86_64: wait4 */
    OS_SYSCALL_NR(LINK, 9, 86),
    OS_SYSCALL_NR(UNLINK, 10, 87),
    OS_SYSCALL_NR(EXECVE, 11, 59),
    OS_SYSCALL_NR(CHDIR, 12, 80),
    OS_SYSCALL_NR(CHMOD, 15, 90),
    OS_SYSCALL_NR(LSEEK, 19, 8),
    OS_SYSCALL_NR(GETPID, 20, 39),
    OS_SYSCALL_NR(ALARM, 27, 37),
    OS_SYSCALL_NR(PAUSE, 29, 34),
    OS_SYSCALL_NR(UTIME, 30, 132),
    OS_SYSCALL_NR(KILL, 37, 62),
    OS_SYSCALL_NR(RENAME, 38, 82),
    OS_SYSCALL_NR(MKDIR, 39, 83),
    OS_SYSCALL_NR(RMDIR, 40, 84),
    OS_SYSCALL_NR(DUP, 41, 32),
    OS_SYSCALL_NR(PIPE, 42, 22),
    OS_SYSCALL_NR(BRK, 45, 12),
    OS_SYSCALL_NR(IOCTL, 54, 16),
    OS_SYSCALL_NR(FCNTL, 55, 72),
    OS_SYSCALL_NR(SETPGID, 57, 109),
    OS_SYSCALL_NR(DUP2, 63, 33),
    OS_SYSCALL_NR(GETPPID, 64, 110),
    OS_SYSCALL_NR(GETPGRP, 65, 111),
    OS_SYSCALL_NR(RT_SIGRETURN, 173, 15),
    OS_SYSCALL_NR(RT_SIGACTION, 174, 13),
    OS_SYSCALL_NR(SELECT, 142, 23),
    OS_SYSCALL_NR(SYMLINK, 83, 88),
    OS_SYSCALL_NR(READLINK, 85, 89),
    OS_SYSCALL_NR(MMAP, 90, 9),
    OS_SYSCALL_NR(FTRUNCATE, 93, 77),
    OS_SYSCALL_NR(SYSLOG, 103, 103),
    OS_SYSCALL_NR(SETITIMER, 104, 38),
    OS_SYSCALL_NR(GETITIMER, 105, 36),
    OS_SYSCALL_NR(STAT, 106, 4),
    OS_SYSCALL_NR(LSTAT, 107, 6),
    OS_SYSCALL_NR(FSTAT, 108, 5),
    OS_SYSCALL_NR(CLONE, 120, 56),
    OS_SYSCALL_NR(UNAME, 122, 63),
    OS_SYSCALL_NR(GETPGID, 132, 121),
    OS_SYSCALL_NR(FCHDIR, 133, 81),
    OS_SYSCALL_NR(GETDENTS, 141, 78),
    OS_SYSCALL_NR(NANOSLEEP, 162, 35),
    OS_SYSCALL_NR(GETCWD, 183, 79),
    OS_SYSCALL_NR(CLOCK_GETTIME, 265, 228),
    OS_SYSCALL_NR(SOCKET, 359, 41),
    OS_SYSCALL_NR(BIND, 361, 49),
    OS_SYSCALL_NR(CONNECT, 362, 42),
    OS_SYSCALL_NR(LISTEN, 363, 50),
    OS_SYSCALL_NR(ACCEPT, 364, 43), /* i386: accept4 */

    OS_PTHREAD_CREATE = 2000,
    OS_PTHREAD_JOIN,
    OS_GETMNTINFO,
    OS_FSTATAT,

    OS_NB_SYSCALLS
};

#if defined(ARCH_i686)
#define os_syscall0(result, syscall) \
    asm volatile ("int $0x80" : "=a"(result) : "a"(syscall) : "memory");

#define os_syscall1(result, syscall, a1) \
    asm volatile ("int $0x80" : "=a"(result) : "a"(syscall), "b"(a1) : "memory");

#define os_syscall2(result, syscall, a1, a2) \
    asm volatile ("int $0x80" : "=a"(result) : "a"(syscall), "b"(a1), "c"(a2) : "memory");

#define os_syscall3(result, syscall, a1, a2, a3) \
    asm volatile ("int $0x80" : "=a"(result) : "a"(syscall), "b"(a1), "c"(a2), "d"(a3) : "memory");

#define os_syscall4(result, syscall, a1, a2, a3, a4) \
    asm volatile ("int $0x80" : "=a"(result) : "a"(syscall), "b"(a1), "c"(a2), "d"(a3), "S"(a4) : "memory");

#define os_syscall5(result, syscall, a1, a2, a3, a4, a5) \
    asm volatile ("int $0x80" : "=a"(result) : "a"(syscall), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5) : "memory");
#elif defined(ARCH_x86_64)
#define os_syscall0(result, syscall) \
    asm volatile ("syscall" : "=a"(result) : "a"(syscall) : "rcx", "r11", "memory");
#define os_syscall1(result, syscall, a1) \
    asm volatile ("syscall" : "=a"(result) : "a"(syscall), "D"(a1) : "rcx", "r11", "memory");
#define os_syscall2(result, syscall, a1, a2) \
    asm volatile ("syscall" : "=a"(result) : "a"(syscall), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
#define os_syscall3(result, syscall, a1, a2, a3) \
    asm volatile ("syscall" : "=a"(result) : "a"(syscall), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
#define os_syscall4(result, syscall, a1, a2, a3, a4) \
do { \
    register long r10 __asm__("r10") = (long)a4; \
    asm volatile ("syscall" : "=a"(result) : "a"(syscall), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory"); \
} while(0)
#define os_syscall5(result, syscall, a1, a2, a3, a4, a5) \
do { \
    register long r10 __asm__("r10") = (long)a4; \
    register long r8 __asm__("r8") = (long)a5; \
    asm volatile ("syscall" : "=a"(result) : "a"(syscall), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8) : "rcx", "r11", "memory"); \
} while(0)
#endif

#define RET_ERRNO(ret_type) \
    if (ret < 0) { \
        errno = -ret; \
        ret = (ret_type)-1; \
    } \
    return ret;

#define MK_SYSCALL0(ret_type, name, syscall) \
ret_type name() \
{ \
    ret_type ret; \
    os_syscall0(ret, syscall); \
    RET_ERRNO(ret_type) \
}

#define MK_SYSCALL1(ret_type, name, syscall, type1) \
ret_type name(type1 a) \
{ \
    ret_type ret; \
    os_syscall1(ret, syscall, a); \
    RET_ERRNO(ret_type) \
}

#define MK_SYSCALL1_NOERRNO(ret_type, name, syscall, type1) \
ret_type name(type1 a) \
{ \
    ret_type ret; \
    os_syscall1(ret, syscall, a); \
    return ret; \
}

#define MK_SYSCALL2(ret_type, name, syscall, type1, type2) \
ret_type name(type1 a, type2 b) \
{ \
    ret_type ret; \
    os_syscall2(ret, syscall, a, b); \
    RET_ERRNO(ret_type) \
}

#define MK_SYSCALL3(ret_type, name, syscall, type1, type2, type3) \
ret_type name(type1 a, type2 b, type3 c) \
{ \
    ret_type ret; \
    os_syscall3(ret, syscall, a, b, c); \
    RET_ERRNO(ret_type) \
}

#define MK_SYSCALL4(ret_type, name, syscall, type1, type2, type3, type4) \
ret_type name(type1 a, type2 b, type3 c, type4 d) \
{ \
    ret_type ret; \
    os_syscall4(ret, syscall, a, b, c, d); \
    RET_ERRNO(ret_type) \
}

#define MK_SYSCALL5(ret_type, name, syscall, type1, type2, type3, type4, type5) \
ret_type name(type1 a, type2 b, type3 c, type4 d, type5 e) \
{ \
    ret_type ret; \
    os_syscall5(ret, syscall, a, b, c, d, e); \
    RET_ERRNO(ret_type) \
}

#include <sys/types.h>

struct os_mmap_request {
    void * addr;
    size_t len;
    int prot;
    int flags;
    int fildes;
    off_t off;
};

#define CLONE_VM 0x100
#define CLONE_FS 0x200
#define CLONE_FILES 0x400
#define CLONE_SIGHAND 0x800
#define CLONE_THREAD 0x10000
#define CLONE_SETTLS 0x80000

#endif /* OS_SYSCALL_H */
