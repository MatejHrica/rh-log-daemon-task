#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>

#define LOG_SOCKET_PATH "/dev/log"

/**
 * Finds the index of the actual start of the message.
 * For example for "<13>May 16 16:19:06 root: hello" returns 26.
 */
static ssize_t find_message_start(const char *message, ssize_t message_length) {
    for (ssize_t i = 0; i < message_length - 1; i++) {
        if (message[i] == ':' && message[i + 1] == ' ') {
            return i;
        }
    }
    return -1;
}

struct stored_messages {
    char **message_array;
    unsigned int count;
    unsigned int capacity;
};

static void init_stored_messages(struct stored_messages *messages) {
    messages->count = 0;
    messages->capacity = 32;
    messages->message_array = malloc(messages->capacity * sizeof(messages[0]));
}

static void release_stored_messages(struct stored_messages *messages) {
    for (unsigned int i = 0; i < messages->count; i++) {
        free(messages->message_array[i]);
    }
    free(messages->message_array);
    messages->message_array = NULL;
    messages->count = 0;
    messages->capacity = 0;
}

static void push_message(struct stored_messages *messages, const char *message, ssize_t message_length) {
    assert(messages != NULL);
    assert(messages->message_array != NULL);
    assert(message_length > 0);
    assert(message != NULL);


    if (messages->count == messages->capacity) {
        messages->capacity *= 2;
        messages->message_array = realloc(messages->message_array, messages->capacity * sizeof(messages[0]));
    }

    ssize_t actual_message_start = find_message_start(message, message_length);
    assert(actual_message_start <= message_length);
    if (actual_message_start < 0) {
        fprintf(stderr, "Received invalid message: \"");
        fwrite(message, message_length, 1, stderr);
        fprintf(stderr, "\"\n");
        return;
    }

    size_t actual_message_length = message_length - actual_message_start;
    char *actual_message = malloc(actual_message_length + 1);
    strncpy(actual_message, message + actual_message_start, actual_message_length);
    actual_message[actual_message_length] = '\0';

    messages->message_array[messages->count++] = actual_message;
}

static void print_info(struct stored_messages *messages) {
    printf("Number of messages: %d\n", messages->count);
}


volatile sig_atomic_t sigint_received = 0;

void sigint_handler(int signal) {
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

int main() {
    struct stored_messages messages;
    init_stored_messages(&messages);
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
        ssize_t bytes_read = read(log_socket, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            perror("read");
        } else if (bytes_read != 0) {
            printf("log msg received\n");
            push_message(&messages, buffer, bytes_read);
        }
    }

    print_info(&messages);
    release_stored_messages(&messages);

    close(log_socket);
    unlink(LOG_SOCKET_PATH);

    return EXIT_SUCCESS;
}