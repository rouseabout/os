#ifndef UTMP_H
#define UTMP_H

#ifdef __cplusplus
extern "C" {
#endif

struct utmp { /* not posix */
    char ut_line[32];
    char ut_name[32];
    char ut_host[64];
    time_t ut_time;
};

#ifdef __cplusplus
}
#endif

#endif /* UTMP_H */
