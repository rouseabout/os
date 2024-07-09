#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#include <errno.h>

enum {
    OS_GETPID = 0,
    OS_WRITE,
    OS_READ,
    OS_FORK,
    OS_EXIT,
    OS_OPEN,
    OS_CLOSE,
    OS_LSEEK,
    OS_GETDENTS,
    OS_DUP,
    OS_DUP2,
    OS_UNLINK,
    OS_RMDIR,
    OS_MKDIR,
    OS_BRK,
    OS_EXECVE,
    OS_WAITPID,
    OS_GETCWD,
    OS_CHDIR,
    OS_STAT,
    OS_LSTAT,
    OS_FSTAT,
    OS_CLOCK_GETTIME,
    OS_NANOSLEEP,
    OS_SETPGID,
    OS_GETPGRP,
    OS_SETPGRP,
    OS_TCGETPGRP,
    OS_TCSETPGRP,
    OS_KILL,
    OS_UNAME,
    OS_IOCTL,
    OS_MMAP,
    OS_FCNTL,
    OS_TCGETATTR,
    OS_TCSETATTR,
    OS_PTHREAD_CREATE,
    OS_PTHREAD_JOIN,
    OS_SIGACTION,
    OS_GETITIMER,
    OS_SETITIMER,
    OS_SELECT,
    OS_PIPE,
    OS_GETPPID,
    OS_ISATTY,
    OS_SYSLOG,
    OS_UTIME,
    OS_RENAME,
    OS_PAUSE,
    OS_ALARM,
    OS_GETMNTINFO,
    OS_READLINK,
    OS_SYMLINK,
    OS_LINK,
    OS_FTRUNCATE,
    OS_FSTATAT,
    OS_FCHDIR,
    OS_GETPGID,
    OS_SOCKET,
    OS_BIND,
    OS_ACCEPT,
    OS_CONNECT,
    OS_CHMOD,
    OS_NB_SYSCALLS
};

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

#endif /* OS_SYSCALL_H */
