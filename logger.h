#ifndef LOG_DAEMON_LOGGER_H
#define LOG_DAEMON_LOGGER_H

#include <unistd.h>
#include <stdio.h>

struct logger {
    char **msg_array;
    unsigned int msg_count;
    unsigned int msg_capacity;
};

void init_logger(struct logger *logger_instance);
void release_logger(struct logger *logger_instance);

void logger_push_message(struct logger *logger_instance, char *msg, ssize_t msg_len);
/**
 * The returned message is owned by the logger. Calling release_logger frees it.
 */
char *logger_find_most_common_message(struct logger *logger_instance);

#endif //LOG_DAEMON_LOGGER_H