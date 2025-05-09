#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <string.h>

#define READ_FILE(buftype, buf, size, path) \
    size_t size; \
    buftype * buf; \
    if (!strcmp(path, "-")) { \
        size = 0; \
        buf = NULL; \
        buftype tmp[1024]; \
        for (int n; (n = read(STDIN_FILENO, tmp, sizeof(tmp))); ) { \
            buf = realloc(buf, size + n); \
            if (!buf) { \
                perror("realloc"); \
                return EXIT_FAILURE; \
            } \
            memcpy(buf + size, tmp, n); \
            size += n; \
        } \
    } else { \
        int fd = open(path, O_RDONLY); \
        if (fd == -1) { \
            perror("open"); \
            return EXIT_FAILURE; \
        } \
        size = lseek(fd, 0, SEEK_END); \
        buf = malloc(size + 1); \
        if (!buf) { \
            perror("malloc"); \
            return EXIT_FAILURE; \
        } \
        lseek(fd, 0, SEEK_SET); \
        size = read(fd, buf, size); \
        if (size == -1) { \
            perror("read"); \
            return EXIT_FAILURE; \
        } \
        close(fd); \
    } \

#include "box_cal.c"
#include "box_cat.c"
#include "box_chmod.c"
#include "box_cksum.c"
#include "box_clear.c"
#include "box_cmp.c"
#include "box_date.c"
#include "box_dd.c"
#include "box_df.c"
#include "box_echo.c"
#include "box_env.c"
#include "box_expr.c"
#include "box_false.c"
#include "box_find.c"
#include "box_grep.c"
#include "box_head.c"
#include "box_kill.c"
#include "box_ln.c"
#include "box_logger.c"
#include "box_ls.c"
#include "box_mkdir.c"
#include "box_mktemp.c"
#include "box_more.c"
#include "box_mv.c"
#include "box_pwd.c"
#include "box_reset.c"
#include "box_rm.c"
#include "box_rmdir.c"
#include "box_sleep.c"
#include "box_sort.c"
#include "box_strings.c"
#include "box_tail.c"
#include "box_tee.c"
#include "box_touch.c"
#include "box_tr.c"
#include "box_true.c"
#include "box_uname.c"
#include "box_vi.c"
#include "box_wc.c"
#include "box_xargs.c"

static const struct {
    const char * name;
    int (*main)(int, char **, char **);
} programs[] = {
    {"cal", cal_main},
    {"cat", cat_main},
    {"chmod", chmod_main},
    {"cksum", cksum_main},
    {"clear", clear_main},
    {"cmp", cmp_main},
    {"date", date_main},
    {"dd", dd_main},
    {"df", df_main},
    {"echo", echo_main},
    {"env", env_main},
    {"expr", expr_main},
    {"false", false_main},
    {"find", find_main},
    {"grep", grep_main},
    {"head", head_main},
    {"kill", kill_main},
    {"ln", ln_main},
    {"logger", logger_main},
    {"ls", ls_main},
    {"mkdir", mkdir_main},
    {"mktemp", mktemp_main},
    {"more", more_main},
    {"mv", mv_main},
    {"pwd", pwd_main},
    {"reset", reset_main},
    {"rm", rm_main},
    {"rmdir", rmdir_main},
    {"sleep", sleep_main},
    {"sort", sort_main},
    {"strings", strings_main},
    {"tail", tail_main},
    {"tee", tee_main},
    {"touch", touch_main},
    {"tr", tr_main},
    {"true", true_main},
    {"uname", uname_main},
    {"vi", vi_main},
    {"wc", wc_main},
    {"xargs", xargs_main},
};

int main(int argc, char ** argv, char ** envp)
{
    const char * name = basename(argv[0]);
    for (int i = 0 ; i < sizeof(programs)/sizeof(programs[0]); i++)
        if (!strcmp(programs[i].name, name))
            return programs[i].main(argc, argv, envp);
    fprintf(stderr, "known commands:");
    for (int i = 0 ; i < sizeof(programs)/sizeof(programs[0]); i++)
        fprintf(stderr, " %s", programs[i].name);
    fprintf(stderr, "\n");
    return EXIT_FAILURE;
}
