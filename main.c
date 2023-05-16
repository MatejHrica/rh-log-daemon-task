#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "logger.h"

#define LOG_SOCKET_PATH "/dev/log"

volatile sig_atomic_t sigint_received = 0;

static void sigint_handler(int signal) {
    (void) signal;
    sigint_received = 1;
}

static void install_sigint_handler() {
    struct sigaction action = {0};
    action.sa_handler = &sigint_handler;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

static void print_statistics(struct logger *logger_instance) {
    fprintf(stderr, "Number of messages: %d\n", logger_instance->msg_count);
    char *most_frequent_message = logger_find_most_common_message(logger_instance);
    if (most_frequent_message != NULL) {
        fprintf(stderr, "The most frequent message is: \"%s\"\n", most_frequent_message);
    } else {
        fprintf(stderr, "The no most frequent message, because there are no messages!\n");
    }
}

int main(int argc, char *argv[]) {
    struct logger logger_instance;
    init_logger(&logger_instance, argv + 1, argc - 1);
    install_sigint_handler();

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

    while (sigint_received == 0) {
        char buffer[1024] = {0};
        ssize_t bytes_read = read(log_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            if (errno != EINTR) {
                perror("read");
            }
        } else if (bytes_read != 0) {
            buffer[bytes_read] = '\n';
            logger_push_message(&logger_instance, buffer, bytes_read + 1);
        }
    }

    print_statistics(&logger_instance);
    release_logger(&logger_instance);

    close(log_socket);
    unlink(LOG_SOCKET_PATH);

    return EXIT_SUCCESS;
}