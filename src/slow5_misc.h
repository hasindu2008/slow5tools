// Miscellaneous definitions and functions

#ifndef SLOW5_MISC_H
#define SLOW5_MISC_H

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
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define MIN(A,B) ( ( (A) < (B) ) ? (A) : (B) )
//#define MAX(A,B) ( ( (A) > (B) ) ? (A) : (B) )

// Types to sizes

#define IS_TYPE(str, type) (strcmp(str, # type) == 0)
#define IS_TYPE_TRUNC(str, type) (strncmp(str, # type, sizeof (# type) - 1) == 0)

#define CHECK_TYPE(name, type) \
    (IS_TYPE_TRUNC(name, type)) { \
        if (strcmp(name + strlen(name) - 2, "**") == 0) {/* TODO this is not right */ \
            type *x; \
            size = sizeof (x); \
        } else { \
            size = sizeof (type); \
        } \
    }


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


// FAST5

bool has_fast5_ext(const char *f_path);


// Other

// Prints to the provided buffer a nice number of bytes (KB, MB, GB, etc)
// From https://www.mbeckler.org/blog/?p=114
static inline void slow5_print_size(const char* name, uint64_t bytes)
{
    const char* suffixes[7];
    suffixes[0] = "B";
    suffixes[1] = "KB";
    suffixes[2] = "MB";
    suffixes[3] = "GB";
    suffixes[4] = "TB";
    suffixes[5] = "PB";
    suffixes[6] = "EB";
    uint64_t s = 0; // which suffix to use
    double count = bytes;
    while (count >= 1024 && s < 7)
    {
        s++;
        count /= 1024;
    }
    if (count - floor(count) == 0.0)
        fprintf(stderr, "[%s] %s : %d %s\n", __func__ , name, (int)count, suffixes[s]);
    else
        fprintf(stderr, "[%s] %s : %.1f %s\n", __func__, name, count, suffixes[s]);
}

// adapted from https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
static inline bool is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == -1) {
        ERROR("Stat failed to retrive file information%s", "");
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}

static inline int slow5_is_big_endian(void)
{
    long one= 1;
    return !(*((char *)(&one)));
}

// sprintf and vsprintf but dynamically allocates strp memory
int asprintf_mine(char **strp, const char *fmt, ...);
int vasprintf_mine(char **strp, const char *fmt, va_list ap);

// From https://code.woboq.org/userspace/glibc/string/strsep.c.html
char *strsep_mine (char **stringp, const char *delim);

// Check that int/float is in a certain format
int int_check(const char *str);
int float_check(const char *str);

// Atoi but to xintx_t
// and without any symbols
// and without 0 prefixing
int8_t ato_int8(const char *str, int *err);
int16_t ato_int16(const char *str, int *err);
int32_t ato_int32(const char *str, int *err);
int64_t ato_int64(const char *str, int *err);
uint8_t ato_uint8(const char *str, int *err);
uint16_t ato_uint16(const char *str, int *err);
uint32_t ato_uint32(const char *str, int *err);
uint64_t ato_uint64(const char *str, int *err);

// Strtod and strtof but
// without any symbols, spaces
// only in decimal form
double strtod_check(const char *str, int *err);
float strtof_check(const char *str, int *err);

// Convert double to decimal string without trailing 0s
char *double_to_str(double x, size_t *len);
// Convert float to decimal string without trailing 0s
char *float_to_str(float x, size_t *len);

#ifdef __cplusplus
}
#endif

#endif
