#ifndef PWD_H
#define PWD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct passwd {
    char * pw_name;
    char * pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char * pw_gecos; /* user information */
    char * pw_dir;
    char * pw_shell;
};

void endpwent(void);
struct passwd * getpwent(void);
struct passwd * getpwnam(const char *);
struct passwd * getpwuid(uid_t);
void setpwent(void);

#ifdef __cplusplus
}
#endif

#endif /* PWD_H */
