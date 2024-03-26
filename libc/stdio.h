#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long fpos_t;
typedef struct FILE FILE;

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0 /* assumed to be zero by libstdc++ */

#define TMP_MAX 64

#define BUFSIZ 512
#define L_ctermid 32
#define L_tmpnam 32

#define EOF (-1)

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define FILENAME_MAX 256

#define P_tmpdir "/tmp"

extern FILE * stderr, * stdin, * stdout;

void clearerr(FILE *);
char * ctermid(char *);
int fclose(FILE *);
FILE * fdopen(int, const char *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fgetc(FILE *);
int fgetpos(FILE *, fpos_t *);
char * fgets(char *, int, FILE *);
int fileno(FILE *);
void flockfile(FILE *);
FILE * fopen(const char *, const char *);
int fprintf(FILE *, const char *, ...);
int fputc(int, FILE *);
int fputs(const char *, FILE *);
size_t fread(void *, size_t, size_t, FILE *);
int fscanf(FILE *, const char *, ...);
int fseek(FILE *, long, int);
int fseeko(FILE *, off_t, int);
int fsetpos(FILE *, const fpos_t *);
long ftell(FILE *);
off_t ftello(FILE *);
void funlockfile(FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int getc(FILE *);
#define getc_unlocked getc
int getchar(void);
FILE * open_memstream(char **, size_t *);
int pclose(FILE *);
void perror(const char *);
FILE * popen(const char * command, const char * mode);
int printf(const char *, ...);
int putc(int, FILE *);
int putchar(int);
int puts(const char *);
int remove(const char *);
int rename(const char *, const char *);
FILE * freopen(const char *, const char *, FILE *);
void rewind(FILE *);
int scanf(const char *, ...);
void setbuf(FILE *, char *);
int setvbuf(FILE *, char *, int, size_t);
int sscanf(const char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);
int sprintf(char *, const char *, ...);
char * tempnam(const char *, const char *);
FILE * tmpfile(void);
char * tmpnam(char *);
int ungetc(int, FILE *);
int vfprintf(FILE *, const char *, va_list);
int vfscanf(FILE *, const char *, va_list);
int vprintf(const char *, va_list);
int vsprintf(char *, const char *, va_list);
int vsnprintf(char *, size_t, const char *, va_list);
int vsscanf(const char *, const char *, va_list);

#ifdef __cplusplus
}
#endif

#endif /* STDIO_H */
