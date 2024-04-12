#include <string.h>
#include <stdint.h>
#include <bsd/string.h>
#include <errno.h>
#include <signal.h>

void * memchr(const void * s, int c, size_t n)
{
    const unsigned char * m = s;
    for (size_t i = 0; i < n; i++)
        if (m[i] == c)
            return (void *)(m + i);
    return NULL;
}

int memcmp(const void * s1, const void * s2, size_t n)
{
    const unsigned char * m1 = s1, * m2 = s2;
    int ret = 0;
    for (size_t i = 0; i < n && !(ret = m1[i] - m2[i]); i++) ;
    return ret;
}

void * memcpy(void * dst_, const void * src_, size_t size)
{
    uint8_t * dst = dst_;
    const uint8_t * src = src_;
    for (unsigned int i = 0; i < size; i++)
        dst[i] = src[i];
    return dst;
}

void * memmove(void * s1, const void * s2, size_t n)
{
    char * dst = s1;
    const char * src = s2;
    if (dst < src)
        memcpy(dst, src, n);
    else
        for (unsigned int i = 0; i < n; i++)
           dst[n - i - 1] = src[n - i - 1];
    return dst;
}

void * memset(void * dst_, int value, size_t size)
{
    uint8_t * dst = dst_;
    for (unsigned int i = 0; i < size; i++)
        dst[i] = value;
    return dst;
}

char * stpncpy(char * s1, const char * s2, size_t n)
{
    size_t i;
    for (i = 0; i < n && s2[i]; i++)
        s1[i] = s2[i];
    char * tail = s1 + i;
    for ( ; i < n; i++)
        s1[i] = 0;
    return tail;
}

char * strcat(char * s1, const char * s2)
{
    strcpy(s1 + strlen(s1), s2);
    return s1;
}

char * strchr(const char *s, int c)
{
    do {
        if (*s == c)
            return (char *)s;
    } while(*s++);
    return NULL;
}

int strcmp(const char * s1, const char * s2)
{
    while (*s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strcoll(const char *s1, const char *s2)
{
    return strcmp(s1, s2); //FIXME:
}

char * strcpy(char * s1, const char * s2)
{
    char * ret = s1;
    while (*s2)
        *s1++ = *s2++;
    *s1 = 0;
    return ret;
}

size_t strcspn(const char * s1, const char * s2)
{
    int i;
    for (i = 0; s1[i] && !strchr(s2, s1[i]); i++) ;
    return i;
}

static const char * error_name[82] = {
    [0] = "No error",
    [E2BIG] = "Argument list too long",
    [EACCES] = "Permission denied",
    [EADDRINUSE] = "Address in use",
    [EADDRNOTAVAIL] = "Address not available",
    [EAFNOSUPPORT] = "Address family not supported",
    [EAGAIN] = "Resource unavailable, try again", // (may be the same value as [EWOULDBLOCK])
    [EALREADY] = "Connection already in progress",
    [EBADF] = "Bad file descriptor",
    [EBADMSG] = "Bad message",
    [EBUSY] = "Device or resource busy",
    [ECANCELED] = "Operation canceled",
    [ECHILD] = "No child processes",
    [ECONNABORTED] = "Connection aborted",
    [ECONNREFUSED] = "Connection refused",
    [ECONNRESET] = "Connection reset",
    [EDEADLK] = "Resource deadlock would occur",
    [EDESTADDRREQ] = "Destination address required",
    [EDOM] = "Mathematics argument out of domain of function",
    [EDQUOT] = "Reserved",
    [EEXIST] = "File exists",
    [EFAULT] = "Bad address",
    [EFBIG] = "File too large",
    [EHOSTUNREACH] = "Host is unreachable",
    [EIDRM] = "Identifier removed",
    [EILSEQ] = "Illegal byte sequence",
    [EINPROGRESS] = "Operation in progress",
    [EINTR] = "Interrupted function",
    [EINVAL] = "Invalid argument",
    [EIO] = "I/O error",
    [EISCONN] = "Socket is connected",
    [EISDIR] = "Is a directory",
    [ELOOP] = "Too many levels of symbolic links",
    [EMFILE] = "File descriptor value too large",
    [EMLINK] = "Too many links",
    [EMSGSIZE] = "Message too large",
    [EMULTIHOP] = "Reserved",
    [ENAMETOOLONG] = "Filename too long",
    [ENETDOWN] = "Network is down",
    [ENETRESET] = "Connection aborted by network",
    [ENETUNREACH] = "Network unreachable",
    [ENFILE] = "Too many files open in system",
    [ENOBUFS] = "No buffer space available",
    [ENODATA] = "No message is available on the STREAM head read queue",
    [ENODEV] = "No such device",
    [ENOENT] = "No such file or directory",
    [ENOEXEC] = "Executable file format error",
    [ENOLCK] = "No locks available",
    [ENOLINK] = "Reserved",
    [ENOMEM] = "Not enough space",
    [ENOMSG] = "No message of the desired type",
    [ENOPROTOOPT] = "Protocol not available",
    [ENOSPC] = "No space left on device",
    [ENOSR] = "No STREAM resources",
    [ENOSTR] = "Not a STREAM",
    [ENOSYS] = "Functionality not supported",
    [ENOTCONN] = "The socket is not connected",
    [ENOTDIR] = "Not a directory or a symbolic link to a directory",
    [ENOTEMPTY] = "Directory not empty",
    [ENOTRECOVERABLE] = "State not recoverable",
    [ENOTSOCK] = "Not a socket",
    [ENOTSUP] = "Not supported", // (may be the same value as [EOPNOTSUPP])
    [ENOTTY] = "Inappropriate I/O control operation",
    [ENXIO] = "No such device or address",
    [EOPNOTSUPP] = "Operation not supported on socket", // (may be the same value as [ENOTSUP])
    [EOVERFLOW] = "Value too large to be stored in data type",
    [EOWNERDEAD] = "Previous owner died",
    [EPERM] = "Operation not permitted",
    [EPIPE] = "Broken pipe",
    [EPROTO] = "Protocol error",
    [EPROTONOSUPPORT] = "Protocol not supported",
    [EPROTOTYPE] = "Protocol wrong type for socket",
    [ERANGE] = "Result too large",
    [EROFS] = "Read-only file system",
    [ESPIPE] = "Invalid seek",
    [ESRCH] = "No such process",
    [ESTALE] = "Reserved",
    [ETIME] = "Stream ioctl() timeout",
    [ETIMEDOUT] = "Connection timed out",
    [ETXTBSY] = "Text file busy",
    [EWOULDBLOCK] = "Operation would block", // (may be the same value as [EAGAIN])
    [EXDEV] = "Cross-device link",
};

char * strerror(int errnum)
{
    if (errnum >= 0 && errnum < sizeof(error_name)/sizeof(error_name[0]))
        return (char *)error_name[errnum];
    return "Unknown error";
}

size_t strlen(const char * s)
{
    size_t size = 0;
    while (*s++)
        size++;
    return size;
}

char * strncat(char * s1, const char * s2, size_t n)
{
    size_t s1_len = strlen(s1);
    size_t i;
    for (i = 0 ; i < n && s2[i]; i++)
        s1[s1_len + i] = s2[i];
    s1[s1_len + i] = 0;
    return s1;
}

int strncmp(const char * s1, const char * s2, size_t size)
{
    while (size && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        size--;
    }
    return size ? *(unsigned char *)s1 - *(unsigned char *)s2 : 0;
}

char *strncpy(char * s1, const char * s2, size_t n)
{
    size_t i;
    for (i = 0; i < n && s2[i]; i++)
        s1[i] = s2[i];
    for ( ; i < n; i++)
        s1[i] = 0;
    return s1;
}

char * strrchr(const char * s, int c)
{
    const char * p;
    for (p = s + strlen(s); p >= s && *p != c; p--) ;
    return p >= s ? (char *)p : NULL;
}

size_t strnlen(const char * s, size_t maxlen)
{
    size_t size = 0;
    while (size < maxlen && *s++)
        size++;
    return size;
}

char *strpbrk(const char * s1, const char * s2)
{
    for(; *s1; s1++)
        if (strchr(s2, *s1))
            return (char *)s1;
    return NULL;
}

static const char * sys_siglist[NSIG] = {
    [SIGABRT] = "Process abort signal",
    [SIGALRM] = "Alarm clock",
    [SIGBUS] = "Access to an undefined portion of a memory object",
    [SIGCHLD] = "Child process terminated, stopped, or continued",
    [SIGCONT] = "Continue executing, if stopped",
    [SIGFPE] = "Erroneous arithmetic operation",
    [SIGHUP] = "Hangup",
    [SIGILL] = "Illegal instruction",
    [SIGINT] = "Terminal interrupt signal",
    [SIGKILL] = "Kill (cannot be caught or ignored)",
    [SIGPIPE] = "Write on a pipe with no one to read it",
    [SIGQUIT] = "Terminal quit signal",
    [SIGSEGV] = "Invalid memory reference",
    [SIGSTOP] = "Stop executing (cannot be caught or ignored)",
    [SIGTERM] = "Termination signal",
    [SIGTSTP] = "Terminal stop signal",
    [SIGTTIN] = "Background process attempting read",
    [SIGTTOU] = "Background process attempting write",
    [SIGUSR1] = "User-defined signal 1",
    [SIGUSR2] = "User-defined signal 2",
 //   [SIGPOLL] = "Pollable event",
 //   [SIGPROF] = "Profiling timer expired",
 //   [SIGSYS] = "Bad system call",
 //   [SIGTRAP] = "Trace/breakpoint trap",
 //   [SIGURG] = "High bandwidth data is available at a socket",
 //   [SIGVTALRM] = "Virtual timer expired",
    [SIGXCPU] = "CPU time limit exceeded",
 //   [SIGXFSZ] = "File size limit exceeded. ",
    [SIGTRAP] = "Trap",
};

char * strsignal(int signum)
{
    if (signum >= 0 && signum < NSIG)
        return (char *)sys_siglist[signum];
    return "Unknown signal";
}

size_t strspn(const char * s1, const char * s2)
{
    int i;
    for (i = 0; s1[i] && strchr(s2, s1[i]); i++) ;
    return i;
}

char *strstr(const char * haystack, const char * needle)
{
    int size = strlen(haystack) + 1 - strlen(needle);
    for (int i = 0; i < size; i++) {
        if (!strncmp(haystack + i, needle, strlen(needle)))
            return (char *)(haystack + i);
    }
    return NULL;
}

static char * g_saveptr;
char * strtok(char * str, const char * delim)
{
    return strtok_r(str, delim, &g_saveptr);
}

static int is_in_set(char c, const char * set)
{
    for (const char * s = set; *s; s++)
        if (*s == c)
            return 1;
    return 0;
}

static char * find_and_clear_delims(char * s, const char * delims)
{
    while (*s && !is_in_set(*s, delims))
        s++;
    if (!*s)
        return NULL;

    while (*s && is_in_set(*s, delims))
        *s++ = 0;
    if (!*s)
        return NULL;

    return s;
}

char * strtok_r(char * str, const char * delim, char ** saveptr)
{
    if (str) {
        while (*str && is_in_set(*str, delim))
            *str++ = 0;
    }

    char *s = str ? str : *saveptr;

    if (!s || !*s)
        return NULL;

    *saveptr = find_and_clear_delims(s, delim);

    return s;
}

size_t strxfrm(char * s1, const char * s2, size_t n)
{
    return strlen(strncpy(s1, s2, n)); //FIXME:
}
