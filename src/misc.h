// Miscellaneous definitions and functions

#ifndef _MISC_H_
#define _MISC_H_

#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <slow5/slow5.h>
#include "slow5_extra.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif
//Parsing
enum slow5_press_method name_to_slow5_press_method(const char *name);

struct parse_fmt_meta {
    enum slow5_fmt format;
    const char *name;
    const char *ext;
};

static const struct parse_fmt_meta PARSE_FORMAT_META[] = {
        { SLOW5_FORMAT_ASCII,  SLOW5_ASCII_NAME,     SLOW5_ASCII_EXTENSION     },
        { SLOW5_FORMAT_BINARY, SLOW5_BINARY_NAME,    SLOW5_BINARY_EXTENSION    },
};

enum slow5_fmt parse_name_to_fmt(const char *fmt_str);
enum slow5_fmt parse_path_to_fmt(const char *fname);
int check_aux_fields_in_record(slow5_rec *slow5_record, const char *attr, int verbose);
int check_aux_fields_in_header(slow5_hdr *slow5_header, const char *attr, int verbose);
// Timing
// From minimap2/misc
static inline double slow5_realtime(void) {
    struct timeval tp;
    struct timezone tzp;
    gettimeofday(&tp, &tzp);
    return tp.tv_sec + tp.tv_usec * 1e-6;
}

// From minimap2/misc
static inline double slow5_cputime(void) {
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_stime.tv_sec +
           1e-6 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}

// From minimap2
static inline long slow5_peakrss(void) {
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
#ifdef __linux__
	return r.ru_maxrss * 1024;
#else
	return r.ru_maxrss;
#endif

}

#ifdef __cplusplus
}
#endif

#endif
