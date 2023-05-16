#ifndef LOG_DAEMON_LOGGER_H
#define LOG_DAEMON_LOGGER_H

#include <unistd.h>
#include <stdio.h>

struct logger {
    char **msg_array;
    unsigned int msg_count;
    unsigned int msg_capacity;
    int *_output_fds;
    unsigned int _output_fds_count;
};

void init_logger(struct logger *logger_instance, char **output_files, int output_files_count);
void release_logger(struct logger *logger_instance);

void logger_push_message(struct logger *logger_instance, char *msg, ssize_t msg_len);
/**
 * The returned message is owned by the logger. Calling release_logger frees it.
 */
char *logger_find_most_common_message(struct logger *logger_instance);

#endif //LOG_DAEMON_LOGGER_H