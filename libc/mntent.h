#ifndef MNTENT_H
#define MNTENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define	MOUNTED "/etc/mtab"

struct mntent {
    char * mnt_fsname;
    char * mnt_dir;
    char * mnt_type;
    char * mnt_opts;
    int mnt_freq;
    int mnt_passno;
};

struct mntent * getmntent(FILE *);

#ifdef __cplusplus
}
#endif

#endif /* MNTENT_H */
