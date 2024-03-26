#include <stdlib.h>
#include <ctype.h>

double atof(const char *str)
{
    return strtod(str, NULL);
}

int atoi(const char * str)
{
    return strtol(str, NULL, 10);
}

long atol(const char * nptr)
{
    return strtol(nptr, NULL, 10);
}

int abs(int x)
{
    return x < 0 ? -x : x;
}

void * bsearch(const void * key, const void * base, size_t nel, size_t width, int (*compar)(const void *, const void *))
{
    while (nel > 0) {
        size_t pos = nel / 2;
        const char * this = (const char *)base + pos * width;
        int res = compar(key, this);
        if (!res) {
            return (void *)this;
        } else if (res > 0) { /* key is greater than this */
            nel -= pos + 1;
            base = this + width;
        } else { /* key is less than this */
            nel = pos;
        }
    }
    return NULL;
}

long labs(long i)
{
    return i < 0 ? -i : i;
}

long long llabs(long long i)
{
    return i < 0 ? -i : i;
}

int mblen(const char *s, size_t n)
{
    return 1; //FIXME:
}

int mbtowc(wchar_t * pwc, const char * s, size_t n)
{
    if (s)
        for (size_t i = 0; i < n; i++)
            pwc[i] = s[i];
    return n;
}

#define MK2(type, name, instruction, ...) \
type name(type x, type y) \
{ \
    double ret; \
    asm (instruction : "=t"(ret) : "0"(x), "u"(y) : __VA_ARGS__); \
    return ret; \
}
static MK2(double, pow, "fyl2x; fld %%st(0); frndint; fsub %%st,%%st(1); fxch; fchs; f2xm1; fld1; faddp; fxch; fld1; fscale; fstp %%st(1); fmulp",)

#include <strings.h>
double strtod(const char * str, char ** endptr)
{
    if (!strcasecmp(str, "nan"))
        return __builtin_nan("");
    else if (!strcasecmp(str, "inf") || !strcasecmp(str, "infinity"))
        return __builtin_inff();
    double v = 0;
    double frac = 0;
    int negative = 0;
    int e = 0;
    int exp = 0;
    int exp_negative = 0;
    while (*str) {
        char c = *str;
        if (e) {
            if (isdigit(c)) {
                exp *= 10;
                exp += c - '0';
            } else if (c == '-') {
                exp_negative = 1;
            } else
                break;
        } else if (isdigit(c)) {
            if (frac < 10) {
                v *= 10.0;
                v += c - '0';
            } else {
                v += (c - '0') / frac;
                frac *= 10;
            }
        } else if (c == '.') {
            frac = 10;
        } else if (c == '-') {
            negative = 1;
        } else if (c == 'e') {
            e = 1;
        } else
            break;
        str++;
    }

    if (endptr)
        *endptr = (char *)str;
    return (negative ? -v : v) * pow(10, exp_negative ? -exp : exp);
}

float strtof(const char * nptr, char ** endptr)
{
    return strtod(nptr, endptr); //FIXME
}

long double strtold(const char * nptr, char ** endptr)
{
    return strtod(nptr, endptr); //FIXME
}

#include "generic_strto.h"

MK_STRTOL(long, strtol)
MK_STRTOL(long long, strtoll)
MK_STRTOL(unsigned long, strtoul)
MK_STRTOL(unsigned long long, strtoull)

/* recursive quick sort */

static void swap2(char * a, char * b, size_t width)
{
    for (size_t i = 0; i < width; i++) {
        char tmp = a[i];
        a[i] = b[i];
        b[i] = tmp;
    }
}

static size_t partition(void * base, size_t l, size_t r, size_t width, int (*compar)(const void *, const void *))
{
    size_t p = l; /* partition value is left most value (ordinary performance) */
    size_t ll = l;
    size_t rr = r;
#define ADDR(x) ((char *)base + (x) * width)
    while (ll < rr) {
       while (ll <= rr && compar(ADDR(ll), ADDR(p)) <= 0) ll++;
       while (compar(ADDR(rr), ADDR(p)) > 0) rr--;
       if (ll < rr)
           swap2(ADDR(ll), ADDR(rr), width);
    }
    swap2(ADDR(p), ADDR(rr), width);
#undef ADDR
    return rr;
}

static void qsort1(void * base, int l, int r, size_t width, int (*compar)(const void *, const void *))
{
    if (l < r) {
        size_t p = partition(base, l, r, width, compar);
        qsort1(base, l, p - 1, width, compar);
        qsort1(base, p + 1, r, width, compar);
    }
}

void qsort(void * base, size_t nelems, size_t width, int (*compar)(const void *, const void *))
{
    if (nelems <= 1)
        return;
    qsort1(base, 0, nelems - 1, width, compar);
}

int wctomb(char * s, wchar_t wchar)
{
    *s = wchar;
    return 1;
}
