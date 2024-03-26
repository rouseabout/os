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
    bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(fd, 5);

    fprintf(stderr, "server: accepting...\n");

    struct sockaddr_un client_addr;
    socklen_t len = sizeof(client_addr);
    int cfd = accept(fd, (struct sockaddr *) &client_addr, &len);
    if (cfd < 0) {
        perror("accept");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "server: waiting for data...\n");

    char buf[1024];
    int ret;
    while ((ret = read(cfd, buf, sizeof(buf))) > 0) {
        fprintf(stderr, "server: read '%.*s'\n", ret, buf);
        write(cfd, buf, ret);
    }
    if (ret < 0) {
        perror("read");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "server: closing\n");

    close(cfd);
    close(fd);

    return EXIT_SUCCESS;
}
