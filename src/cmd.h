#ifndef CMD_H
#define CMD_H

#define SLOW5TOOLS_VERSION "0.2.0-dirty"

#define DEFAULT_NUM_THREADS 8
#define DEFAULT_BATCH_SIZE 4096

#define TO_STR(x) TO_STR2(x)
#define TO_STR2(x) #x

#define HELP_SMALL_MSG \
    "Try '%s --help' for more information.\n"


#define HELP_MSG_PRESS \
    "    -c, --compress REC_MTD        record compression method [zlib] (only for blow5 format)\n" \
    "    -s, --sig-compress SIG_MTD    signal compression method [none] (only for blow5 format)\n"

#define HELP_MSG_THREADS \
    "    -t, --threads INT             number of threads [" TO_STR(DEFAULT_NUM_THREADS) "]\n"

#define HELP_MSG_BATCH \
    "    -K, --batchsize INT           number of records loaded to the memory at once. [" TO_STR(DEFAULT_BATCH_SIZE) "]\n"


#define HELP_FORMATS_METHODS \
    "FORMATS:\n" \
    "    slow5 - SLOW5 ASCII\n" \
    "    blow5 - SLOW5binary (BLOW5)\n" \
    "REC_MTD:\n" \
    "    none - no record compression\n" \
    "    zlib - Z library (equivalent to gzip)\n" \
    "    zstd - Z standard \n" \
    "SIG_MTD:\n" \
    "    none - no special signal compression\n" \
    "    svb-zd - StreamVByte with zig-zag delta \n"

struct program_meta {
    int verbosity_level;
};

struct command {
    const char *name;
    int (*main)(int, char **, struct program_meta *);
};



#endif
