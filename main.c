#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define LOG_SOCKET_PATH "/dev/log"

int main() {
    struct sockaddr_un name;
    memset(&name, 0, sizeof(name));
    int log_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (log_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, LOG_SOCKET_PATH, sizeof(name.sun_path) - 1);

    if (bind(log_socket, (const struct sockaddr *) &name, sizeof(name)) != 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        char buffer[1024] = {0};
        ssize_t bytes_read = read(log_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        } else if (bytes_read != 0) {
            buffer[bytes_read] = '\0';
            printf("received: %s\n", buffer);
        };
    }

    close(log_socket);
    unlink(LOG_SOCKET_PATH);

    return EXIT_SUCCESS;
}