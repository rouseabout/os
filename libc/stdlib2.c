#include <stdlib.h>

/* Heap initialisation */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <bsd/string.h>
#include <os/syscall.h>
#include <sys/stat.h>
#include <syslog.h>
#include "heap.h"

static MK_SYSCALL2(int, brk, OS_BRK, void *, void **)

static int grow_cb(Halloc * cntx, unsigned int extra)
{
    void * cur;
    brk(NULL, &cur);
    if (brk((char *)cur + extra, NULL) < 0)
        return -ENOMEM;
    cntx->end = (char *)cntx->end + extra;
    return 0;
}

static void shrink_cb(Halloc * cntx, uintptr_t boundary_addr)
{
    brk((void *)boundary_addr, NULL);
}

static Halloc uheap;

char ** environ = NULL;
size_t environ_allocated = 0;

static void (*atexit_func)(void) = NULL;

extern void (*__init_array_start [])(void) __attribute__((weak));
extern void (*__init_array_end [])(void) __attribute__((weak));
extern void (*__fini_array_start [])(void) __attribute__((weak));
extern void (*__fini_array_end [])(void) __attribute__((weak));

extern char _end;
int main(int argc, char **argv, char ** envp);
int _libc_main(int argc, char **argv, char ** envp);
int _libc_main(int argc, char **argv, char ** envp)
{
    uheap.reserve_size = 0;
    uheap.grow_cb = grow_cb;
    uheap.shrink_cb = shrink_cb;
    uheap.dump_cb = NULL;
    uheap.printf = printf;
    uheap.abort = abort;

    void *cur;
    brk(NULL, &cur);
    halloc_init(&uheap, &_end, (char *)cur - &_end);

    for (size_t i = 0; i < __init_array_end - __init_array_start; i++)
        __init_array_start[i]();

    environ = envp;
    environ_allocated = 0;
    int ret = main(argc, argv, envp);
    exit(ret);

    return ret;
}

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);
int __cxa_atexit(void (*f)(void *), void *objptr, void *dso)
{
    syslog(LOG_DEBUG, "libc: __cxa_atexit");
    return 0;
}

#include <signal.h>
void abort()
{
    raise(SIGABRT);
    exit(1);
}

int atexit(void (*func)(void))
{
    atexit_func = func;
    return 0;
}

void * calloc(size_t nelem, size_t elsize)
{
    void * ptr = malloc(nelem * elsize);
    if (!ptr)
        return NULL;
    memset(ptr, 0, nelem * elsize);
    return ptr;
}

div_t div(int numer, int denom)
{
    div_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

static MK_SYSCALL1(int, sys_exit, OS_EXIT, int)
void exit(int status)
{
    if (atexit_func)
        atexit_func();

    for (size_t i = 0; i < __fini_array_end - __fini_array_start; i++)
        __fini_array_start[i]();

    sys_exit(status);
}

void free(void * addr)
{
    hfree(&uheap, addr);
}

char * getenv(const char * name)
{
    if (!environ)
        return NULL;

    size_t size = strlen(name);
    for (unsigned int i = 0; environ[i]; i++)
        if (!strncmp(environ[i], name, size) && environ[i][size] == '=')
            return environ[i] + size + 1;
    return NULL;
}

ldiv_t ldiv(long numer, long denom)
{
    ldiv_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

void * malloc(size_t size)
{
    void * ret = halloc(&uheap, size ? size : 1, 4, 0, "stdlib");
    if (!ret)
        errno = ENOMEM;
    return ret;
}

size_t mbstowcs(wchar_t * pwcs, const char * s, size_t n)
{
    syslog(LOG_DEBUG, "libc: mbstowcs");
    errno = EILSEQ; //FIXME:
    return (size_t)-1;
}

#include <fcntl.h>
#include <sys/stat.h>
char * mkdtemp(char * template)
{
    syslog(LOG_DEBUG, "libc: mkdtemp");
    mkdir(template, 0666); //FIXME:
    return template;
}

#include <time.h>
static int template_counter = 0;
static int fill_template(char * template)
{
    int n = strlen(template);
    if (n < 6 || strcmp(template + n - 6, "XXXXXX")) {
        errno = EINVAL;
        return -1;
    }
    int id = time(NULL) + 1000*getpid() + template_counter++;
    for (int i = 0; i < 6; i++) {
        template[n - 6 + i] = 'A' + (id % 26);
        id /= 26;
    }
    return 0;
}

int mkstemp(char * template)
{
    if (fill_template(template) == -1)
        return -1;
    return open(template, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
}

char * mktemp(char * template)
{
    if (fill_template(template) == -1)
        template[0] = 0;
    return template;
}

static int is_power_of_two(int x)
{
    return (x & (x - 1)) == 0;
}

int posix_memalign(void ** memptr, size_t alignment, size_t size)
{
    if (alignment < 4 || !is_power_of_two(alignment))
        return EINVAL;

    void * ptr = halloc(&uheap, size, alignment, 0, "posix_memalign");
    if (!ptr)
        return ENOMEM;

    *memptr = ptr;
    return 0;
}

int putenv(char * string)
{
    syslog(LOG_DEBUG, "libc: putenv: %s", string);
    errno = ENOSYS;
    return -1; //FIXME:
}

static unsigned int g_seed = 123456789;

int rand()
{
    return random() % RAND_MAX;
}

long random()
{
    g_seed = 1103515245 * g_seed + 12345;
    return g_seed & 0x7fffffff;
}

void * realloc(void * ptr, size_t size)
{
    return hrealloc(&uheap, ptr, size);
}

static int realpath2(const char * file_name, char * resolved_name)
{
    struct stat st;
    if (lstat(file_name, &st))
        return -1;
    if (S_ISLNK(st.st_mode)) {
        char tmp[PATH_MAX];
        int size = readlink(file_name, tmp, PATH_MAX);
        if (size < 0)
            return -1;
        tmp[size] = 0;
        return realpath2(tmp, resolved_name);
    }
    strlcpy(resolved_name, file_name, PATH_MAX);
    return 0;
}

char * realpath(const char * file_name, char * resolved_name)
{
    char tmp[PATH_MAX];
    if (!resolved_name)
        resolved_name = tmp;

    int ret = realpath2(file_name, resolved_name);
    if (ret < 0)
        return NULL;

    return resolved_name == tmp ? strdup(tmp) : resolved_name;
}

int setenv(const char *envname, const char *envval, int overwrite)
{
    syslog(LOG_DEBUG, "libc: setenv %s=%s, overwrite=%d", envname, envval, overwrite);

    size_t name_size = strlen(envname);

    size_t size = 0;
    int count = 0, found = 0;
    for (int i = 0; environ[i]; i++) {
        if (!strncmp(environ[i], envname, name_size)) {
            if (envval) {
                size += sizeof(char *) + strlen(envname) + 1 + strlen(envval) + 1;
                found = 1;
                count++;
            }
        } else {
            size += sizeof(char *) + strlen(environ[i]) + 1;
            count++;
        }
    }
    if (envval && !found) {
        size += sizeof(char *) + strlen(envname) + 1 + strlen(envval) + 1;
        count++;
    }

    size += sizeof(char *);

    char ** environ2 = malloc(size);
    if (!environ2) {
        errno = ENOMEM;
        return -1;
    }

    char * p = (char *)(environ2 + count + 1);
    int j = 0;
    for (unsigned int i = 0; environ[i]; i++) {
        if (!strncmp(environ[i], envname, name_size)) {
            if (envval) {
                environ2[j++] = p;
                memcpy(p, envname, strlen(envname));
                p += strlen(envname);
                *p++ = '=';
                memcpy(p, envval, strlen(envval) + 1);
                p += strlen(envval) + 1;
            }
        } else {
            environ2[j++] = p;
            memcpy(p, environ[i], strlen(environ[i]) + 1);
            p += strlen(environ[i]) + 1;
        }
    }
    if (envval && !found) {
        environ2[j++] = p;
        memcpy(p, envname, strlen(envname));
        p += strlen(envname);
        *p++ = '=';
        memcpy(p, envval, strlen(envval) + 1);
    }
    environ2[j] = 0;

    if (environ_allocated)
        free(environ);
    environ = environ2;
    environ_allocated = 1;
    return 0;
}

void srand(unsigned seed)
{
    srandom(seed);
}

void srandom(unsigned seed)
{
    g_seed = seed;
}

#include <unistd.h>
#include <sys/wait.h>
int system(const char * command)
{
    if (!command)
        return 0;
    pid_t pid = fork();
    if (pid == -1)
        return -1;
    if (!pid) {
        char * argv[] = {"sh", "-c", (char *)command, NULL};
        return execv(getenv("SHELL") ? getenv("SHELL") : "/bin/sh", argv);
    }
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

int unsetenv(const char *name)
{
    syslog(LOG_DEBUG, "libc: unsetenv");
    errno = ENOSYS;
    return -1;
}

size_t wcstombs(char * s, const wchar_t * pwcs, size_t n)
{
    syslog(LOG_DEBUG, "libc: wcstombs");
    return -1;
}
