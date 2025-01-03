#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv, char ** envp)
{
    do {
        pid_t pid = fork();
        if (!pid) {
            tcsetpgrp(STDIN_FILENO, setpgrp());
            char * newargv[] = { "/bin/sh", NULL };
            printf("execv returned %d\n", execv(newargv[0], newargv));
            return -1;
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    } while(1);
}
