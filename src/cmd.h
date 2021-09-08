#ifndef CMD_H
#define CMD_H

#define SLOW5TOOLS_VERSION "0.2.0-dirty"

#define DEFAULT_NUM_THREADS (4)
#define READ_ID_BATCH_CAPACITY (4096)

#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"

#define HELP_FORMATS_METHODS \
    "FORMATS:\n" \
    "    slow5\n" \
    "    blow5\n" \
    "REC_METHODS:\n" \
    "    none\n" \
    "    zlib\n" \
    "    zstd\n" \
    "SIG_METHODS:\n" \
    "    none\n" \
    "    svb-zd\n"

struct program_meta {
    int verbosity_level;
};

struct command {
    const char *name;
    int (*main)(int, char **, struct program_meta *);
};


#endif
