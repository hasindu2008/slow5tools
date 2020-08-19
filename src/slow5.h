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

#define SLOW5_VERSION "0.0"

struct program_meta {
    bool debug;
    bool verbose;
};

struct command {
    char *name;
    int (*main)(int, char **, struct program_meta *);
};

#endif
