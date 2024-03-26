#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void tree(const char * path)
{
    printf("%s\n", path);

    struct stat st = {0};
    if (lstat(path, &st)) {
        perror("lstat");
        return;
    }

    if (!S_ISDIR(st.st_mode))
        return;

    DIR * dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }
     
    struct dirent * de;
    while ((de = readdir(dir))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        char path2[PATH_MAX];
        snprintf(path2, sizeof(path2), "%s/%s", path, de->d_name);
        tree(path2);
    }

    closedir(dir);
}

int main(int argc, char ** argv)
{
    tree(argc >= 2 ? argv[1] : ".");
    return 0;
}
