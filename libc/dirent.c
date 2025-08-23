#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <os/syscall.h>

struct DIR {
    int fd;
    char buf[sizeof(struct dirent)];
    size_t size;
    size_t pos;
};

int closedir(DIR * dir)
{
    int ret = close(dir->fd);
    free(dir);
    return ret;
}

DIR * opendir(const char * path)
{
    DIR * dir = malloc(sizeof(DIR));
    if (!dir)
        return NULL;
    dir->fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir->fd < 0) {
        free(dir);
        return NULL;
    }
    dir->pos = dir->size = 0;
    return dir;
}

static MK_SYSCALL3(int, sys_getdents, OS_GETDENTS, int, struct dirent *, size_t)
struct dirent *readdir(DIR * dir)
{
    if (dir->pos >= dir->size) {
        int ret = sys_getdents(dir->fd, (struct dirent *)dir->buf, sizeof(dir->buf));
        if (ret <= 0)
            return NULL;
        dir->size = ret;
        dir->pos = 0;
    }

    struct dirent *d = (struct dirent *)(dir->buf + dir->pos);
    dir->pos += d->d_reclen;
    return d;
}

void rewinddir(DIR * dirp)
{
    lseek(dirp->fd, 0, SEEK_SET);
}
