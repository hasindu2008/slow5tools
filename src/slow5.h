// Header with slow5 file definitions

#ifndef SLOW5_H
#define SLOW5_H

#include <stdio.h>
#include <stdint.h>
#include "klib/khash.h"

// Order and type of main SLOW5 columns
#define FOREACH_(FN, END) \
    FN(const char *,    read_id) \
    FN(uint32_t,        read_group) \
    FN(float,           digitisation) \
    FN(double,          offset) \
    FN(double,          range) \
    FN(double,          sampling_rate) \
    FN(uint64_t,        len_raw_signal) \
    END(int16_t *,      raw_signal) // Use END for last column

#define FOREACH(FN) FOREACH_(FN, FN)

#define STRUCTIFY(TYPE, NAME) TYPE NAME;
#define ENUMIFY(TYPE, NAME) COL_ ## NAME,
#define STRINGIFY(TYPE, NAME) #NAME
#define STRINGIFY_TAB(TYPE, NAME) STRINGIFY(,NAME) "\t"

// File formats to be dealing with
enum SLOW5Format {
    FORMAT_NONE,
    FORMAT_ASCII,
    FORMAT_BINARY
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
	struct SLOW5Version version; // TODO ptr or not
    uint32_t num_read_groups;
    khash_t(s2s) **data; // length = num_read_groups TODO kvec_t/linked list of this?
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
    FOREACH(GENERATE_STRUCT)
    struct SLOW5ReadAux *read_aux;
};

// SLOW5 main record columns
enum SLOW5Cols = {
    FOREACH_SAME(GENERATE_ENUM)
}

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


#define SLOW5_HEADER_PREFIX "#"
#define SLOW5_HEADER_DATA_PREFIX "@"
#define COLUMN_HEADER_PREFIX "#"
#define FILE_FORMAT_HEADER "file_format"
#define FILE_VERSION_HEADER "file_version"
#define NUM_GROUPS_HEADER "num_read_groups"
#define NUM_GROUPS_INIT (1)

#define SLOW5_HEADER_ID(header_name) SLOW5_HEADER_PREFIX header_name
#define FILE_FORMAT_HEADER_ID SLOW5_HEADER_ID(FILE_FORMAT_HEADER)
#define FILE_VERSION_HEADER_ID SLOW5_HEADER_ID(FILE_VERSION_HEADER)
#define NUM_GROUPS_HEADER_ID SLOW5_HEADER_ID(NUM_GROUPS_HEADER)

#define ASCII_VERSION "0.1.0"
#define ASCII_NAME "slow5"
#define ASCII_EXTENSION "." ASCII_NAME
#define ASCII_FILE_FORMAT FILE_FORMAT_HEADER_ID "\t" ASCII_NAME "\n"
#define ASCII_FILE_VERSION FILE_VERSION_HEADER_ID "\t" ASCII_VERSION "\n"
#define ASCII_NUM_GROUPS NUM_GROUPS_HEADER_ID "\t" "%d\n"
#define ASCII_SLOW5_HEADER ASCII_FILE_FORMAT ASCII_FILE_VERSION ASCII_NUM_GROUPS
#define ASCII_COLUMN_HEADER_MIN \
    COLUMN_HEADER_PREFIX \
    FOREACH(GENERATE_STRING_TAB)
    "read_id\t" \
    "read_group\t" \
    "digitisation\t" \
    "offset\t" \
    "range\t" \
    "sampling_rate\t" \
    "len_raw_signal\t" \
    "raw_signal"
#define ASCII_COLUMN_HEADER_FULL \
    ASCII_COLUMN_HEADER_MIN "\t" \
    "channel_number\t" \
    "median_before\t" \
    "read_number\t" \
    "start_mux\t" \
    "start_time\n"

#define BINARY_VERSION "0.1.0"
#define BINARY_NAME "blow5"
#define BINARY_EXTENSION "." BINARY_NAME
#define BINARY_FILE_FORMAT FILE_FORMAT_HEADER_ID "\t" BINARY_NAME "\n"
#define BINARY_FILE_VERSION FILE_VERSION_HEADER_ID "\t" BINARY_VERSION "\n"
#define BINARY_SLOW5_HEADER BINARY_FILE_FORMAT BINARY_FILE_VERSION ASCII_NUM_GROUPS


struct SLOW5FormatMap {
    const char *name;
    enum SLOW5Format format;
};

static const struct SLOW5FormatMap SLOW5_FORMAT_MAP[] = {
    { ASCII_NAME,   FORMAT_ASCII    },
    { BINARY_NAME,  FORMAT_BINARY   }
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
