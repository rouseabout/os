#include <math.h>
#include <stdlib.h>

typedef struct {
    unsigned int mantissa:23;
    unsigned int exponent:8;
    unsigned int sign:1;
} Float;

typedef struct {
    unsigned int mantissa_low;
    unsigned int mantissa_high:20;
    unsigned int exponent:11;
    unsigned int sign:1;
} Double;

#define MK1(type, name, instruction) \
type name(type v) \
{ \
    double ret; \
    asm (instruction : "=t"(ret) : "0"(v)); \
    return ret; \
}

#define MK2R(type, name, instruction) \
type name(type y, type x) \
{ \
    double ret; \
    asm (instruction : "=t"(ret) : "0"(x), "u"(y)); \
    return ret; \
}

#define MK2(type, name, instruction, ...) \
type name(type x, type y) \
{ \
    double ret; \
    asm (instruction : "=t"(ret) : "0"(x), "u"(y) : __VA_ARGS__); \
    return ret; \
}

double acos(double x) { return atan2(sqrt(1 - x*x), x); }
float acosf(float x) { return atan2f(sqrtf(1 - x*x), x); }

double asin(double x) { return atan2(x, sqrt(1 - x*x)); }
float asinf(float x) { return atan2f(x, sqrtf(1 - x*x)); }

MK2R(double, atan2, "fpatan")
MK2R(float, atan2f, "fpatan")
MK1(double, atan, "fld1; fpatan")
MK1(float, atanf, "fld1; fpatan")

double asinh(double x) { return log(x + sqrt(x * x + 1)); }
float asinhf(float x) { return logf(x + sqrt(x * x + 1)); }

double acosh(double x) { return log(x + sqrt(x * x - 1)); }
float acoshf(float x) { return logf(x + sqrt(x * x - 1)); }

double atanh(double x) { return log((1 + x) / (1 - x)) / 2.0; }
float atanhf(float x) { return log((1 + x) / (1 - x)) / 2.0f; }

#define MK_ROUND(type, name, cw_round_bits, frndint) \
type name(type x) \
{ \
    unsigned short orig, cw; \
    asm volatile("fstcw %w0" : "=m"(orig)); \
    cw = orig; \
    cw &= ~0xC00; \
    cw |=  cw_round_bits; \
    asm volatile("fldcw %w0" : :"m"(cw)); \
    type y = frndint(x); \
    asm volatile("fldcw %w0" : :"m"(orig)); \
    return y; \
}

MK_ROUND(double, ceil, 0x800, rint) /* round towards +infinity */
MK_ROUND(float, ceilf, 0x800, rintf) /* round towards +infinity */

double copysign(double x, double y) { return y < 0 ? -fabs(x) : fabs(x); }
float copysignf(float x, float y) { return y < 0 ? -fabsf(x) : fabsf(x); }

MK1(double, exp, "fldl2e; fmulp; fld1; fld %%st(1); fprem; f2xm1; faddp; fscale; fstp %%st(1)")
MK1(float, expf, "fldl2e; fmulp; fld1; fld %%st(1); fprem; f2xm1; faddp; fscale; fstp %%st(1)")

double cbrt(double x) { return x; }//FIXME:
float cbrtf(float x) { return x; }//FIXME:

MK1(double, cos, "fcos")
MK1(float, cosf, "fcos")

double cosh(double x) { return (exp(x) + exp(-x)) / 2.0; }
float coshf(float x) { return (expf(x) + expf(-x)) / 2.0f; }

// https://en.wikipedia.org/wiki/Error_function
double erf(double x) {
    double xx = fabs(x);
    double p = 1.0 + 0.278393*xx + 0.230389*xx*xx + 0.000972*xx*xx*xx + 0.078108*xx*xx*xx*xx;
    double y = 1.0 - 1.0/(p*p*p*p);
    return x < 0 ? -y : y;
}

float erff(float x) { return erf(x); }

double erfc(double x) { return 1.0 - erf(x); }
float erfcf(float x) { return 1.0f - erff(x); }

MK1(double, exp2, "fld1; fld %%st(1); fprem; f2xm1; faddp; fscale; fstp %%st(1)")
MK1(float, exp2f, "fld1; fld %%st(1); fprem; f2xm1; faddp; fscale; fstp %%st(1)")

double expm1(double x) { return exp(x) - 1.0; }
float expm1f(float x) { return expf(x) - 1.0f; }

MK1(double, fabs, "fabs")
MK1(float, fabsf, "fabs")

MK_ROUND(double, floor, 0x400, rint) /* round towards -infinity */
MK_ROUND(float, floorf, 0x400, rintf) /* round towards -infinity */

double fma(double x, double y, double z) { return x * y + z; }
float fmaf(float x, float y, float z) { return x * y + z; }

double fmax(double x, double y) { return x > y ? x : y; }
float fmaxf(float x, float y) { return x > y ? x : y; }

double fmin(double x, double y) { return x < y ? x : y; }
float fminf(float x, float y) { return x < y ? x : y; }

MK2(double, fmod, "1: fprem; fstsw %%ax; sahf; jp 1b", "ax", "cc")
MK2(float, fmodf, "1: fprem; fstsw %%ax; sahf; jp 1b", "ax", "cc")

double frexp(double x, int * exp) {
    union { double f; Double b; } u = { x };
    *exp = u.b.exponent - 1022;
    return scalbn(x, -(*exp));
}

float frexpf(float x, int * exp) {
    union { float f; Float b; } u = { x };
    *exp = u.b.exponent - 126;
    return scalbnf(x, -(*exp));
}

double hypot(double x, double y) { return sqrt(x * x + y * y); }
float hypotf(float x, float y) { return sqrtf(x * x + y * y); }

double ldexp(double x, int exp) { return x * pow(2.0, exp); }
float ldexpf(float x, int exp) { return x * powf(2.0f, exp); }

double lgamma(double x) { return log(tgamma(x)); }
float lgammaf(float x) { return logf(tgammaf(x)); }

long lrint(double x) { return rint(x); }
long lrintf(float x) { return rintf(x); }

MK1(double, log, "fldln2; fxch; fyl2x")
MK1(float, logf, "fldln2; fxch; fyl2x")

double log1p(double x) { return log(x + 1.0); }
float log1pf(float x) { return logf(x + 1.0f); }

MK1(double, log2, "fld1; fxch %%st(1); fyl2x")
MK1(float, log2f, "fld1; fxch %%st(1); fyl2x")

MK1(double, log10, "fldlg2; fxch; fyl2x")
MK1(float, log10f, "fldlg2; fxch; fyl2x")

double modf(double x, double * iptr) { *iptr = (int)x; return x - *iptr; }
float modff(float x, float * iptr) { *iptr = (int)x; return x - *iptr; }

double nan(const char *tagp) { return strtod("NAN", NULL); } //FIXME:
float nanf(const char *tagp) { return strtof("NAN", NULL); } //FIXME:

double nearbyint(double x) { return rint(x); } //FIXME:
float nearbyintf(float x) { return rintf(x); } //FIXME:

double nextafter(double x, double y) { return x; } //FIXME:
float nextafterf(float x, float y) { return x; } //FIXME:

MK2(double, pow, "fyl2x; fld %%st(0); frndint; fsub %%st,%%st(1); fxch; fchs; f2xm1; fld1; faddp; fxch; fld1; fscale; fstp %%st(1); fmulp",)
MK2(float, powf, "fyl2x; fld %%st(0); frndint; fsub %%st,%%st(1); fxch; fchs; f2xm1; fld1; faddp; fxch; fld1; fscale; fstp %%st(1); fmulp",)

MK1(double, rint, "frndint")
MK1(float, rintf, "frndint")

MK_ROUND(double, round, 0x000, rint) /* round to nearest */
MK_ROUND(float, roundf, 0x000, rintf) /* round to nearest */

double scalbn(double x, int n) { return x * pow(2.0, n); }
float scalbnf(float x, int n) { return x * powf(2.0f, n); }

MK1(double, sin, "fsin")
MK1(float, sinf, "fsin")

double sinh(double x) { return (exp(x) - exp(-x)) / 2.0; }
float sinhf(float x) { return (expf(x) - expf(-x)) / 2.0f; }

MK1(double, sqrt, "fsqrt")
MK1(float, sqrtf, "fsqrt")

MK1(double, tan, "fptan; fstp %%st(0)")
MK1(float, tanf, "fptan; fstp %%st(0)")

double tanh(double x) { return sinh(x) / cosh(x); }
float tanhf(float x) { return sinhf(x) / coshf(x); }

double tgamma(double x) { return sqrt(2.0 * M_PI / x) * pow(x / M_E, x); }
float tgammaf(float x) { return tgamma(x); }

double trunc(double x) { return (long)x; }
float truncf(float x) { return (long)x; }
