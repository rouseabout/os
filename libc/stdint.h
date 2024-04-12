#ifdef __GNUC__
#include "stdint-gcc.h" // gcc supplied stdint
#else
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
#if __x86_64__
typedef int64_t intptr_t;
typedef uint64_t uintptr_t;
#else
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
#endif
#endif
