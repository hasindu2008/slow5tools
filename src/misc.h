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


typedef struct {
    // Default options
    FILE *f_out;
    enum slow5_fmt fmt_in;
    enum slow5_fmt fmt_out;
    enum slow5_press_method record_press_out;
    enum slow5_press_method signal_press_out;
    size_t num_threads;
    size_t num_processes;
    int64_t read_id_batch_capacity;
    int flag_lossy;
    int flag_allow_run_id_mismatch;
    int flag_dump_all;

    // Input arguments
    char *arg_fname_in;
    char *arg_fname_out;
    char *arg_fmt_in;
    char *arg_fmt_out;
    char *arg_record_press_out;
    char *arg_signal_press_out;
    char *arg_num_threads;
    char *arg_num_processes;
    char *arg_batch;
    char *arg_dir_out;
    char *arg_lossless;
    char *arg_dump_all;

} opt_t;


enum slow5_fmt parse_name_to_fmt(const char *fmt_str);
enum slow5_fmt parse_path_to_fmt(const char *fname);
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

void print_args(int argc, char **argv);

void init_opt(opt_t *opt);
int parse_num_threads(opt_t *opt, int argc, char **argv, struct program_meta *meta);
int parse_num_processes(opt_t *opt, int argc, char **argv, struct program_meta *meta);
int parse_arg_lossless(opt_t *opt, int argc, char **argv, struct program_meta *meta);
int parse_arg_dump_all(opt_t *opt, int argc, char **argv, struct program_meta *meta);
int parse_batch_size(opt_t *opt, int argc, char **arg);
int parse_format_args(opt_t *opt, int argc, char **argv, struct program_meta *meta);
int auto_detect_formats(opt_t *opt, int set_default_output_format = 1);
int parse_compression_opts(opt_t *opt);

#ifdef __cplusplus
}
#endif

#endif
