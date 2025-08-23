#ifndef DIRENT_H
#define DIRENT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DIR DIR;

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[256];
};

int closedir(DIR *);
DIR * opendir(const char *);
struct dirent *readdir(DIR *);
void rewinddir(DIR *);

#ifdef __cplusplus
}
#endif

#endif /* DIRENT_H */
