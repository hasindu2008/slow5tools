#ifndef CMD_H
#define CMD_H

#define LOG_OFF     0
#define LOG_ERR     1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_VERBOSE 4
#define LOG_DEBUG   5
#define LOG_TRACE   6

#define SLOW5TOOLS_VERSION "0.1.0-dirty"

struct program_meta {
    int verbosity_level;
};

struct command {
    const char *name;
    int (*main)(int, char **, struct program_meta *);
};

#define EXIT_MSG(exit_code, argv, meta) exit_msg(exit_code, argv, meta, __FILE__, __func__, __LINE__);

static inline void exit_msg(const int exit_code, char **argv, struct program_meta *meta,
                            const char *file, const char *func, const int line) {
    if (meta != NULL) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("exiting with %s",
                    exit_code == EXIT_SUCCESS ? "SUCCESS" :
                    exit_code == EXIT_FAILURE ? "FAILURE" :
                    "UNKNOWN OUTCOME");
        }
        if (meta->verbosity_level >= LOG_DEBUG) {
            fprintf(stderr, DEBUG_PREFIX "exit code %d" NO_COLOUR,
                    file, func, line, exit_code);
        }
    }
}

#endif
