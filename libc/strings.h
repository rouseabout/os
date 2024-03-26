#ifndef _STRINGS_H
#define _STRINGS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* _STRINGS_H */
