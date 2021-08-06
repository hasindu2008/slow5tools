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
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//#include "slow5_error.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define MIN(A,B) ( ( (A) < (B) ) ? (A) : (B) )
//#define MAX(A,B) ( ( (A) > (B) ) ? (A) : (B) )


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


// Other

// Prints to the provided buffer a nice number of bytes (KB, MB, GB, etc)
// From https://www.mbeckler.org/blog/?p=114
//static inline void slow5_print_size(const char* name, uint64_t bytes)
//{
//    const char* suffixes[7];
//    suffixes[0] = "B";
//    suffixes[1] = "KB";
//    suffixes[2] = "MB";
//    suffixes[3] = "GB";
//    suffixes[4] = "TB";
//    suffixes[5] = "PB";
//    suffixes[6] = "EB";
//    uint64_t s = 0; // which suffix to use
//    double count = bytes;
//    while (count >= 1024 && s < 7)
//    {
//        s++;
//        count /= 1024;
//    }
//    if (count - floor(count) == 0.0)
//        fprintf(stderr, "[%s] %s : %d %s\n", __func__ , name, (int)count, suffixes[s]);
//    else
//        fprintf(stderr, "[%s] %s : %.1f %s\n", __func__, name, count, suffixes[s]);
//}


#ifdef __cplusplus
}
#endif

#endif
