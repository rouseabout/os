#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#define OSABS(x) ((x) < 0 ? -(x) : (x))

static void print(void (*_putc)(void *cntx, int c), void * cntx, const char * s)
{
    for (; *s; s++)
        _putc(cntx, *s);
}

static void printn(void (*_putc)(void *cntx, int c), void * cntx, const char * s, size_t n)
{
    while (n--)
        _putc(cntx, *s++);
}

static const char * tbl[2] = { "0123456789abcdef",
                               "0123456789ABCDEF" };

static void print_number(void (*_putc)(void *cntx, int c), void *cntx, const char * prefix, unsigned long long v, unsigned int base, int uppercase, int width, int zeropad, int sign, int force_sign)
{
    char buf[32];
    int pos = 0;

    if (prefix && v) print(_putc, cntx, prefix);

    do {
        buf[pos++] = tbl[!!uppercase][v % base];
        v /= base;
    } while (v > 0);

    if (sign)
        _putc(cntx, '-');
    else if (force_sign)
        _putc(cntx, '+');

    for (int i = 0; i < width - !!(sign||force_sign) - pos; i++)
        _putc(cntx, zeropad ? '0' : ' ');

    for (int i = pos - 1; i >= 0; i--)
        _putc(cntx, buf[i]);
}

static void print_number_unsigned(void (*_putc)(void *cntx, int c), void *cntx, const char * prefix, unsigned long long v, unsigned int base, int uppercase, int width, int zeropad, int force_sign)
{
    print_number(_putc, cntx, prefix, v, base, uppercase, width, zeropad, 0, force_sign);
}

static void print_number_signed(void (*_putc)(void *cntx, int c), void *cntx, const char * prefix, int v, unsigned int base, int uppercase, int width, int zeropad, int force_sign)
{
    print_number(_putc, cntx, prefix, OSABS(v), base, uppercase, width, zeropad, v < 0, force_sign);
}

#include <math.h>
static void print_float(void (*_putc)(void *cntx, int c), void *cntx, double v, int width, int zeropad, int force_sign, int precision)
{
    double va = fabs(v);
    print_number(_putc, cntx, NULL, (int)va, 10, 0, width - (precision ? 1 + precision : 0), zeropad, v < 0, force_sign);

    if (precision) {
        _putc(cntx, '.');

        double fraction = va - (int)va;
        for (int i = 0; i < precision; i++)
            fraction *= 10;

        print_number_unsigned(_putc, cntx, NULL, (int)fraction, 10, 0, precision, 1, 0);
    }
}

static void pad(void (*_putc)(void *cntx, int c), void * cntx, int width)
{
    for (int i = 0; i < width; i++)
        _putc(cntx, ' ');
}

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static void generic_vprintf(void (*_putc)(void *cntx, int c), void * cntx, const char * fmt, va_list args)
{
    int state = 0;
    int width;
    int zeropad;
    int long_counter;
    int point;
    int precision;
    int left_justify;
    int force_sign;
    int alternate_form;
    for (; *fmt; fmt++) {
        if (!state) {
            if (*fmt == '%') {
                state = 1;
                width = 0;
                zeropad = 0;
                long_counter = 0;
                point = 0;
                precision = 6;
                left_justify = 0;
                force_sign = 0;
                alternate_form = 0;
            } else
                _putc(cntx, *fmt);
        } else {
            if (isdigit(*fmt)) {
                if (point) {
                    precision *= 10;
                    precision += (*fmt - '0');
                } else if (*fmt == '0' && width == 0) {
                    zeropad = 1;
                } else {
                    width *= 10;
                    width += (*fmt - '0');
                }
                continue;
            } else if (*fmt == '*') {
                int v = va_arg(args, int);
                if (point)
                    precision = v;
                else
                    width = v;
                continue;
            } else if (*fmt == '.') {
                point = 1;
                precision = 0;
                continue;
            } else if (*fmt == '-') {
                left_justify = 1;
                continue;
            } else if (*fmt == '+') {
                force_sign = 1;
                continue;
            } else if (*fmt == 'l' || *fmt == 'L') {
                long_counter++;
                continue;
            } else if (*fmt == '#') {
                alternate_form = 1;
                continue;
            } else if (*fmt == '%')
                _putc(cntx, '%');
            else if (*fmt == 'c')
                _putc(cntx, va_arg(args, int));
            else if (*fmt == 's') {
                const char * s = va_arg(args, const char*);
                if (!s) {
                    s = "(null)";
                    point = 0;
                }
                size_t len = point ? precision : strlen(s);
                if (left_justify) {
                    printn(_putc, cntx, s, len);
                    pad(_putc, cntx, width - len);
                } else {
                    pad(_putc, cntx, width - len);
                    printn(_putc, cntx, s, len);
                }
            } else if (*fmt == 'o') {
                const char * prefix = alternate_form ? "0" : NULL;
                if (long_counter == 0)
                    print_number_signed(_putc, cntx, prefix, va_arg(args, int), 8, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else if (long_counter == 1)
                    print_number_signed(_putc, cntx, prefix, va_arg(args, long), 8, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else
                    print_number_signed(_putc, cntx, prefix, va_arg(args, long long), 8, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
            } else if (*fmt == 'O')
                if (long_counter == 0)
                    print_number_signed(_putc, cntx, NULL, va_arg(args, int), 8, 1, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else if (long_counter == 1)
                    print_number_signed(_putc, cntx, NULL, va_arg(args, long), 8, 1, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else
                    print_number_signed(_putc, cntx, NULL, va_arg(args, long long), 8, 1, point ? MAX(width, precision) : width, zeropad || point, force_sign);
            else if (*fmt == 'd' || *fmt == 'i')
                if (long_counter == 0)
                    print_number_signed(_putc, cntx, NULL, va_arg(args, int), 10, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else if (long_counter == 1)
                    print_number_signed(_putc, cntx, NULL, va_arg(args, long), 10, 0, point ? MAX(width, precision): width, zeropad || point, force_sign);
                else
                    print_number_signed(_putc, cntx, NULL, va_arg(args, long long), 10, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
            else if (*fmt == 'u')
                if (long_counter == 0)
                    print_number_unsigned(_putc, cntx, NULL, va_arg(args, unsigned int), 10, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else if (long_counter == 1)
                    print_number_unsigned(_putc, cntx, NULL, va_arg(args, unsigned long), 10, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else
                    print_number_unsigned(_putc, cntx, NULL, va_arg(args, unsigned long long), 10, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
            else if (*fmt == 'x') {
                const char * prefix = alternate_form ? "0x" : NULL;
                if (long_counter == 0)
                    print_number_unsigned(_putc, cntx, prefix, va_arg(args, unsigned int), 16, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else if (long_counter == 1)
                    print_number_unsigned(_putc, cntx, prefix, va_arg(args, unsigned long), 16, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else
                    print_number_unsigned(_putc, cntx, prefix, va_arg(args, unsigned long long), 16, 0, point ? MAX(width, precision) : width, zeropad || point, force_sign);
            } else if (*fmt == 'X') {
                const char * prefix = alternate_form ? "0X" : NULL;
                if (long_counter == 0)
                    print_number_unsigned(_putc, cntx, prefix, va_arg(args, unsigned int), 16, 1, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else if (long_counter == 1)
                    print_number_unsigned(_putc, cntx, prefix, va_arg(args, unsigned long), 16, 1, point ? MAX(width, precision) : width, zeropad || point, force_sign);
                else
                    print_number_unsigned(_putc, cntx, prefix, va_arg(args, unsigned long long), 16, 1, point ? MAX(width, precision) : width, zeropad || point, force_sign);
            } else if (*fmt == 'p') {
                _putc(cntx, '0');
                _putc(cntx, 'x');
                print_number_unsigned(_putc, cntx, NULL, va_arg(args, unsigned long), 16, 0, width, zeropad, 0);
            } else if (*fmt == 'f' || *fmt == 'g') {
                if (long_counter == 0)
                    print_float(_putc, cntx, va_arg(args, double), width, zeropad, force_sign, precision);
                else
                    print_float(_putc, cntx, va_arg(args, long double), width, zeropad, force_sign, precision);
            } else {
                print(_putc, cntx, "{UNEXPECTED CHARACTER:");
                _putc(cntx, *fmt);
                _putc(cntx, '}');
            }
            state = 0;
        }
    }
}
