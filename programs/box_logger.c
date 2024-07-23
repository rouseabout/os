#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

static int logger_main(int argc, char ** argv, char ** envp)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s MESSAGE\n", argv[0]);
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "%s", argv[1]);
    return EXIT_SUCCESS;
}
