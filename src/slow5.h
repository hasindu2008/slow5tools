// Header with slow5 file definitions

#ifndef SLOW5_H
#define SLOW5_H

#include <stdio.h>
#include <stdint.h>
#include "klib/khash.h"

// SLOW5 format specs
#define SLOW5_HEADER_PREFIX         "#"
#define SLOW5_HEADER_DATA_PREFIX    "@"
#define COLUMN_HEADER_PREFIX        "#"
#define SEPARATOR                   "\t"
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
#define GENERATE_STRING_SEP(type, name) GENERATE_STRING(type, name) SEPARATOR

// More SLOW5 specs
#define SLOW5_HEADER_ID(header_name)    SLOW5_HEADER_PREFIX header_name
#define HEADER_FILE_FORMAT_ID           SLOW5_HEADER_ID(HEADER_FILE_FORMAT)
#define HEADER_FILE_VERSION_ID          SLOW5_HEADER_ID(HEADER_FILE_VERSION)
#define HEADER_NUM_GROUPS_ID            SLOW5_HEADER_ID(HEADER_NUM_GROUPS)

#define SLOW5_HEADER_ENTRY(header_name, data) SLOW5_HEADER_ID(header_name) SEPARATOR data "\n"

// ASCII SLOW5 specs
#define ASCII_VERSION           "0.1.0"
#define ASCII_NAME              "slow5"
#define ASCII_EXTENSION         "." ASCII_NAME
#define ASCII_FILE_FORMAT       SLOW5_HEADER_ENTRY(HEADER_FILE_FORMAT, ASCII_NAME)
#define ASCII_FILE_VERSION      SLOW5_HEADER_ENTRY(HEADER_FILE_VERSION, ASCII_VERSION)
#define ASCII_NUM_GROUPS        SLOW5_HEADER_ENTRY(NUM_GROUPS_HEADER, "%d")
#define ASCII_SLOW5_HEADER      ASCII_FILE_FORMAT ASCII_FILE_VERSION ASCII_NUM_GROUPS
#define ASCII_COLUMN_HEADER_MIN COLUMN_HEADER_PREFIX SLOW5_COLS(GENERATE_STRING_SEP, GENERATE_STRING)
#define ASCII_COLUMN_HEADER_FULL \
    ASCII_COLUMN_HEADER_MIN SEPARATOR \
    "channel_number" SEPARATOR \
    "median_before" SEPARATOR \
    "read_number" SEPARATOR \
    "start_mux" SEPARATOR \
    "start_time\n"

#define BINARY_VERSION "0.1.0"
#define BINARY_NAME "blow5"
#define BINARY_EXTENSION "." BINARY_NAME
#define BINARY_FILE_FORMAT FILE_FORMAT_HEADER_ID SEPARATOR BINARY_NAME "\n"
#define BINARY_FILE_VERSION FILE_VERSION_HEADER_ID SEPARATOR BINARY_VERSION "\n"
#define BINARY_SLOW5_HEADER BINARY_FILE_FORMAT BINARY_FILE_VERSION ASCII_NUM_GROUPS

// SLOW5 main record columns
enum SLOW5Cols {
    SLOW5_COLS_FOREACH(GENERATE_ENUM)
};

// File formats to be dealing with
enum SLOW5Format {
    FORMAT_NONE,
    FORMAT_ASCII,
    FORMAT_BINARY
};

struct SLOW5FormatMap {
    const char *name;
    enum SLOW5Format format;
};

static const struct SLOW5FormatMap SLOW5_FORMAT_MAP[] = {
    { ASCII_NAME,   FORMAT_ASCII    },
    { BINARY_NAME,  FORMAT_BINARY   }
};

// Header data map: attribute string -> data string
KHASH_MAP_INIT_STR(s2s, const char *)
// Read id map: read id -> read record
KHASH_MAP_INIT_STR(s2r, struct SLOW5Read *)

// SLOW5 versioning
struct SLOW5Version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

// SLOW5 header
struct SLOW5Header {
    enum SLOW5Format format;
    char *version_str;
	struct SLOW5Version version;
    uint32_t num_read_groups;
    khash_t(s2s) **data; // length = num_read_groups
};

// SLOW5 auxillary record data
// TODO make dynamic
struct SLOW5ReadAux {
	char *channel_number;
    char *end_reason;
    double median_before;
	int32_t read_number;
	uint64_t start_time;
    uint8_t start_mux;
};

// SLOW5 record data
struct SLOW5Read {
    SLOW5_COLS_FOREACH(GENERATE_STRUCT)
    struct SLOW5ReadAux *read_aux;
};

// SLOW5 object
struct SLOW5 {
    struct SLOW5Header *header;
    // TODO do we want the reads to be in order of addition?
    khash_t(s2r) *reads;
};

// SLOW5 file object
/*
struct SLOW5File {
    FILE *stream;
    struct SLOW5 *slow5;
};
*/

// SLOW5 writing configuration
struct SLOW5WriteConf {
    enum SLOW5Format format;
    struct Press *compress;
};

/*
struct SLOW5VersionMap {
    const char *version_str;
    const uint8_t version[3];
};

static const struct SLOW5VersionMap SLOW5_VERSION_MAP[] = {
    { "0.1.0",  {0, 1, 0}   }
};
*/

// API

// Initiate an empty slow5 object
struct SLOW5 *slow5_init_empty(void);
// Initiate a slow5 object from a slow5 file
struct SLOW5 *slow5_init(enum SLOW5Format format, FILE *stream);
// Destroy a slow5 object
void slow5_destroy(struct SLOW5 **slow5);

// Read from a file into a slow5 object
void slow5_read(struct SLOW5 *slow5, enum SLOW5Format format, FILE *stream);

// Write to a file from a slow5 object
void slow5_write(struct SLOW5 *slow5, struct SLOW5WriteConf config, FILE *stream);
// Write a slow5 file header
uint8_t slow5_write_hdr(struct SLOW5 *slow5, struct SLOW5WriteConf config, FILE *stream);
// Write header data
uint8_t slow5_write_hdr_data(struct SLOW5 *slow5, struct SLOW5WriteConf config, FILE *stream);
// Write header attribute
uint8_t slow5_write_hdr_data_attr(struct SLOW5 *slow5, struct SLOW5WriteConf config, const char *attr, FILE *stream);

// Print out the SLOW5 structure contents
void slow5_print_hdr(const struct SLOW5Header *hdr);

// Get the format from a slow5 format name
enum SLOW5Format str_get_slow5_format(const char *str);
// Get the format of a slow5 pathname
enum SLOW5Format path_get_slow5_format(const char *pathname);
// Get the format of a slow5 FILE
enum SLOW5Format stream_get_slow5_format(const FILE *stream);

// Get the slow5 format name from the format
const char *slow5_format_get_str(enum SLOW5Format format);

// Get the slow5 version array from a version string
//const uint8_t *str_get_slow5_version(const char *str);

// Atoi but to uintx_t
// and without any symbols
// and without 0 prefixing
uint8_t ato_uint8(const char *str);
uint32_t ato_uint32(const char *str);

// Open a slow5 file
/*
struct SLOW5File *slow5_fopen(const char *pathname, const char *mode);
struct SLOW5File *slow5_fopen_format(enum SLOW5Format format, const char *pathname, const char *mode);
*/

/*
// Initiate and destroy a slow5 file
uint8_t fslow5_init(struct SLOW5 *slow5, FILE *stream);
*/

#endif
