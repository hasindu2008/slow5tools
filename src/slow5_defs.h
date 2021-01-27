#ifndef SLOW5_DEFS_H
#define SLOW5_DEFS_H

// SLOW5 format specs
#define SLOW5_HEADER_PREFIX         "#"
#define SLOW5_HEADER_DATA_PREFIX    "@"
#define COLUMN_HEADER_PREFIX        "#"
#define SEP                         "\t"
#define SEP_RAW_SIGNAL              ","
#define HEADER_FILE_FORMAT          "file_format"
#define HEADER_FILE_VERSION         "file_version"
#define HEADER_NUM_GROUPS           "num_read_groups"
#define HEADER_NUM_GROUPS_INIT      (1)

// Order and type of main SLOW5 columns
#define SLOW5_COLS(col, end) \
    col(char *,         read_id) \
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

// SLOW5 Index specs
#define SLOW5_INDEX_HEADER_PREFIX   "#"
#define SLOW5_INDEX_HEADER          SLOW5_INDEX_HEADER_PREFIX "read_id" SEP "offset" SEP "length\n"

#endif
