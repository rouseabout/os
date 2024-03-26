#ifndef FNMATCH_H
#define FNMATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#define FNM_NOMATCH 1
#define FNM_PATHNAME 2
#define FNM_PERIOD 3

int fnmatch(const char *, const char *, int);

#ifdef __cplusplus
}
#endif

#endif /* FNMATCH_H */
