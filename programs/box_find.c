#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void tree(const char * path, const char * name)
{
    if (!name || strstr(path, name))
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
        tree(path2, name);
    }

    closedir(dir);
}

static int find_main(int argc, char ** argv, char ** envp)
{
    char * path = ".";
    char * name = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i] + 1, "name")) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "argument missing\n");
                    return EXIT_FAILURE;
                }
                name = argv[i];
            }
        } else
            path = argv[i];
    }

    tree(path, name);
    return EXIT_SUCCESS;
}
