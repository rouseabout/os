#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main()
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, "unix_socket");

    fprintf(stderr, "ping: connecting\n");

    int ret = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }

    const char * ping = "PING";
    write(fd, ping, strlen(ping));

    char buf[1024];
    ret = read(fd, buf, sizeof(buf));
    if (ret < 0) {
        perror("read");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "ping: read: '%.*s'\n", ret, buf);
    fprintf(stderr, "ping: closing\n");

    close(fd);

    return EXIT_SUCCESS;
}
