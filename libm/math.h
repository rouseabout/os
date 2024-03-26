#ifndef MATH_H
#define MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_NORMAL 2
#define FP_SUBNORMAL 3
#define FP_ZERO 4
#define fpclassify(x) __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)

#define isfinite(x) __builtin_isfinite(x)
#define isinf(x) __builtin_isinf(x)
#define isnan(x) __builtin_isnan(x)
#define isnormal(x) __builtin_isnormal(x)
#define signbit(x) __builtin_signbit(x)

#define HUGE_VAL __builtin_huge_val()
#define HUGE_VALF __builtin_huge_valf()

#define M_E       2.718281828459045235360
#define M_PI      3.141592653589793238463
#define M_PI_2    1.570796326794896619231
#define M_PI_4    0.785398163397448309616
#define M_2_PI    0.636619772367581343076
#define M_SQRT1_2 0.707106781186547524400

#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))

double acos(double);
float acosf(float);
double acosh(double);
float acoshf(float);
double asin(double);
float asinf(float);
double asinh(double);
float asinhf(float);
double atan(double);
float atanf(float);
double atan2(double, double);
float atan2f(float, float);
double atanh(double);
float atanhf(float);
double cbrt(double);
float cbrtf(float);
double ceil(double);
float ceilf(float);
double copysign(double, double);
float copysignf(float, float);
double cos(double);
float cosf(float);
double cosh(double);
float coshf(float);
double erf(double);
float erff(float);
double erfc(double);
float erfcf(float);
double exp(double);
float expf(float);
double exp2(double);
float exp2f(float);
double expm1(double);
float expm1f(float);
double fabs(double);
float fabsf(float);
double floor(double);
float floorf(float);
double fma(double, double, double);
float fmaf(float, float, float);
double fmax(double, double);
float fmaxf(float, float);
double fmin(double, double);
float fminf(float, float);
double fmod(double, double);
float fmodf(float, float);
double frexp(double, int *);
float frexpf(float, int *);
double hypot(double, double);
float hypotf(float, float);
double ldexp(double, int);
float ldexpf(float, int);
double lgamma(double);
float lgammaf(float);
long lrint(double);
long lrintf(float);
double log(double);
float logf(float);
double log1p(double);
float log1pf(float);
double log2(double);
float log2f(float);
double log10(double);
float log10f(float);
double modf(double, double *);
float modff(float, float *);
double nan(const char *);
float nanf(const char *);
double nearbyint(double);
float nearbyintf(float);
double nextafter(double, double);
float nextafterf(float, float);
double round(double);
float roundf(float);
double pow(double, double);
float powf(float, float);
double rint(double);
float rintf(float);
double scalbn(double, int);
float scalbnf(float, int);
double sin(double);
float sinf(float);
double sinh(double);
float sinhf(float);
double sqrt(double);
float sqrtf(float);
double tan(double);
float tanf(float);
double tanh(double);
float tanhf(float);
double tgamma(double);
float tgammaf(float);
double trunc(double);
float truncf(float);

#ifdef __cplusplus
}
#endif

#endif /* MATH_H */
