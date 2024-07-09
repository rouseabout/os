#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "generic_printf.h"

int sscanf(const char * s, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsscanf(s, format, args);
    va_end(args);
    return ret;
}

int sprintf(char * s, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(s, INT_MAX, format, args);
    va_end(args);
    return ret;
}

int snprintf(char * s, size_t n, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(s, n, format, args);
    va_end(args);
    return ret;
}

#define DEBUG_SCANF 0

static int scanlist_is_valid_char(int c, const char * scanlist, int carrot)
{
    if (!carrot) {
        while (*scanlist != ']') {
            if (*scanlist == c)
                return 1; /* pass */
            scanlist++;
        }
        return 0; /* fail */
    } else {
        while (*scanlist != ']') {
            if (*scanlist == c)
                return 0; /* fail */
            scanlist++;
        }
        return 1; /* pass */
    }
}

int vsscanf(const char * s, const char * fmt, va_list args)
{
    int count = 0;
    int state = 0;
    int long_counter, short_counter, suppress, width, carrot;
    const char * scanlist;
    int c;
#if DEBUG_SCANF
        printf("[fmt='%s', s='%s']\n", fmt, s);
#endif
    for (; *fmt && (c = *s); ) {

#if DEBUG_SCANF
        printf("[fmt=%c, c=%c, state=%d]\n", *fmt, c, state);
#endif

        if (state == 0) {
            if (*fmt == c) {
                /* do nothing */
                s++;
            } else if (*fmt == '%') {
                state = 1;
                long_counter = short_counter = suppress = width = 0;
            } else {
                return EOF;
            }
        } else {
            if (*fmt == 'l') {
                long_counter++;
            } else if (*fmt == 'h') {
                short_counter++;
            } else if (*fmt == '*') {
                suppress = 1;
            } else if (isdigit(*fmt)) {
                width = strtoul(fmt, (char **)&fmt, 10);
                continue;
            } else if (*fmt == '[') {
                fmt++;
                if (!*fmt)
                    return EOF;
                if (*fmt == '^') {
                    carrot = 1;
                    fmt++;
                    if (!*fmt)
                        return EOF;
                } else
                    carrot = 0;
                scanlist = fmt;

                while (*fmt && *fmt != ']')
                    fmt++;
                if (!*fmt)
                    return EOF;

                while (width > 0) {
                    if (!scanlist_is_valid_char(*s, scanlist, carrot))
                        return EOF;
                    if (!suppress) {
                        //FIXME: write character to destination array
                    }
                    width--;
                    s++;
                }
                state = 0;
                if (!suppress)
                    count++;
            } else if (*fmt == 'd' || *fmt == 'o') {
                const char * orig_s = s;
                long long v = strtoll(s, (char **)&s, *fmt == 'f' ? 10 : 8);
                if (s == orig_s)
                    return EOF;
                if (!suppress) {
                    if (short_counter >= 2)
                        *va_arg(args, char *) = v;
                    else if (short_counter == 1)
                        *va_arg(args, short *) = v;
                    else if (long_counter >= 2)
                        *va_arg(args, long long *) = v;
                    else if (long_counter == 1)
                        *va_arg(args, long *) = v;
                    else
                        *va_arg(args, int *) = v;
                    count++;
                }
                state = 0;
            } else if (*fmt == 'u') {
                const char * orig_s = s;
                unsigned long v = strtoul(s, (char **)&s, 10);
                if (s == orig_s)
                    return EOF;
                if (!suppress) {
                    if (short_counter >= 2)
                        *va_arg(args, unsigned char *) = v;
                    else if (short_counter == 1)
                        *va_arg(args, unsigned short *) = v;
                    else if (long_counter >= 2)
                        *va_arg(args, unsigned long long *) = v;
                    else if (long_counter == 1)
                        *va_arg(args, unsigned long *) = v;
                    else
                        *va_arg(args, unsigned int *) = v;
                    count++;
                }
                state = 0;
            } else if (*fmt == 'x') {
                const char * orig_s = s;
                unsigned long long v = strtoull(s, (char **)&s, 16);
                if (s == orig_s)
                    return EOF;
                if (!suppress) {
                    if (short_counter >= 2)
                        *va_arg(args, unsigned char *) = v;
                    else if (short_counter == 1)
                        *va_arg(args, unsigned short *) = v;
                    else if (long_counter >= 2)
                        *va_arg(args, unsigned long long *) = v;
                    else if (long_counter == 1)
                        *va_arg(args, unsigned long *) = v;
                    else
                        *va_arg(args, unsigned int *) = v;
                    count++;
                }
                state = 0;
            } else if (*fmt == 'f') {
                const char * orig_s = s;
                double v = strtod(s, (char **)&s);
                if (s == orig_s)
                    return EOF;
                if (!suppress) {
                    if (long_counter == 0)
                        *va_arg(args, float *) = v;
                    else
                        *va_arg(args, double *) = v;
                    count++;
                }
                state = 0;
            } else {
#if DEBUG_SCANF
                printf("[unsupported format character]\n");
#endif
                return EOF;
            }
        }
        fmt++;
    }

#if DEBUG_SCANF
    printf("[scanf count=%d]\n", count);
#endif
    return count;
}

typedef struct {
    char * s;
    size_t size;
    size_t pos;
    size_t total;
} SNBuffer;

static void snbuf_append(void * cntx, int c)
{
    SNBuffer * buf = cntx;
    if (buf->pos + 1 < buf->size)
        buf->s[buf->pos++] = c;
    buf->total++;
}

int vsnprintf(char * s, size_t n, const char * format, va_list ap)
{
    SNBuffer buf = {.s = s, .size = n, .pos = 0, .total = 0};
    generic_vprintf(snbuf_append, &buf, format, ap);
    if (n)
        s[buf.pos] = 0;
    return buf.total;
}

int vsprintf(char * s, const char * format, va_list ap)
{
    return vsnprintf(s, INT_MAX, format, ap);
}
