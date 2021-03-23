#ifndef SLOW5_ERROR_H
#define SLOW5_ERROR_H

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* Debug and verbosity */
enum slow5_log_level_opt {
    SLOW5_LOG_OFF,
    SLOW5_LOG_ERR,
    SLOW5_LOG_WARN,
    SLOW5_LOG_INFO,
    SLOW5_LOG_DEBUG
};

enum slow5_exit_condition_opt {
    SLOW5_EXIT_OFF,
    SLOW5_EXIT_ON_ERR,
    SLOW5_EXIT_ON_WARN
};


#define SLOW5_WARNING_PREFIX "[%s::WARNING]\033[1;33m "
#define SLOW5_ERROR_PREFIX "[%s::ERROR]\033[1;31m "
#define SLOW5_NO_COLOUR "\033[0m\n"

#define SLOW5_WARNING(msg, ...) { \
        if (slow5_log_level == SLOW5_LOG_WARN) { \
                fprintf(stderr, SLOW5_WARNING_PREFIX msg SLOW5_NO_COLOUR, __func__, __VA_ARGS__); \
                fprintf(stderr, "At %s:%d\n", __FILE__, __LINE__ - 1); \
        } \
        if (slow5_exit_condition == SLOW5_EXIT_ON_WARN){ \
                fprintf(stderr,"Exiting on warning.\n"); \
                exit(EXIT_FAILURE); \
        } \
}

#define SLOW5_ERROR(msg, ...) { \
        if (slow5_log_level == SLOW5_LOG_ERR) { \
                fprintf(stderr, SLOW5_ERROR_PREFIX msg SLOW5_NO_COLOUR, __func__, __VA_ARGS__); \
                fprintf(stderr, "At %s:%d\n", __FILE__, __LINE__ - 1); \
        } \
        if (slow5_exit_condition == SLOW5_EXIT_ON_ERR){ \
                fprintf(stderr,"Exiting on error.\n"); \
                exit(EXIT_FAILURE); \
        } \
}


#endif
