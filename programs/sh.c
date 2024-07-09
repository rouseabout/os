#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <bsd/string.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char ** argv)
{
    int interactive = isatty(STDIN_FILENO);
    int single_command = 0;
    int ret = 0;

    if (argc >= 3 && !strcmp(argv[1], "-c")) {
        interactive = 0;
        single_command = 1;
    } else if (argc > 1) {
        fprintf(stderr, "usage: %s [-c command]\n", argv[0]);
        return EXIT_FAILURE;
    }

    while(1) {
        int pos = 0;

        if (interactive) {
            printf("%s", getenv("PS1") ? getenv("PS1") : "$ ");
            fflush(stdout);
        }

        char cmdline[1024];

        if (!single_command) {
            do {
                int c = getchar();
                if (c == EOF) return 0;
                if (c == '\n') break;
                cmdline[pos++] = c;
            } while(pos < sizeof(cmdline) - 1);
            cmdline[pos] = 0;
        } else
            strlcpy(cmdline, argv[2], sizeof(cmdline));

        pid_t pid;
        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) fprintf(stderr, "[1] Done\n");

        char * p = cmdline;
        int pipe0 = -1;
        int first_pid = -1;
        while (*p) {
            char * newargv[16];
            int newargc = 0;
            int input = 0, output = 0;
            char * input_path = NULL;
            char * output_path = NULL;
            int background = 0;
            int do_pipe = 0;

            while(*p && newargc + 1 < sizeof(newargv)/sizeof(newargv[0])) {
                while(*p==' ' && *p) *p++ = 0;
                if (!*p) break;
                if (p[0] == '<') {
                    input = 1;
                    output = 0;
                    *p++ = 0;
                    continue;
                } else if (p[0] == '>') {
                    input = 0;
                    output = 1;
                    *p++ = 0;
                    continue;
                } else if (p[0] == '&') {
                    background = 1;
                    *p++ = 0;
                    continue;
                } else if (p[0] == '|') {
                    background = 1;
                    do_pipe = 1;
                    *p++ = 0;
                    break; /* stop */
                } else if (input == 1) {
                    input_path = p;
                } else if (output == 1) {
                    output_path = p;
                } else if (p[0] == '\'') {
                    p++;
                    newargv[newargc++] = p;
                    while(*p != '\'' && *p) p++;
                    *p++ = 0;
                } else
                    newargv[newargc++] = p;
                while(!strchr(" <>&|", *p) && *p) p++;
            }

            if (!newargc)
                continue;

            newargv[newargc] = NULL;

#if 0
            for (int i = 0;i < newargc; i++) {
                printf("newargv[%d]='%s'\n", i, newargv[i]);
            }
#endif
            if (!strcmp(newargv[0], "exit"))
                break;
            if (!strcmp(newargv[0], "cd")) {
                if (newargc > 2) {
                    fprintf(stderr, "too many arguments\n");
                    continue;
                }
                if (newargc == 2) {
                    ret = chdir(newargv[1]);
                } else {
                    char * home = getenv("HOME");
                    if (!home) {
                        fprintf(stderr, "HOME variable not set\n");
                        continue;
                    }
                    ret = chdir(home);
                }
                if (ret < 0)
                    fprintf(stderr, "chdir failed\n");
                continue;
            }
            if (!strcmp(newargv[0], "help")) {
                fprintf(stderr, "sh built-in commands: cd exit help setenv wait\n");
                continue;
            }
            if (!strcmp(newargv[0], "setenv")) {
                if (newargc < 2 || newargc > 3) {
                    fprintf(stderr, "expect one or two arguments\n");
                    continue;
                }
                if (setenv(newargv[1], newargc > 2 ? newargv[2] : NULL, 1) < 0)
                    perror("setenv");
                continue;
            }
            if (!strcmp(newargv[0], "wait")) {
                if (newargc != 2) {
                    fprintf(stderr, "expect one arguments\n");
                    continue;
                }
                if (waitpid(atoi(newargv[1]), NULL, 0) == -1)
                    perror("waitpid");
                continue;
            }

            int pfds[2];
            if (do_pipe) {
                if (pipe(pfds) < 0) {
                    perror("pipe");
                    return EXIT_FAILURE;
                }
            }

            pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (!pid) {
                if (background) {
                     fprintf(stderr, "[1] %d\n", getpid());
                     if (!do_pipe && !input_path)
                         input_path = "/dev/null";
                     setpgrp();
                }
                if (input_path) {
                    int fd = open(input_path, O_RDONLY);
                    if (fd < 0) {
                        perror(input_path);
                        return EXIT_FAILURE;
                    }
                    dup2(fd, STDIN_FILENO);
                } else if (pipe0 >= 0) {
                    dup2(pipe0, STDIN_FILENO);
                    close(pipe0);
                }

                if (output_path) {
                    int fd = open(output_path, O_WRONLY|O_CREAT, 0666);
                    if (fd < 0) {
                        perror(output_path);
                        return EXIT_FAILURE;
                    }
                    dup2(fd, STDOUT_FILENO);
                } else if (do_pipe) {
                    dup2(pfds[1], STDOUT_FILENO);
                    close(pfds[1]);
                }

                //printf("(as pid %d) exec: %s\n", getpid(), newargv[0]);
                execvp(newargv[0], newargv);
                fprintf(stderr, "%s: %s: command not found\n", argv[0], cmdline);
                return EXIT_FAILURE;
            } else {
                if (first_pid < 0)
                    first_pid = pid;

                setpgid(pid, first_pid);
                if (!background) {
                    tcsetpgrp(STDIN_FILENO, first_pid);

                    if (pipe0 >= 0)
                        close(pipe0);

                    int wpid, status;
                    do {
                        wpid = waitpid(pid, &status, 0);
                    } while (wpid >= 0 && wpid != pid);
                    ret = WEXITSTATUS(status);

                    signal(SIGTTOU, SIG_IGN);
                    tcsetpgrp(STDIN_FILENO, getpgrp());
                }
            }
            if (do_pipe) {
                pipe0 = pfds[0];
                close(pfds[1]);
            }
        }

        if (pipe0 >= 0)
            close(pipe0);

        if (single_command)
            break;
    }

    return ret;
}
