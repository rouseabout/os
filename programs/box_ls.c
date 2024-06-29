#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>

static void print_mode(mode_t m)
{
    switch (m & S_IFMT) {
    case S_IFCHR: printf("c"); break;
    case S_IFDIR: printf("d"); break;
    case S_IFLNK: printf("l"); break;
    case S_IFREG: printf("-"); break;
    default: printf("?"); break;
    }

    if (m & S_IROTH) printf("r"); else printf("-");
    if (m & S_IWOTH) printf("w"); else printf("-");
    if (m & S_IXOTH) printf("x"); else printf("-");

    if (m & S_IRGRP) printf("r"); else printf("-");
    if (m & S_IWGRP) printf("w"); else printf("-");
    if (m & S_IXGRP) printf("x"); else printf("-");

    if (m & S_IRUSR) printf("r"); else printf("-");
    if (m & S_IWUSR) printf("w"); else printf("-");
    if (m & S_IXUSR) printf("x"); else printf("-");
}

static void ls(const char * dir_name, int a, int l)
{
    DIR * dir = opendir(dir_name);
    if (!dir) {
        perror(dir_name);
        exit(EXIT_FAILURE);
    }

    struct dirent * de;
    while ((de = readdir(dir))) {
        if (!a && de->d_name[0] == '.')
            continue;
        if (l)
            printf("%6d ", de->d_ino);
        struct stat st = {0};
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_name, de->d_name);
        if (l && lstat(path, &st) == 0) {
            print_mode(st.st_mode);
            printf(" %8d", st.st_size);
            struct tm tm;
            localtime_r(&st.st_mtime, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
            printf(" %s ", buf);
        }
        printf("%s", de->d_name);
        if (S_ISDIR(st.st_mode)) printf("/");
        if (l && S_ISLNK(st.st_mode)) {
            char symlink[PATH_MAX + 1];
            int size = readlink(path, symlink, PATH_MAX);
            if (size >= 0) {
                symlink[size] = 0;
                printf(" -> %s", symlink);
            }
        }
        printf("\n");
    }

    closedir(dir);
}

static int ls_main(int argc, char ** argv, char ** envp)
{
    int a = 0, l = 0;
    int opt;
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
        case 'a':
            a = 1;
            break;
        case 'l':
            l = 1;
            break;
        default:
            fprintf(stderr, "usage: %s [-a] [-l] [path]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    ls(optind < argc ? argv[optind] : ".", a, l);
    return EXIT_SUCCESS;
}
