#ifndef INTTYPES_H
#define INTTYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRId8 "d"
#define PRId16 "d"
#define PRId32 "ld"
#define PRId64 "lld"
#define PRIdPTR PRId32
#define PRIdMAX PRId64

#define PRIi8 "i"
#define PRIi16 "i"
#define PRIi32 "li"
#define PRIi64 "lli"
#define PRIiPTR PRIi32
#define PRIiMAX PRIi64

#define PRIo8 "o"
#define PRIo16 "o"
#define PRIo32 "lo"
#define PRIo64 "llo"
#define PRIoPTR PRIo32
#define PRIoMAX PRIo64

#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 "lu"
#define PRIu64 "llu"
#define PRIuPTR PRIu32
#define PRIuMAX PRIu64

#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 "lx"
#define PRIx64 "llx"
#define PRIxPTR PRIx32
#define PRIxMAX PRIx64

#define PRIX8 "X"
#define PRIX16 "X"
#define PRIX32 "lX"
#define PRIX64 "llX"
#define PRIXPTR PRIX32
#define PRIXMAX PRIX64

#define SCNd8 "hhd"
#define SCNd16 "hd"
#define SCNd32 "d"
#define SCNd64 "lld"
#define SCNdMAX SCNd64

#define SCNi8 "hhi"
#define SCNi16 "hi"
#define SCNi32 "i"
#define SCNi64 "lli"
#define SCNiMAX SCNi64

#define SCNu8 "hhu"
#define SCNu16 "hu"
#define SCNu32 "u"
#define SCNu64 "llu"
#define SCNuMAX SCNu64

#define SCNx8 "hhx"
#define SCNx16 "hx"
#define SCNx32 "x"
#define SCNx64 "llx"
#define SCNxMAX SCNx64

intmax_t strtoimax(const char *, char **, int);
uintmax_t strtoumax(const char *, char **, int);

#ifdef __cplusplus
}
#endif

#endif /* INTTYPES_H */
