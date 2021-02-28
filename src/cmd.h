#ifndef CMD_H
#define CMD_H

struct program_meta {
    bool debug;
    bool verbose;
};

struct command {
    const char *name;
    int (*main)(int, char **, struct program_meta *);
};

#define EXIT_MSG(exit_code, argv, meta) exit_msg(exit_code, argv, meta, __FILE__, __func__, __LINE__);

static inline void exit_msg(const int exit_code, char **argv, struct program_meta *meta,
                            const char *file, const char *func, const int line) {
    if (meta != NULL) {
        if (meta->verbose) {
            VERBOSE("exiting with %s",
                    exit_code == EXIT_SUCCESS ? "SUCCESS" :
                    exit_code == EXIT_FAILURE ? "FAILURE" :
                    "UNKNOWN OUTCOME");
        }
        if (meta->debug) {
            fprintf(stderr, DEBUG_PREFIX "exit code %d" NO_COLOUR,
                    file, func, line, exit_code);
        }
    }
}

#endif
