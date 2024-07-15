#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>

static void run2(int argc1, char ** argv1, int argc2, char ** argv2)
{
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
}

static int xargs_main(int argc, char ** argv, char ** envp)
{
    int ret = 0;
    char path[PATH_MAX];
    do {
        int i;
        for (i = 0; i < sizeof(path) - 1 && (ret = read(STDIN_FILENO, &path[i], 1)) == 1 && !isspace(path[i]); i++) ;
        if (!i)
            continue;
        path[i] = 0;
        pid_t pid = fork();
        if (pid < 0)
            perror("fork");
        else if (!pid) {
            char * path2 = path;
            run2(argc - 1, argv + 1, 1, &path2);
        } else
            wait(NULL);    
    } while (ret == 1);
    return EXIT_SUCCESS;
}
