#ifndef ERROR_H
#define ERROR_H

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOG_OFF     0
#define LOG_ERR     1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_VERBOSE 4
#define LOG_DEBUG   5
#define LOG_TRACE   6

#define STDERR_PREFIX "[%s] "
#define VERBOSE_PREFIX "[%s] "
#define DEBUG_PREFIX "[%s(%s:%d)::DEBUG] "
#define WARNING_PREFIX "[%s::WARNING]\033[1;33m "
#define ERROR_PREFIX "[%s::ERROR]\033[1;31m "
#define INFO_PREFIX "[%s::INFO]\033[1;34m "
#define SUCCESS_PREFIX "[%s::SUCCESS]\033[1;32m "
#define DEBUG2_PREFIX "[%s::DEBUG] "
#define NO_COLOUR "\033[0m\n"

#define VERBOSE(msg, ...) { \
    if (slow5tools_verbosity_level >= LOG_VERBOSE) { \
        fprintf(stderr, VERBOSE_PREFIX msg "\n", __func__, __VA_ARGS__); \
    } \
}

#define DEBUG(msg, ...) { \
    if (slow5tools_verbosity_level >= LOG_DEBUG) { \
        fprintf(stderr, DEBUG_PREFIX msg "\n", __func__, __FILE__, __LINE__, __VA_ARGS__); \
    } \
}

#define WARNING(msg, ...) { \
    if (slow5tools_verbosity_level >= LOG_WARN) { \
        fprintf(stderr, WARNING_PREFIX msg NO_COLOUR, __func__, __VA_ARGS__); \
    } \
}

#define ERROR(msg, ...)  { \
    if (slow5tools_verbosity_level >= LOG_ERR) { \
        fprintf(stderr, ERROR_PREFIX msg NO_COLOUR, __func__, __VA_ARGS__); \
    } \
}

#define INFO(msg, ...)  { \
    if (slow5tools_verbosity_level >= LOG_INFO) { \
        fprintf(stderr, INFO_PREFIX msg NO_COLOUR, __func__, __VA_ARGS__); \
    } \
}

#define STDERR(msg, ...) fprintf(stderr, STDERR_PREFIX msg "\n", __func__, __VA_ARGS__)
#define SUCCESS(msg, ...)  fprintf(stderr, SUCCESS_PREFIX msg NO_COLOUR, __func__, __VA_ARGS__)
#define DEBUG2(msg, ...) fprintf(stderr, DEBUG2_PREFIX "Error occured at %s:%d. " msg NO_COLOUR, \
                                 __func__, __FILE__, __LINE__ - 1, __VA_ARGS__) // TODO why -2

#define EXIT_MSG(exit_code, argv, meta)  { \
    DEBUG("exiting with %s (exit code %d)", \
                    exit_code == EXIT_SUCCESS ? "SUCCESS" : \
                    exit_code == EXIT_FAILURE ? "FAILURE" : \
                    "UNKNOWN OUTCOME", exit_code); \
}


#define MALLOC_CHK(ret) malloc_chk((void*) ret, __func__, __FILE__, __LINE__ - 1)
#define F_CHK(ret, filename) f_chk((void*) ret, __func__, __FILE__, __LINE__ - 1, filename)
#define NULL_CHK(ret) null_chk((void*) ret, __func__, __FILE__, __LINE__ - 1)
#define NEG_CHK(ret) neg_chk(ret, __func__, __FILE__, __LINE__ - 1)

static inline void malloc_chk(void* ret, const char* func, const char* file,
                              int line) {
    if (ret == NULL) {
        fprintf(stderr, ERROR_PREFIX "Failed to allocate memory : %s.", func, strerror(errno));
        fprintf(stderr, DEBUG2_PREFIX "Error occured at %s:%d. ", func, file, line);
        exit(EXIT_FAILURE);
    }
}

static inline void f_chk(void* ret, const char* func, const char* file,
                         int line, const char* fopen_f) {
    if (ret == NULL) {
        fprintf(stderr, ERROR_PREFIX "Failed to open %s : %s." NO_COLOUR, func, fopen_f, strerror(errno));
        fprintf(stderr, DEBUG2_PREFIX "Error occured at %s:%d." , func, file, line);
        exit(EXIT_FAILURE);
    }
}

static inline void null_chk(void* ret, const char* func, const char* file,
                            int line) {
    if (ret == NULL) {
        fprintf(stderr, ERROR_PREFIX "Unexpected NULL value : %s." NO_COLOUR, func, strerror(errno));
        fprintf(stderr, DEBUG2_PREFIX "Error occured at %s:%d." , func, file, line);
        exit(EXIT_FAILURE);
    }
}

static inline void neg_chk(int ret, const char* func, const char* file,
                           int line) {
    if (ret < 0) {
        fprintf(stderr, ERROR_PREFIX "Unexpected negative value : %s." NO_COLOUR, func, strerror(errno));
        fprintf(stderr, DEBUG2_PREFIX "Error occured at %s:%d." ,func, file, line);
        exit(EXIT_FAILURE);
    }
}

#endif
