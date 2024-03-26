#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <os/syscall.h>

struct DIR {
    int fd;
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
    return dir;
}

static struct dirent g_dirent;
static MK_SYSCALL3(int, sys_getdents, OS_GETDENTS, int, struct dirent *, size_t)
struct dirent *readdir(DIR * dir)
{
    int ret = sys_getdents(dir->fd, &g_dirent, 1);
    if (ret <= 0)
        return NULL;
    return &g_dirent;
}

void rewinddir(DIR * dirp)
{
    lseek(dirp->fd, 0, SEEK_SET);
}
