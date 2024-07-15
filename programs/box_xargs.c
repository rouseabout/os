#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>

#define NMAX 5

static void run2(int argc1, char ** argv1, int argc2, char argv2[NMAX][PATH_MAX])
{
    pid_t pid = fork();
    if (pid < 0)
        perror("fork");
    else if (!pid) {
        char ** argv = malloc((argc1 + argc2 + 1) * sizeof(char *));
        if (!argv) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < argc1; i++)
            argv[i] = argv1[i];
        for (int i = 0; i < argc2; i++)
            argv[argc1 + i] = argv2[i];
        argv[argc1 + argc2] = NULL;
        execvp(argv[0], argv);
    } else
        wait(NULL);
}

static int xargs_main(int argc, char ** argv, char ** envp)
{
    int ret = 0;
    char path[NMAX][PATH_MAX];
    int n = 0;
    do {
        int i;
        for (i = 0; i < sizeof(path[n]) - 1 && (ret = read(STDIN_FILENO, &path[n][i], 1)) == 1 && !isspace(path[n][i]); i++) ;
        if (!i)
            continue;
        path[n][i] = 0;
        n++;
        if (n >= NMAX) {
            run2(argc - 1, argv + 1, n, path);
            n = 0;
        }
    } while (ret == 1);
    if (n)
        run2(argc - 1, argv + 1, n, path);
    return EXIT_SUCCESS;
}
