#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

/**
 * Finds the index of the actual start of the message.
 * For example for "<13>May 16 16:19:06 root: hello" returns 26.
 */
static ssize_t find_message_start(const char *message, ssize_t message_length) {
    for (ssize_t i = 0; i < message_length - 1; i++) {
        if (message[i] == ':' && message[i + 1] == ' ') {
            return i + 2;
        }
    }
    return -1;
}

void init_logger(struct logger *logger_instance, char **output_files, int output_files_count) {
    logger_instance->msg_count = 0;
    logger_instance->msg_capacity = 32;
    logger_instance->msg_array = malloc(logger_instance->msg_capacity * sizeof(logger_instance[0]));

    logger_instance->_output_fds_count = output_files_count;
    logger_instance->_output_fds = malloc((logger_instance->_output_fds_count) * sizeof(int));
    for (int i = 0; i < output_files_count; i++) {
        int fd = open(output_files[i], O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) {
            perror(output_files[i]);
            // TODO: should to properly propagate the error
            exit(EXIT_FAILURE);
        }
        logger_instance->_output_fds[i] = fd;
    }
}

void release_logger(struct logger *logger_instance) {
    for (unsigned int i = 0; i < logger_instance->msg_count; i++) {
        free(logger_instance->msg_array[i]);
    }
    free(logger_instance->msg_array);
    logger_instance->msg_array = NULL;
    logger_instance->msg_count = 0;
    logger_instance->msg_capacity = 0;

    for (unsigned int i = 0; i < logger_instance->_output_fds_count; i++) {
        if (close(logger_instance->_output_fds[i]) != 0) {
            perror("close");
        }
    }

    free(logger_instance->_output_fds);
    logger_instance->_output_fds = NULL;
    logger_instance->_output_fds_count = 0;
}

void write_all(int fd, const char *buffer, ssize_t buffer_len) {
    while (buffer_len > 0) {
        ssize_t bytes_written = write(fd, buffer, buffer_len);
        if (bytes_written < 0) {
            if (errno == EINTR) {
                perror("write");
            }
            return;
        }
        buffer += bytes_written;
        buffer_len -= bytes_written;
    }
}

void logger_push_message(struct logger *logger_instance, char *msg, ssize_t msg_len) {
    assert(logger_instance != NULL);
    assert(logger_instance->msg_array != NULL);
    assert(msg_len > 0);
    assert(msg != NULL);

    for (unsigned int i = 0; i < logger_instance->_output_fds_count; i++) {
        write_all(logger_instance->_output_fds[i], msg, msg_len);
    }
    write_all(STDOUT_FILENO, msg, msg_len);

    if (logger_instance->msg_count == logger_instance->msg_capacity) {
        logger_instance->msg_capacity *= 2;
        logger_instance->msg_array = realloc(logger_instance->msg_array,
                                             logger_instance->msg_capacity * sizeof(logger_instance[0]));
    }

    ssize_t actual_message_start = find_message_start(msg, msg_len);
    assert(actual_message_start <= msg_len);
    if (actual_message_start < 0) {
        fprintf(stderr, "Received invalid msg: \"");
        fwrite(msg, msg_len, 1, stderr);
        fprintf(stderr, "\"\n");
        return;
    }

    size_t actual_message_length = msg_len - actual_message_start;
    char *actual_message = malloc(actual_message_length + 1);
    strncpy(actual_message, msg + actual_message_start, actual_message_length);
    actual_message[actual_message_length] = '\0';

    logger_instance->msg_array[logger_instance->msg_count++] = actual_message;
}

static int sort_strcmp(const void *a, const void *b) {
    return strcmp(*(char **) a, *(char **) b);
}

char *logger_find_most_common_message(struct logger *logger_instance) {
    if (logger_instance->msg_count == 0) {
        return NULL;
    }

    size_t size = logger_instance->msg_count * sizeof(char *);
    char **message_array = malloc(size);
    memcpy(message_array, logger_instance->msg_array, size);

    qsort(message_array, logger_instance->msg_count, sizeof(char *), &sort_strcmp);
    unsigned int count = 0;
    char *most_common_message = NULL;
    for (unsigned int i = 0; i < logger_instance->msg_count; i++) {
        if (count == 0 || strcmp(message_array[i], most_common_message) != 0) {
            most_common_message = message_array[i];
            count = 1;
        } else {
            count++;
        }
    }
    free(message_array);

    return most_common_message;
}
