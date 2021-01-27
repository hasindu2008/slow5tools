// Header with slow5 file definitions
// TODO structure pack to min size

#ifndef SLOW5_H
#define SLOW5_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "klib/khash.h"
#include "press.h"
#include "slow5_defs.h"

/* Formats */

// File formats to be dealing with
enum slow5_fmt {
    FORMAT_UNKNOWN,
    FORMAT_ASCII,
    FORMAT_BINARY
};

// SLOW5 file name with corresponding format
struct slow5_fmt_meta {
    const char *name;
    enum slow5_fmt format;
};
static const struct slow5_fmt_meta SLOW5_FORMAT_META[] = {
    { ASCII_NAME,   FORMAT_ASCII    },
    { BINARY_NAME,  FORMAT_BINARY   }
};

/* Header */

// SLOW5 versioning
struct slow5_version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

// Header data map: attribute string -> data string
KHASH_MAP_INIT_STR(s2s, const char *)

// SLOW5 header
struct slow5_hdr {
	struct slow5_version version;
    char *version_str;
    uint32_t num_read_groups; // Number of read groups
    khash_t(s2s) **data; // length = num_read_groups
};

/* Read Record */

// SLOW5 main record columns
enum slow5_cols {
    SLOW5_COLS_FOREACH(GENERATE_ENUM)
    SLOW5_COLS_NUM
};

// SLOW5 auxillary record data
// TODO make dynamic
struct slow5_rec_aux {
	char *channel_number;
    char *end_reason;
    double median_before;
	int32_t read_number;
	uint64_t start_time;
    uint8_t start_mux;
};

// SLOW5 record data
struct slow5_rec {
    char *str;
    SLOW5_COLS_FOREACH(GENERATE_STRUCT)
    struct slow5_rec_aux *read_aux;
};

/* SLOW5 file */

// SLOW5 file meta
struct slow5_file_meta {
    const char *pathname;
    bool is_fp_preowned;
    int fd;
    //enum press_mtd compress_hint;
};

// SLOW5 file structure
struct slow5_file {
    FILE *fp;
    enum slow5_fmt format;
    struct press *compress;
    struct slow5_hdr *header;
    struct slow5_idx *index;
    struct slow5_file_meta meta;
};


/* Public API */

// Open a slow5 file
struct slow5_file *slow5_open(const char *pathname, const char *mode);
struct slow5_file *slow5_open_with(const char *pathname, const char *mode, enum slow5_fmt format);
//struct slow5_file *slow5_init_fp(FILE *fp, enum slow5_fmt format);

// Write from a slow5 file to another slow5 file
int8_t slow5_write(struct slow5_file *s5p_to, struct slow5_file *s5p_from); // TODO decide return type

// Merge slow5 files to another slow5 file
// TODO Just a merge for 2 -> 1?
// TODO compile time 2 args in ...
int8_t slow5_merge(struct slow5_file *s5p_to, ...);
int8_t slow5_vmerge(struct slow5_file *s5p_to, va_list ap);

// Split a slow5 file to a dir
// TODO split into multiple slow5 files from same rg
int8_t slow5_split(const char *dirname_to, struct slow5_file *s5p_from);

// Close a slow5 file
void slow5_close(struct slow5_file *s5p);



// Get a read entry
void slow5_get(const char *read_id, struct slow5_rec **read, struct slow5_file *s5p);

// Get next read entry under file pointer
struct slow5_rec *slow5_get_next(struct slow5_file *s5p);

// Print read entry
int slow5_rec_fprint(FILE *fp, struct slow5_rec *read);
static inline int slow5_rec_print(struct slow5_rec *read) {
    return slow5_rec_fprint(stdout, read);
}

// Free a read entry
void slow5_rec_free(struct slow5_rec *read);



// Get a header value
const char *slow5_hdr_get(const char *attr, const struct slow5_file *s5p);
void slow5_hdr_print(const struct slow5_hdr *header);

#endif
