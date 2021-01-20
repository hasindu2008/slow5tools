// Header with slow5 file definitions
// TODO structure pack to min size

#ifndef SLOW5_H
#define SLOW5_H

#include <stdio.h>
#include <stdint.h>
#include "klib/khash.h"

// SLOW5 format specs
#define SLOW5_HEADER_PREFIX         "#"
#define SLOW5_HEADER_DATA_PREFIX    "@"
#define COLUMN_HEADER_PREFIX        "#"
#define SEP                         "\t"
#define HEADER_FILE_FORMAT          "file_format"
#define HEADER_FILE_VERSION         "file_version"
#define HEADER_NUM_GROUPS           "num_read_groups"
#define HEADER_NUM_GROUPS_INIT      (1)

// Order and type of main SLOW5 columns
#define SLOW5_COLS(col, end) \
    col(const char *,   read_id) \
    col(uint32_t,       read_group) \
    col(float,          digitisation) \
    col(double,         offset) \
    col(double,         range) \
    col(double,         sampling_rate) \
    col(uint64_t,       len_raw_signal) \
    end(int16_t *,      raw_signal) // Use end() for last column

// Apply the same function to each column including the last one
#define SLOW5_COLS_FOREACH(foo) SLOW5_COLS(foo, foo)

#define GENERATE_STRUCT(type, name)     type name;
#define GENERATE_ENUM(type, name)       COL_ ## name,
#define GENERATE_STRING(type, name)     #name
#define GENERATE_STRING_SEP(type, name) GENERATE_STRING(type, name) SEP

// More SLOW5 specs
#define SLOW5_HEADER_ID(header_name)    SLOW5_HEADER_PREFIX header_name
#define HEADER_FILE_FORMAT_ID           SLOW5_HEADER_ID(HEADER_FILE_FORMAT)
#define HEADER_FILE_VERSION_ID          SLOW5_HEADER_ID(HEADER_FILE_VERSION)
#define HEADER_NUM_GROUPS_ID            SLOW5_HEADER_ID(HEADER_NUM_GROUPS)

#define SLOW5_HEADER_ENTRY(header_name, data) SLOW5_HEADER_ID(header_name) SEP data "\n"

// ASCII SLOW5 specs
#define ASCII_VERSION           "0.1.0"
#define ASCII_NAME              "slow5"
#define ASCII_EXTENSION         "." ASCII_NAME
#define ASCII_FILE_FORMAT       SLOW5_HEADER_ENTRY(HEADER_FILE_FORMAT, ASCII_NAME)
#define ASCII_FILE_VERSION      SLOW5_HEADER_ENTRY(HEADER_FILE_VERSION, ASCII_VERSION)
#define ASCII_NUM_GROUPS        SLOW5_HEADER_ENTRY(NUM_GROUPS_HEADER, "%d")
#define ASCII_SLOW5_HEADER      ASCII_FILE_FORMAT ASCII_FILE_VERSION ASCII_NUM_GROUPS
#define ASCII_COLUMN_HEADER_MIN COLUMN_HEADER_PREFIX SLOW5_COLS(GENERATE_STRING_SEP, GENERATE_STRING)

// Binary SLOW5 specs
#define BINARY_VERSION          "0.1.0"
#define BINARY_NAME             "blow5"
#define BINARY_EXTENSION        "." BINARY_NAME
#define BINARY_FILE_FORMAT      SLOW5_HEADER_ENTRY(HEADER_FILE_FORMAT, BINARY_NAME)
#define BINARY_FILE_VERSION     SLOW5_HEADER_ENTRY(HEADER_FILE_VERSION, BINARY_VERSION)
#define BINARY_SLOW5_HEADER     BINARY_FILE_FORMAT BINARY_FILE_VERSION ASCII_NUM_GROUPS

/* Formats */

// File formats to be dealing with
enum slow5_format {
    FORMAT_UNKNOWN,
    FORMAT_ASCII,
    FORMAT_BINARY
};

// SLOW5 file name with corresponding format
struct slow5_format_map {
    const char *name;
    enum slow5_format format;
};
static const struct slow5_format_map SLOW5_FORMAT_MAP[] = {
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
struct slow5_header {
    enum slow5_format format;
	enum slow5_version version;
    char *version_str;
    uint32_t num_rgs; // Number of read groups
    khash_t(s2s) **data; // length = num_rgs
};

/* Read Record */

// SLOW5 main record columns
enum slow5_cols {
    SLOW5_COLS_FOREACH(GENERATE_ENUM)
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
    SLOW5_COLS_FOREACH(GENERATE_STRUCT)
    struct slow5_rec_aux *read_aux;
};

/* SLOW5 object */

// SLOW5 index
struct slow5_idx {
    uint64_t size;
    uint64_t offset;
};

// Read id map: read id -> index data
KHASH_MAP_INIT_STR(s2i, struct slow5_idx *)

/* SLOW5 file */

// SLOW5 file structure
struct slow5_file {
    FILE *fp;
    bool is_fp_preowned;
    enum slow5_format format;
    struct Press *compress;
    struct slow5_header *header;
    khash_t(s2i) *idx;
};


/* Public API */

// Open a slow5 file
struct slow5_file *slow5_open(const char *pathname, const char *mode);

struct slow5_file *slow5_open_with(const char *pathname, const char *mode, enum slow5_format format, PressMethod compress);
struct slow5_file *slow5_init_fp(FILE *fp, enum slow5_format format, PressMethod compress);

// Write from a slow5 file to another slow5 file
int8_t slow5_write(struct slow5_file *s5p_from, struct slow5_file *s5p_to); // TODO decide return type

// Merge slow5 files to another slow5 file
// TODO Just a merge for 2 -> 1?
// TODO compile time 2 args in ...
int8_t slow5_merge(struct slow5_file *s5p_to, ...);
int8_t slow5_vmerge(struct slow5_file *s5p_to, va_list ap);

// Split a slow5 file to a dir
// TODO split into multiple slow5 files from same rg
int8_t slow5_split(struct slow5_file *s5p_from, const char *dirname_to);

// Close a slow5 file
int slow5_close(struct slow5_file *s5p);
void slow5_free(struct slow5_file *s5p);



// Get a read entry
struct Slow5Rec *slow5_get(const char *read_id, const struct slow5_file *s5p);
struct Slow5Rec **slow5_get_multi(const char **read_id, const uint64_t num_reads, const struct slow5_file *s5p);

// Print read entry
int slow5_rec_fprint(FILE *fp, struct Slow5Rec *read);
static inline int slow5_rec_print(struct Slow5Rec *read) { slow5_rec_fprint(stdout, read); }

// Free a read entry
void slow5_rec_free(struct Slow5Rec *read);



// Get a header value
const char *slow5_hdr_get(const char *attr, const struct slow5_file *s5p);

#endif
