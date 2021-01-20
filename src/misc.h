// Miscellaneous definitions and functions

#ifndef MISC_H
#define MISC_H

#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "error.h"

//#define MIN(A,B) ( ( (A) < (B) ) ? (A) : (B) )
//#define MAX(A,B) ( ( (A) > (B) ) ? (A) : (B) )

// Timing

// From minimap2/misc
static inline double realtime(void) {
    struct timeval tp;
    struct timezone tzp;
    gettimeofday(&tp, &tzp);
    return tp.tv_sec + tp.tv_usec * 1e-6;
}

// From minimap2/misc
static inline double cputime(void) {
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_stime.tv_sec +
           1e-6 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}

// From minimap2
static inline long peakrss(void) {
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
static inline void print_size(const char* name, uint64_t bytes)
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

// From https://stackoverflow.com/questions/26522583/c-strtok-skips-second-token-or-consecutive-delimiter
char *strtok_solo(char *str, char *seps);

// Atoi but to uintx_t
// and without any symbols
// and without 0 prefixing
uint8_t ato_uint8(const char *str);
uint32_t ato_uint32(const char *str);

#endif
