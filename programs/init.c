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
            char * newenvp[] = { "HOME=/", "PATH=/bin:/usr/bin:/usr/local/bin:/usr/local/sbin", "TERM=vt100", NULL };
            printf("execve returned %d\n", execve(newargv[0], newargv, newenvp));
            return -1;
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    } while(1);
}
