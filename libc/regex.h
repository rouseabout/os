#ifndef REGEX_H
#define REGEX_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t re_nsub;
} regex_t;

typedef ssize_t regoff_t;

#define REG_EXTENDED 1
#define REG_ICASE 2
#define REG_NEWLINE 3
#define REG_NOSUB 4
#define REG_NOMATCH 5

#define REG_NOTBOL 1

typedef struct {
    regoff_t rm_so;
    regoff_t rm_eo;
} regmatch_t;

int regcomp(regex_t *, const char *, int);
size_t regerror(int, const regex_t *, char *, size_t);
int regexec(const regex_t *, const char *, size_t, regmatch_t[1], int);
void regfree(regex_t *);

#ifdef __cplusplus
}
#endif

#endif /* REGEX_H */
