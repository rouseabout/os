#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

static int touch_main(int argc, char ** argv, char ** envp)
{
    if (argc !=2)
        return -1;

    int fd = open(argv[1], O_WRONLY|O_CREAT, 0666);
    if (fd < 0) {
        perror(argv[1]);
        return -1;
    }
    close(fd);

    struct utimbuf times;
    times.actime = times.modtime = time(NULL);
    int ret = utime(argv[1], &times);
    if (ret < 0) {
        perror(argv[1]);
        return -1;
    }

    return 0;
}
