#ifndef SLOW5_DEFS_H
#define SLOW5_DEFS_H

#include <stdint.h>
#include <inttypes.h>

// SLOW5 format specs
#define SLOW5_HEADER_PREFIX             "#"
#define SLOW5_HEADER_DATA_PREFIX        "@"
#define SLOW5_HEADER_DATA_PREFIX_CHAR   '@'
#define COLUMN_HEADER_PREFIX            "#"
#define SEP                             "\t"
#define SEP_CHAR                        '\t'
#define SEP_RAW_SIGNAL                  ","
#define HEADER_FILE_VERSION             "slow5_version"
#define HEADER_NUM_GROUPS               "num_read_groups"
#define HEADER_NUM_GROUPS_INIT          (1)

// Order, format string and type of main SLOW5 columns
// NOTE if this is changed, also edit:
//      slow5.c:slow5_rec_to_str FORMAT_BINARY
//      slow5idx_clean.c
#define SLOW5_COLS(col, end)                    \
    col(char*,      "%s",       read_id)        /* A malloced string */ \
    col(uint32_t,   "%" PRIu32, read_group)     \
    col(double,     "%s",       digitisation)   \
    col(double,     "%s",       offset)         \
    col(double,     "%s",       range)          \
    col(double,     "%s",       sampling_rate)  \
    col(uint64_t,   "%" PRIu64, len_raw_signal) \
    end(int16_t*,   ,           raw_signal) // Use end() for last column
// Format string of raw signal
#define FORMAT_STRING_RAW_SIGNAL "%" PRId16

// Apply the same function to each column including the last one
#define SLOW5_COLS_FOREACH(foo) SLOW5_COLS(foo, foo)

#define GENERATE_STRUCT(type, fmt, name)            type name;
#define GENERATE_ENUM(type, fmt, name)              COL_ ## name,
#define GENERATE_NAME_STRING(type, fmt, name)       #name
#define GENERATE_NAME_STRING_SEP(type, fmt, name)   GENERATE_NAME_STRING(type, fmt, name) SEP
#define GENERATE_TYPE_STRING(type, fmt, name)       #type
#define GENERATE_TYPE_STRING_SEP(type, fmt, name)   GENERATE_TYPE_STRING(type, fmt, name) SEP
#define GENERATE_FORMAT_STRING(type, fmt, name)     fmt
#define GENERATE_FORMAT_STRING_SEP(type, fmt, name) GENERATE_FORMAT_STRING(type, fmt, name) SEP
#define GENERATE_NULL(type, fmt, name)

// More SLOW5 specs
#define SLOW5_HEADER_ID(header_name)    SLOW5_HEADER_PREFIX header_name
#define HEADER_FILE_VERSION_ID          SLOW5_HEADER_ID(HEADER_FILE_VERSION)
#define HEADER_NUM_GROUPS_ID            SLOW5_HEADER_ID(HEADER_NUM_GROUPS)

#define SLOW5_HEADER_ENTRY(header_name, data) SLOW5_HEADER_ID(header_name) SEP data "\n"

// ASCII SLOW5 specs
#define ASCII_NAME                      "slow5"
#define ASCII_EXTENSION                 "." ASCII_NAME
#define ASCII_VERSION                   "0.1.0"
#define ASCII_VERSION_FORMAT            "%" PRIu8 ".%" PRIu8 ".%" PRIu8
#define ASCII_NUM_GROUPS_FORMAT         "%" PRIu32
#define ASCII_ENTRY_VERSION             SLOW5_HEADER_ENTRY(HEADER_FILE_VERSION, ASCII_VERSION)
#define ASCII_ENTRY_VERSION_FORMAT      SLOW5_HEADER_ENTRY(HEADER_FILE_VERSION, ASCII_VERSION_FORMAT)
#define ASCII_ENTRY_NUM_GROUPS_FORMAT   SLOW5_HEADER_ENTRY(HEADER_NUM_GROUPS, ASCII_NUM_GROUPS_FORMAT)
#define ASCII_SLOW5_HEADER_FORMAT       ASCII_ENTRY_VERSION_FORMAT ASCII_ENTRY_NUM_GROUPS_FORMAT
#define ASCII_TYPE_HEADER_MIN           COLUMN_HEADER_PREFIX SLOW5_COLS(GENERATE_TYPE_STRING_SEP, GENERATE_TYPE_STRING)
#define ASCII_COLUMN_HEADER_MIN         COLUMN_HEADER_PREFIX SLOW5_COLS(GENERATE_NAME_STRING_SEP, GENERATE_NAME_STRING)

// Binary SLOW5 specs
#define BINARY_NAME                     "blow5"
#define BINARY_EXTENSION                "." BINARY_NAME
#define BINARY_VERSION                  { 0, 1, 0 }
#define BINARY_MAGIC_NUMBER             { 'B', 'L', 'O', 'W', '5', '\1' }
#define BINARY_EOF                      { '5', 'W', 'O', 'L', 'B' }
#define BINARY_HEADER_SIZE_OFFSET       (64L)

// SLOW5 Index specs
//#define SLOW5_INDEX_HEADER_PREFIX   "#"
//#define SLOW5_INDEX_HEADER          SLOW5_INDEX_HEADER_PREFIX "read_id" SEP "offset" SEP "length\n"

#endif
