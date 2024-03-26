#ifndef LANGINFO_H
#define LANGINFO_H

#include <nl_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CODESET 0
#define D_T_FMT 1
#define D_FMT 2
#define T_FMT 3
#define DAY_1 4
#define DAY_2 5
#define DAY_3 6
#define DAY_4 7
#define DAY_5 8
#define DAY_6 9
#define DAY_7 10
#define ABDAY_1 11
#define ABDAY_2 12
#define ABDAY_3 13
#define ABDAY_4 14
#define ABDAY_5 15
#define ABDAY_6 16
#define ABDAY_7 17
#define MON_1 18
#define MON_2 19
#define MON_3 20
#define MON_4 21
#define MON_5 22
#define MON_6 23
#define MON_7 24
#define MON_8 25
#define MON_9 26
#define MON_10 27
#define MON_11 28
#define MON_12 29
#define ABMON_1 30
#define ABMON_2 31
#define ABMON_3 32
#define ABMON_4 33
#define ABMON_5 34
#define ABMON_6 35
#define ABMON_7 36
#define ABMON_8 37
#define ABMON_9 38
#define ABMON_10 39
#define ABMON_11 40
#define ABMON_12 41
#define RADIXCHAR 42
#define THOUSEP 43
#define AM_STR 44
#define PM_STR 45
#define CRNCYSTR 46

char * nl_langinfo(nl_item);

#ifdef __cplusplus
}
#endif

#endif /* LANGINFO_H */
