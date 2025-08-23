#ifndef UTSNAME_H
#define UTSNAME_H

#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
    char  sysname[65];
    char  nodename[65];
    char  release[65];
    char  version[65];
    char  machine[65];
    char  domainname[65];
};

int uname(struct utsname *);

#ifdef __cplusplus
}
#endif

#endif /* UTSNAME_H */
