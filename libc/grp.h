#ifndef GRP_H
#define GRP_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct group {
    char * gr_name;
    gid_t gr_gid;
    char ** gr_mem;
};

void endgrent(void);
struct group * getgrent(void);
struct group * getgrgid(gid_t);
struct group * getgrnam(const char *);
void setgrent(void);

#ifdef __cplusplus
}
#endif

#endif /* GRP_H_ */
