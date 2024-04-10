#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX 32768

#define MB_CUR_MAX 1

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

void abort(void);
int abs(int);
int atexit(void (*)(void));
double atof(const char *);
int atoi(const char *);
long atol(const char *);
void * bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
void * calloc(size_t, size_t);
div_t div(int, int);
void exit(int);
void free(void *);
char * getenv(const char *);
long labs(long);
ldiv_t ldiv(long numer, long denom);
long long llabs(long long);
void * malloc(size_t);
int mblen(const char *, size_t);
size_t mbstowcs(wchar_t *, const char *, size_t);
int mbtowc(wchar_t *, const char *, size_t);
char *mkdtemp(char *);
int mkstemp(char *);
char * mktemp(char *); /* legacy */
int posix_memalign(void **, size_t, size_t);
int putenv(char *);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int rand(void);
long random(void);
void * realloc(void *, size_t);
char * realpath(const char *, char *);
int setenv(const char *, const char *, int);
void srand(unsigned);
void srandom(unsigned);
double strtod(const char *, char **);
float strtof(const char *, char **);
long strtol(const char *, char **, int);
long double strtold(const char *, char **);
long long strtoll(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);
int system(const char *);
int unsetenv(const char *);
int wctomb(char *, wchar_t);
size_t wcstombs(char *, const wchar_t *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* STDLIB_H */
