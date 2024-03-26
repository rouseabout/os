#ifndef UNISTD_H
#define UNISTD_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _POSIX_VERSION 200809L

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0 /* assumed to be zero by libstdc++ */

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK 0
#define R_OK 1
#define W_OK 2
#define X_OK 3

#define _PC_PATH_MAX 1
#define _PC_NAME_MAX 2
#define _PC_PIPE_BUF 3
#define _PC_VDISABLE 4

#define _SC_PAGESIZE 1
#define _SC_OPEN_MAX 2
#define _SC_CLK_TCK 3
#define _SC_TTY_NAME_MAX 4
#define _SC_HOST_NAME_MAX 5

int access(const char *, int);
unsigned alarm(unsigned);
int chdir(const char *);
int chown(const char *, uid_t, gid_t);
int close(int);
char * crypt(const char *, const char *);
int execl(const char *, const char *, ...);
int execle(const char *, const char *, ...);
int execlp(const char *, const char *, ...);
int execv(const char *, char *const []);
int execve(const char *, char * const [], char * const []);
int execvp(const char *, char * const []);
void _exit(int);
int fchdir(int);
int fchown(int, uid_t, gid_t);
int dup(int);
int dup2(int, int);
pid_t fork(void);
int fsync(int);
int ftruncate(int, off_t);
char * getcwd(char *, size_t);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int, gid_t []);
int gethostname(char *, size_t);
char * getlogin(void);
int getopt(int, char * const [], const char *) __attribute__((weak));
int getpagesize(void); /* legacy */
char * getpass(const char *); /* legacy */
pid_t getpgrp(void);
pid_t getpgid(pid_t);
pid_t getpid(void);
pid_t getppid(void);
int getdtablesize(void); /* legacy */
uid_t getuid(void);
int isatty(int);
int link(const char *, const char *);
off_t lseek(int, off_t, int);
long fpathconf(int, int);
long pathconf(const char *, int);
int pause(void);
int pipe(int [2]);
ssize_t pread(int, void *, size_t, off_t);
ssize_t read(int fd, void * buf, size_t size);
ssize_t readlink(const char *, char *, size_t);
int rmdir(const char *);
int setgid(gid_t);
int setpgid(pid_t, pid_t);
pid_t setpgrp(void);
pid_t setsid(void);
int setuid(uid_t);
unsigned sleep(unsigned seconds);
void sync(void);
void swab(const void *, void *, ssize_t);
int symlink(const char *, const char *);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
int truncate(const char *, off_t);
char * ttyname(int);
int ttyname_r(int, char *, size_t);
int unlink(const char *);
int usleep(useconds_t usec);
ssize_t write(int fd, const void * buf, size_t size);

extern char * optarg;
extern int opterr, optind, optopt;

#ifdef __cplusplus
}
#endif

#endif /* UNISTD_H */
