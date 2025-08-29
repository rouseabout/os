#include <unistd.h>
#include <os/syscall.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <bsd/string.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <termios.h>

char * optarg __attribute__((weak));
int opterr __attribute__((weak)), optind __attribute__((weak)) = 1, optopt __attribute__((weak));

int access(const char *path, int amode)
{
    syslog(LOG_DEBUG, "libc: access: '%s', mode=%d", path, amode);
    int fd = open(path, O_RDONLY); //FIXME: F_WRITE, F_READ
    if (fd >= 0) {
        close(fd);
        syslog(LOG_DEBUG, "libc: access ret=0");
        return 0;
    }
    errno = ENOENT;
    return -1;
}

MK_SYSCALL1_NOERRNO(unsigned, alarm, OS_ALARM, unsigned)

MK_SYSCALL1(int, chdir, OS_CHDIR, const char *)

int chown(const char * path, uid_t owner, gid_t group)
{
    syslog(LOG_DEBUG, "libc: chown");
    errno = ENOSYS;
    return -1; //FIXME:
}

MK_SYSCALL1(int, close, OS_CLOSE, int)

char * crypt(const char * key, const char * salt)
{
    //FIXME: actually encrypt the key
    return (char *)key;
}

int execl(const char * path, const char * arg0, ...)
{
    syslog(LOG_DEBUG, "libc: execl");
    errno = ENOSYS;
    return -1;
}

static int execve_internal(const char * path, char * const * argv, char * const * envp)
{
    char path2[PATH_MAX];
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1;
    char shebang[2];
    if (read(fd, shebang, sizeof(shebang)) == sizeof(shebang) && shebang[0] == '#' && shebang[1]) {
        int i;
        for (i = 0; i < PATH_MAX - 1 && read(fd, &path2[i], sizeof(char)) == sizeof(char) && path2[i] != '\n'; i++) ;
        path2[i] = 0;
        dup2(fd, STDIN_FILENO);
        path = path2; //FIXME: split path2 and prepend argv
    }
    close(fd);
    return execve(path, argv, envp);
}

int execle(const char *path, const char *arg0, ... /*, (char *)0, char *const envp[]*/)
{
    if (!arg0) {
        syslog(LOG_DEBUG, "libc: execle special case");
        errno = ENOSYS;
        return -1;
    }

    /* count arguments */

    size_t argc = 1;
    va_list ap;
    va_start(ap, arg0);
    while (va_arg(ap, char *))
        argc++;
    va_end(ap);

    char ** argv = malloc((argc + 1) * sizeof(char *));
    if (!argv) {
        errno = ENOMEM;
        return -1;
    }

    /* copy arguments */

    va_start(ap, arg0);
    argv[0] = (char *)arg0;
    for (size_t i = 1; i <= argc; i++)
        argv[i] = va_arg(ap, char *);
    char ** envp = va_arg(ap, char **);
    va_end(ap);

    int ret = execve_internal(path, argv, envp);
    free(argv);
    return ret;
}

int execlp(const char *file, const char *arg0, ... /*, (char *)0 */)
{
    syslog(LOG_DEBUG, "libc: execlp");
    errno = ENOSYS;
    return -1;
}

extern char **environ;

int execv(const char * path, char * const argv[])
{
    return execve_internal(path, argv, environ);
}

MK_SYSCALL3(int, execve, OS_EXECVE, const char *, char * const *, char * const *)

static int find_in_path(char * abspath, int size, const char * filename)
{
    struct stat st;
    if (!stat(filename, &st)) {
        strlcpy(abspath, filename, size);
        return 0;
    }

    const char * path_env = getenv("PATH");
    if (!path_env)
        path_env = "/bin:/usr/bin";

    char * path_env2 = strdup(path_env);

    char * tok = strtok(path_env2, ":");
    while (tok) {
        snprintf(abspath, size, "%s/%s", tok, filename);
        if (!stat(abspath, &st)) {
            free(path_env2);
            return 0;
        }
        tok = strtok(NULL, ":");
    }

    free(path_env2);
    return -ENOENT;
}

int execvp(const char * pathname, char * const argv[])
{
    int ret;
    char abspath[PATH_MAX];
    if ((ret = find_in_path(abspath, sizeof(abspath), pathname) < 0))
        return ret;

    return execve_internal(abspath, argv, environ);
}

void _exit(int status)
{
#if defined(ARCH_i686)
    asm volatile ("int $0x80" : : "a"(OS_EXIT), "b"(status));
#elif defined(ARCH_x86_64)
    asm volatile ("syscall" : : "a"(OS_EXIT), "D"(status));
#endif
}

MK_SYSCALL1(int, dup, OS_DUP, int)
MK_SYSCALL2(int, dup2, OS_DUP2, int, int)
MK_SYSCALL1(int, fchdir, OS_FCHDIR, int)

int fchown(int fildes, uid_t owner, gid_t group)
{
    syslog(LOG_DEBUG, "libc: fchown");
    errno = ENOSYS;
    return -1; //FIXME:
}

MK_SYSCALL0(pid_t, fork, OS_FORK)

long fpathconf(int fildes, int name)
{
    switch (name) {
    case _PC_VDISABLE: return 0;
    }

    syslog(LOG_DEBUG, "libc: fpathconf fildes=%d, name=%d", fildes, name);
    errno = ENOSYS;
    return -1; //FIXME:
}

int fsync(int fildes)
{
    syslog(LOG_DEBUG, "libc: fsync fildes=%d", fildes);
    return 0; //FIXME:
}

MK_SYSCALL2(int, ftruncate, OS_FTRUNCATE, int, off_t)

static MK_SYSCALL2(int, sys_getcwd, OS_GETCWD, char *, size_t)
char * getcwd(char * buf, size_t size)
{
    if (!buf)
        buf = malloc(size ? size : PATH_MAX);
    int ret = sys_getcwd(buf, size);
    if (ret < 0)
        return NULL;
    return buf;
}

gid_t getegid()
{
    return 1; //FIXME:
}

uid_t geteuid()
{
    return 1; //FIXME:
}

gid_t getgid()
{
    return 1; //FIXME:
}

int getgroups(int gidsetsize, gid_t grouplist[])
{
    errno = ENOSYS;
    return -1;
}

int gethostname(char * name, size_t namelen)
{
    struct utsname uts;
    if (uname(&uts) == -1)
        return -1;

    strlcpy(name, uts.nodename, namelen);
    return 0;
}

char * getlogin()
{
    return "user"; //FIXME:
}

static const char * nextchar = NULL;
int getopt(int argc, char * const argv[], const char *optstring)
{
    const char * s, *p;

    if (nextchar && *nextchar) {
        s = nextchar++;
        nextchar = NULL;
        goto process;
    }

    if (nextchar) {
        nextchar = NULL;
        optind++;
    }

    if (optind >= argc)
        return -1;

    s = argv[optind];
    if (s[0] != '-')
        return -1;
    s++;

process:
    p = strchr(optstring, s[0]);
    if (!p) {
        fprintf(stderr, "%s: invalid option '%c'\n", argv[0], s[0]);
        opterr = 1;
        optopt = *s;
        if (s[1])
            nextchar = s + 1;
        else
            optind++;
        return '?';
    }

    if (p[1] == ':') {
        optind += 1;
        optarg = argv[optind];
        optind += 1;
        return s[0];
    }

    if (s[1])
        nextchar = s + 1;
    else
        optind++;

    return s[0];
}

int getpagesize(void)
{
    return 4096;
}

char * getpass(const char * prompt)
{
    syslog(LOG_DEBUG, "libc: getpass");
    //FIMXE: open /dev/tty, display prompt, disable echo, read one line, close tty
    return "password";
}

MK_SYSCALL1(pid_t, getpgid, OS_GETPGID, pid_t)

MK_SYSCALL0(pid_t, getpid, OS_GETPID)

MK_SYSCALL0(pid_t, getppid, OS_GETPPID)

MK_SYSCALL0(pid_t, getpgrp, OS_GETPGRP)

int getdtablesize(void)
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
        return -1;
    return rl.rlim_cur;
}

uid_t getuid()
{
    return 1; //FIXME:
}

int isatty(int fildes)
{
    struct termios term;
    if (!tcgetattr(fildes, &term))
        return 1;
    errno = ENOTTY;
    return 0;
}

MK_SYSCALL2(int, link, OS_LINK, const char *, const char *)

MK_SYSCALL3(off_t, lseek, OS_LSEEK, int, off_t, int)

long pathconf(const char *path, int name)
{
    syslog(LOG_DEBUG, "libc: fpathconf path='%s' name=%d", path, name);
    errno = ENOSYS;
    return -1; //FIXME:
}

MK_SYSCALL0(int, pause, OS_PAUSE)

static MK_SYSCALL1(int, sys_pipe, OS_PIPE, int *)
int pipe(int fildes[2])
{
    return sys_pipe(fildes);
}

ssize_t pread(int fildes, void * buf, size_t nbyte, off_t offset)
{
    int pos = lseek(fildes, 0, SEEK_CUR);
    if (pos < 0)
        return -1;
    if (lseek(fildes, offset, SEEK_SET) < 0)
        return -1;
    int n = read(fildes, buf, nbyte);
    lseek(fildes, pos, SEEK_SET);
    return n;
}

MK_SYSCALL3(ssize_t, read,OS_READ, int, void *, size_t)

MK_SYSCALL3(ssize_t, readlink, OS_READLINK, const char *, char *, size_t)

MK_SYSCALL1(int, rmdir, OS_RMDIR, const char *)

int setgid(gid_t gid)
{
    syslog(LOG_DEBUG, "libc: setgid");
    errno = EPERM;
    return -1; //FIXME:
}

MK_SYSCALL2(int, setpgid, OS_SETPGID, pid_t, pid_t)

int setpgrp(void)
{
    return setpgid(0, 0);
}

pid_t setsid()
{
    syslog(LOG_DEBUG, "libc: setsid");
    errno = ENOSYS;
    return -1; //FIXME:
}

int setuid(uid_t uid)
{
    syslog(LOG_DEBUG, "libc: setuid");
    errno = EPERM;
    return -1; //FIXME:
}

unsigned sleep(unsigned seconds)
{
    nanosleep(&(struct timespec){.tv_sec=seconds, .tv_nsec=0}, NULL);
    return 0;
}

void sync(void)
{
    syslog(LOG_DEBUG, "libc: sync");
    //FIXME:
}

long sysconf(int name)
{
    switch (name) {
    case _SC_CLK_TCK: return 1000000;
    case _SC_OPEN_MAX: return OPEN_MAX;
    case _SC_PAGESIZE: return 4096;
    case _SC_TTY_NAME_MAX: return 32;
    case _SC_HOST_NAME_MAX: return 255;
    };

    syslog(LOG_DEBUG, "libc: sysconf name=%d", name);
    errno = EINVAL;
    return -1;
}

#include <stdint.h>
void swab(const void * src_, void * dst_, ssize_t nbytes)
{
    const uint8_t * src = src_;
    uint8_t * dst = dst_;
    while (nbytes) {
        dst[0] = src[1];
        dst[1] = src[0];
        src += 2;
        dst += 2;
        nbytes -= 2;
    }
}

MK_SYSCALL2(int, symlink, OS_SYMLINK, const char *, const char *)

pid_t tcgetpgrp(int fildes)
{
    pid_t pgrp;
    if (ioctl(fildes, TIOCGPGRP, &pgrp) < 0)
        return -1;
    return pgrp;
}

int tcsetpgrp(int fildes, pid_t pgrp)
{
    return ioctl(fildes, TIOCSPGRP, &pgrp);
}

int truncate(const char * path, off_t length)
{
    int fd = open(path, O_APPEND|O_WRONLY);
    if (fd == -1)
        return -1;
    int ret = ftruncate(fd, length);
    close(fd);
    return ret;
}

char * ttyname(int fildes)
{
    return "/dev/tty"; //FIXME:
}

int ttyname_r(int fildes, char * name, size_t namesize)
{
    strlcpy(name, "/dev/tty", namesize); //FIXME:
    return 0;
}

MK_SYSCALL1(int, unlink, OS_UNLINK, const char *)
MK_SYSCALL3(ssize_t, write, OS_WRITE, int, const void *, size_t)

int usleep(useconds_t usec)
{
    nanosleep(&(struct timespec){.tv_sec=usec/1000000, .tv_nsec=1000*(usec % 1000000)}, NULL);
    return 0;
}
