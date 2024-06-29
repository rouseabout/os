#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <os/syscall.h>

enum {
    TYPE_FD = 0,
    TYPE_MEM
};

struct FILE {
    int type;
    union {
        int fd;
        struct {
            char ** bufp;
            size_t * sizep;
            size_t pos;
        } mem;
    } u;
    int pos;
    char buffer[1024];
    int eof;
    int error;
};

static FILE stdin_file = {TYPE_FD, {STDIN_FILENO}, 0, {0}, 0, 0};
static FILE stdout_file = {TYPE_FD, {STDOUT_FILENO}, 0, {0}, 0, 0};
static FILE stderr_file = {TYPE_FD, {STDERR_FILENO}, 0, {0}, 0, 0};
FILE * stdin = &stdin_file;
FILE * stdout = &stdout_file;
FILE * stderr = &stderr_file;

void clearerr(FILE * stream)
{
    stream->error = 0;
}

char * ctermid(char * s)
{
    return "/dev/tty";
}

int fclose(FILE * stream)
{
    fflush(stream);
    if (stream->type == TYPE_FD)
        close(stream->u.fd);
    if (stream != &stdin_file && stream != &stdout_file && stream != &stderr_file)
        free(stream);
    return 0;
}

FILE * fdopen(int fildes, const char *mode)
{
    FILE * stream = malloc(sizeof(FILE));
    if (!stream)
        return NULL;

    stream->type = TYPE_FD;
    stream->u.fd = fildes;
    stream->pos = 0;
    stream->eof = 0;
    stream->error = 0;
    return stream;
}

int feof(FILE * stream)
{
    return stream->eof;
}

int ferror(FILE * stream)
{
    return stream->error;
}

static int grow(FILE * stream, size_t size)
{
    if (size < *stream->u.mem.sizep)
        return 0;

    char * new = realloc(*stream->u.mem.bufp, size);
    if (!new)
        return -1;
    memset(new + stream->u.mem.pos, 0, size - *stream->u.mem.sizep);
    *stream->u.mem.bufp = new;
    *stream->u.mem.sizep = size;
    return 0;
}

static size_t fwrite_internal(const void * ptr, size_t size, size_t nitems, FILE * stream)
{
    if (stream->type == TYPE_FD) {
        int ret = write(stream->u.fd, ptr, size * nitems);
        if (ret < 0) {
            stream->error = 1;
            return 0;
        }
        return nitems;
    } else {
        if (grow(stream, stream->u.mem.pos + nitems * size) < 0) {
            stream->error = 1;
            return 0;
        }
        memcpy(*stream->u.mem.bufp + stream->u.mem.pos, ptr, nitems * size);
        stream->u.mem.pos += nitems * size;
        return nitems;
    }
}

int fflush(FILE * stream)
{
    if (stream == NULL) {
        fflush(stdout);
        fflush(stderr);
        //FIXME: flush all output streams
        return 0;
    }
    if (stream->pos) {
        fwrite_internal(stream->buffer, 1, stream->pos, stream);
        stream->pos = 0;
    }
    return 0;
}

int fgetc(FILE * stream)
{
    unsigned char c;
    int ret = fread(&c, 1, 1, stream);
    return ret == 1 ? c : EOF;
}

int fgetpos(FILE * stream, fpos_t * pos)
{
    if (stream->type == TYPE_MEM) {
        *pos = lseek(stream->u.fd, 0, SEEK_CUR);
        return *pos < 0 ? -1 : 0;
    } else {
        *pos = stream->u.mem.pos;
        return 0;
    }
}

char * fgets(char * s, int n , FILE * stream)
{
    int i = 0;
    while (i < n - 1) {
        int c = fgetc(stream);
        if (c == EOF)
            break;
        s[i++] = c;
        if (c == '\n')
            break;
    }
    if (i == 0)
        return NULL;
    s[i] = 0;
    return s;
}

int fileno(FILE * stream)
{
    if (stream->type == TYPE_MEM) {
        errno = EBADF;
        return -1;
    }
    return stream->u.fd;
}

void flockfile(FILE *file)
{
    syslog(LOG_DEBUG, "libc: flockfile");
    //FIXME:
}

static FILE * fopen2(FILE * stream, int free_on_error, const char * pathname, const char * mode)
{
    int flags = 0;
    while (*mode) {
             if (*mode == 'r') flags |= O_RDONLY;
        else if (*mode == 'w') flags |= O_WRONLY | O_CREAT;
        else if (*mode == 'a') flags |= O_WRONLY | O_CREAT | O_APPEND;
        mode++;
    }

    stream->type = TYPE_FD;
    stream->u.fd = open(pathname, flags);
    if (stream->u.fd < 0) {
        if (free_on_error)
            free(stream);
        return NULL;
    }

    stream->pos = 0;
    stream->eof = 0;
    stream->error = 0;
    return stream;
}

FILE * fopen(const char * pathname, const char * mode)
{
    FILE * stream = malloc(sizeof(FILE));
    if (!stream)
        return NULL;
    return fopen2(stream, 1, pathname, mode);
}

#include "generic_printf.h"

int fprintf(FILE * stream, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stream, fmt, args);
    va_end(args);
    return ret;
}

static void stream_append(void * cntx, int c)
{
    FILE * stream = cntx;
    stream->buffer[stream->pos++] = c;
    if (c == '\n' || stream->pos > sizeof(stream->buffer)) {
        fwrite_internal(stream->buffer, 1, stream->pos, stream);
        stream->pos = 0;
    }
}

int fputc(int c, FILE * stream)
{
    stream_append(stream, c);
    return 0;
}

int fputs(const char * s, FILE * stream)
{
    return fwrite(s, strlen(s), 1, stream);
}

size_t fread(void * ptr, size_t size, size_t nitems, FILE * stream)
{
    if (stream->type == TYPE_FD) {
        int ret = read(stream->u.fd, ptr, size * nitems);
        if (ret == 0) {
            stream->eof = 1;
            return 0;
        }
        if (ret < 0) {
            stream->error = 1;
            return 0;
        }
        return ret / size;
    } else {
        size_t available = (*stream->u.mem.sizep - stream->u.mem.pos) / size;
        if (nitems > available)
            nitems = available;
        memcpy(ptr, *stream->u.mem.bufp + stream->u.mem.pos, nitems * size);
        return nitems;
    }
}

FILE * freopen(const char * pathname, const char * mode, FILE * stream)
{
    fclose(stream);
    if (!pathname)
        return NULL; //FIXME:
    if (stream == &stdin_file || stream == &stdout_file || stream == &stderr_file) {
        int origfd = stream->u.fd;
        if (!fopen2(stream, 0, pathname, mode))
            return NULL;
        if (dup2(stream->u.fd, origfd) == -1)
            return NULL;
        close(stream->u.fd);
        stream->u.fd = origfd;
        return stream;
    }
    return fopen(pathname, mode);
}

int fscanf(FILE * stream, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vfscanf(stream, format, args);
    va_end(args);
    return ret;
}

int fseek(FILE *stream, long offset, int whence)
{
    fflush(stream);
    if (stream->type == TYPE_FD) {
        return lseek(stream->u.fd, offset, whence) < 0 ? -1 : 0;
    } else {
        long newpos;
        if (whence == SEEK_SET)
            newpos = offset;
        else if (whence == SEEK_CUR)
            newpos = stream->u.mem.pos + offset;
        else if (whence == SEEK_END)
            newpos = *stream->u.mem.sizep + offset;
        else {
            errno = EINVAL;
            return -1;
        }
        if (newpos < 0)
            newpos = 0;
        grow(stream, newpos);
        stream->u.mem.pos = newpos;
        return 0;
    }
}

int fseeko(FILE *stream, off_t offset, int whence)
{
    return fseek(stream, offset, whence);
}

int fsetpos(FILE * stream, const fpos_t * pos)
{
    return fseek(stream, *pos, SEEK_SET);
}

long ftell(FILE * stream)
{
    fflush(stream);
    return stream->type == TYPE_FD ? lseek(stream->u.fd, 0, SEEK_CUR) : stream->u.mem.pos;
}

off_t ftello(FILE * stream)
{
    return ftell(stream);
}

void funlockfile(FILE *file)
{
    syslog(LOG_DEBUG, "libc: funlockfile");
    //FIXME:
}

size_t fwrite(const void * ptr, size_t size, size_t nitems, FILE * stream)
{
    fflush(stream);
    return fwrite_internal(ptr, size, nitems, stream);
}

int getc(FILE * stream)
{
    return fgetc(stream);
}

int getchar()
{
    return fgetc(stdin);
}

FILE * open_memstream(char ** bufp, size_t * sizep)
{
    FILE * stream = malloc(sizeof(FILE));
    if (!stream)
        return NULL;

    stream->type = TYPE_MEM;
    *bufp = NULL;
    *sizep = 0;
    stream->u.mem.bufp = bufp;
    stream->u.mem.sizep = sizep;
    stream->u.mem.pos = 0;
    stream->pos = 0;
    stream->eof = 0;
    stream->error = 0;
    return stream;
}

int pclose(FILE * stream)
{
    return 0; //FIXME
}

void perror(const char *s)
{
    if (s)
        fprintf(stderr, "%s: ", s);
    fprintf(stderr, "%s\n", strerror(errno));
}

FILE * popen(const char * command, const char * mode)
{
    syslog(LOG_DEBUG, "libc: popen");
    errno = ENOSYS;
    return NULL; //FIXME:
}

int printf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stdout, fmt, args);
    va_end(args);
    return ret;
}

int putc(int c, FILE * stream)
{
    return fputc(c, stream);
}

int putchar(int c)
{
    return fputc(c, stdout);
}

int puts(const char *s)
{
    printf("%s\n", s);
    return 0;
}

int remove(const char * path)
{
    if (rmdir(path) < 0)
        if (unlink(path) < 0)
            return -1;
    return 0;
}

MK_SYSCALL2(int, rename, OS_RENAME, const char *, const char *)

void rewind(FILE * stream)
{
    fseek(stream, 0, SEEK_SET);
}

int scanf(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vfscanf(stdin, format, args);
    va_end(args);
    return ret;
}

void setbuf(FILE * stream, char * buf)
{
    //FIXME:
}

int setvbuf(FILE * stream, char * buf, int type, size_t size)
{
    return -1; //FIXME:
}

char * tempnam(const char * dir, const char * pfx)
{
    return strdup("/tmp/file.XXX"); //FIXME:
}

FILE * tmpfile()
{
    return fopen("/tmp/file.XXX", "wb+"); //FIXME:
}

char * tmpnam(char * s)
{
    if (s) {
        return strcpy(s, "/tmp/file.XXX"); //FIXME:
    } else {
        static char name[] = "/tmp/file.XXX"; //FIXME:
        return name;
    }
}

int ungetc(int c, FILE * stream)
{
    if (stream->type == TYPE_FD) {
        return lseek(stream->u.fd, -1, SEEK_CUR) < 0 ? EOF : c;
    } else {
        if (!stream->u.mem.pos)
            return EOF;
        stream->u.mem.pos--;
        return c;
    }
    //FIXME: write c also?
}

int vfprintf(FILE * stream, const char * fmt, va_list args)
{
    generic_vprintf(stream_append, stream, fmt, args);
    return 0; //FIXME: return bytes printed
}

int vprintf(const char * format, va_list ap)
{
    return vfprintf(stdout, format, ap);
}

int vfscanf(FILE * stream, const char * format, va_list arg)
{
    char buf[BUFSIZ];
    if (!fgets(buf, BUFSIZ, stream))
        return EOF;
    return vsscanf(buf, format, arg);
}
