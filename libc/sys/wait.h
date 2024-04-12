#ifndef SYS_WAIT_H
#define SYS_WAIT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WNOHANG 0x1
#define WUNTRACED 0x2

#define WEXITSTATUS(x) (((x) >> 8) & 0xFF)
#define WTERMSIG(x) ((x) & 0x7F)
#define WSTOPSIG(x) (WTERMSIG(x) != 0)
#define WIFEXITED(x) (WTERMSIG(x) == 0)
#define WIFSTOPPED(x) ((x) >> 16)
#define WIFSIGNALED(x) 0

pid_t wait(int * stat_loc);
pid_t waitpid(pid_t pid, int * stat_loc, int options);

//wait3 was removed in issue 6
#define wait3(stat_loc, options, resource_usage) waitpid((pid_t)-1, stat_loc, options)

#ifdef __cplusplus
}
#endif

#endif /* SYS_WAIT_H */
