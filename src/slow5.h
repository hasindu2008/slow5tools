/* @slow5
**
** slow5 interface
** @author: Hasindu Gamaarachchi (hasindu@garvan.org.au)
** @author: Sasha Jenner (jenner.sasha@gmail.com)
** @@
******************************************************************************/

#ifndef SLOW5_H
#define SLOW5_H

#include "fast5lite.h"
#include "slow5idx.h"
#include "slow5misc.h"
#include "error.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif


//required for eventalign
//#include <vector>

#define FAST5_EXTENSION ".fast5"

#define SLOW5_VERSION "1.0"
#define SLOW5_HEADER \
    "#" \
    "read_id\t" \
    "n_samples\t" \
    "digitisation\t" \
    "offset\t" \
    "range\t" \
    "sample_rate\t" \
    "raw_signal\t" \
    "num_bases\t" \
    "sequence\t" \
    "fast5_path\n" 

struct program_meta {
    bool debug;
    bool verbose;
};

struct command {
    const char *name;
    int (*main)(int, char **, struct program_meta *);
};

// TODO in misc or here?
#define EXIT_MSG(exit_code, argv, meta) exit_msg(exit_code, argv, meta, __FILE__, __func__, __LINE__);

static inline void exit_msg(int exit_code, char **argv, struct program_meta *meta,
                            const char *file, const char *func, int line) {
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
