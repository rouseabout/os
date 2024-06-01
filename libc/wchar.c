#include <wchar.h>
#include <syslog.h>
#include <errno.h>

wint_t fgetwc(FILE * stream)
{
    return fgetc(stream);
}

size_t mbrtowc(wchar_t * pwc, const char * s, size_t n, mbstate_t * ps)
{
    if (pwc)
        pwc[0] = s[0];
    return s[0] ? 1 : 0;
}

int mbsinit(const mbstate_t * ps)
{
    return 0;
}

int swprintf(wchar_t * ws, size_t n, const wchar_t * format, ...)
{
    syslog(LOG_DEBUG, "libc: swprintf");
    return 0;
}

size_t wcrtomb(char * s, wchar_t wc, mbstate_t * ps)
{
    if (s)
        s[0] = wc;
    return wc ? 1 : 0;
}

wchar_t * wcscat(wchar_t * ws1, const wchar_t * ws2)
{
    wcscpy(ws1 + wcslen(ws1), ws2);
    return ws1;
}

wchar_t * wcschr(const wchar_t * ws, wchar_t wc)
{
    do {
        if (*ws == wc)
            return (wchar_t *)ws;
    } while(*ws++);
    return NULL;
}

int wcscmp(const wchar_t * ws1, const wchar_t * ws2)
{
    while (*ws2 && *ws1 == *ws2) {
        ws1++;
        ws2++;
    }
    return *ws1 - *ws2;
}

wchar_t * wcscpy(wchar_t * ws1, const wchar_t * ws2)
{
    wchar_t * ret = ws1;
    while (*ws2)
        *ws1++ = *ws2++;
    *ws1 = 0;
    return ret;
}

size_t wcslen(const wchar_t * ws)
{
    size_t size = 0;
    while (*ws++)
        size++;
    return size;
}

int wcsncmp(const wchar_t * ws1, const wchar_t * ws2, size_t n)
{
     while (n && *ws1 && *ws1 == *ws2) {
          ws1++;
          ws2++;
          n--;
      }
      return n ? *ws1 - *ws2 : 0;
}

wchar_t * wcsncpy(wchar_t * ws1, const wchar_t * ws2, size_t n)
{
    size_t i;
    for (i = 0; i < n && ws2[i]; i++)
        ws1[i] = ws2[i];
    wchar_t * tail = ws1 + i;
    for ( ; i < n; i++)
        ws1[i] = 0;
    return tail;
}

wchar_t * wcsrchr(const wchar_t * ws, wchar_t wc)
{
    const wchar_t * p;
    for (p = ws + wcslen(ws); p >= ws && *p != wc; p--) ;
    return p >= ws ? (wchar_t *)p : NULL;
}

wchar_t * wcstok(wchar_t * ws1, const wchar_t * ws2, wchar_t ** ptr)
{
    syslog(LOG_DEBUG, "libc: wcstok");
    return NULL;
}

long wcstol(const wchar_t * nptr, wchar_t ** endptr, int base)
{
    syslog(LOG_DEBUG, "libc: wcstol");
    return 0;
}

int wcwidth(wchar_t wc)
{
    return 1;
}

wchar_t * wmemchr(const wchar_t * ws, wchar_t wc, size_t n)
{
    while (n && *ws) {
        if (*ws == wc)
            return (wchar_t *)ws;
    }
    return NULL;
}

wchar_t * wmemcpy(wchar_t * ws1, const wchar_t * ws2, size_t n)
{
    for (unsigned int i = 0; i < n; i++)
        ws1[i] = ws2[i];
    return ws1;
}

wint_t ungetwc(wint_t wc, FILE *stream)
{
    return ungetc(wc, stream);
}
