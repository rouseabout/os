#ifndef WCHAR_H
#define WCHAR_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int mbstate_t;
typedef int wctype_t;
typedef int wint_t;

#define WEOF EOF

wint_t fgetwc(FILE *);
size_t mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
int mbsinit(const mbstate_t *);
int swprintf(wchar_t *, size_t, const wchar_t *, ...);
size_t wcrtomb(char *, wchar_t, mbstate_t *);
wchar_t * wcscat(wchar_t *, const wchar_t *);
wchar_t * wcschr(const wchar_t *, wchar_t);
int wcscmp(const wchar_t *, const wchar_t *);
wchar_t * wcscpy(wchar_t *, const wchar_t *);
size_t wcslen(const wchar_t *);
int wcsncmp(const wchar_t *, const wchar_t *, size_t n);
wchar_t * wcsncpy(wchar_t *, const wchar_t *, size_t);
wchar_t * wcsrchr(const wchar_t *, wchar_t);
wchar_t * wcstok(wchar_t *, const wchar_t *, wchar_t **);
long wcstol(const wchar_t *, wchar_t **, int base);
int wcwidth(wchar_t);
wchar_t * wmemchr(const wchar_t *, wchar_t, size_t);
wchar_t * wmemcpy(wchar_t *, const wchar_t *, size_t);
wint_t ungetwc(wint_t, FILE *);

#ifdef __cplusplus
}
#endif

#endif /* WCHAR_H */
