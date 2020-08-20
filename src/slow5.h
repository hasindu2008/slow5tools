/* @f5c
**
** f5c interface
** @author: Hasindu Gamaarachchi (hasindu@garvan.org.au)
** @@
******************************************************************************/

#ifndef F5C_H
#define F5C_H

//#include "fast5lite.h"
//#include "ftidx.h"

//required for eventalign
//#include <vector>

#include "error.h"

#define SLOW5_VERSION "0.0"

struct program_meta {
    bool debug;
    bool verbose;
};

struct command {
    char *name;
    int (*main)(int, char **, struct program_meta *);
};

# define EXIT_MSG(exit_code, argv, meta) exit_msg(exit_code, argv, meta, __FILE__, __func__, __LINE__);

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
