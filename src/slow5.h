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
KHASH_MAP_INIT_STR(s2s, char *)

// SLOW5 header
struct slow5_hdr {
	struct slow5_version version;
    uint32_t num_read_groups; // Number of read groups
    uint32_t num_attrs; // Number of header attributes TODO type ok maybe smaller?
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
    SLOW5_COLS_FOREACH(GENERATE_STRUCT)
    struct slow5_rec_aux *read_aux;
};

/* SLOW5 file */

// SLOW5 file meta
struct slow5_file_meta {
    const char *pathname;
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

/**
 * Open a slow5 file with a specific mode given it's pathname.
 *
 * Attempt to guess the file's slow5 format from the pathname's extension.
 * Return NULL if pathname or mode is NULL,
 * or if the pathname's extension is not recognised,
 * of if the pathname is invalid.
 *
 * Otherwise, return a slow5 file structure with the header parsed.
 * slow5_close() should be called when finished with the structure.
 *
 * @param   pathname    relative or absolute path to slow5 file
 * @param   mode        same mode as in fopen()
 * @return              slow5 file structure
 */
struct slow5_file *slow5_open(const char *pathname, const char *mode);

/**
 * Open a slow5 file of a specific format with a mode given it's pathname.
 *
 * Return NULL if pathname or mode is NULL, or if the format specified doesn't match the file.
 * slow5_open_with(pathname, mode, FORMAT_UNKNOWN) is equivalent to slow5_open(pathname, mode).
 *
 * Otherwise, return a slow5 file structure with the header parsed.
 * slow5_close() should be called when finished with the structure.
 *
 * @param   pathname    relative or absolute path to slow5 file
 * @param   mode        same mode as in fopen()
 * @param   format      format of the slow5 file
 * @return              slow5 file structure
 */
struct slow5_file *slow5_open_with(const char *pathname, const char *mode, enum slow5_fmt format);

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

/**
 * Close a slow5 file and free its memory.
 *
 * @param   s5p slow5 file structure
 * @return      same as fclose()
 */
int slow5_close(struct slow5_file *s5p);



/**
 * Get a read entry from a slow5 file corresponding to a read_id.
 *
 * Allocates memory for *read if it is NULL.
 * Otherwise, the data in *read is freed and overwritten.
 * slow5_rec_free() should be called when finished with the structure.
 *
 * Return
 * TODO are these error codes too much?
 *  0   the read was successfully found and stored
 * -1   read_id, read or s5p is NULL
 * -2   the index was not previously init and failed to init
 * -3   read_id was not found in the index
 * -4   reading error when reading the slow5 file
 * -5   parsing error
 *
 * @param   read_id the read identifier
 * @param   read    address of a slow5_rec pointer
 * @param   s5p     slow5 file
 * @return  error code described above
 */
int slow5_get(const char *read_id, struct slow5_rec **read, struct slow5_file *s5p);

/**
 * Get the read entry under the current file pointer of a slow5 file.
 *
 * Allocates memory for *read if it is NULL.
 * Otherwise, the data in *read is freed and overwritten.
 * slow5_rec_free() should be called when finished with the structure.
 *
 * Return
 * TODO are these error codes too much?
 *  0   the read was successfully found and stored
 * -1   read_id, read or s5p is NULL
 * -2   reading error when reading the slow5 file
 * -3   parsing error
 *
 * @param   read    address of a slow5_rec pointer
 * @param   s5p     slow5 file
 * @return  error code described above
 */
int slow5_get_next(struct slow5_rec **read, struct slow5_file *s5p);

/**
 * Get the read entry as a string in the specified format.
 *
 * Returns NULL if read is NULL, or format is FORMAT_UNKNOWN,
 * or the read attribute values are invalid
 *
 * @param   read    slow5_rec pointer
 * @param   format  slow5 format to write the entry in
 * @return  malloced string to use free() on, NULL on error
 */
char *slow5_rec_to_str(struct slow5_rec *read, enum slow5_fmt format);

/**
 * Print a read entry in the specified format to a file pointer.
 *
 * On success, the number of bytes written is returned.
 * On error, -1 is returned.
 *
 * @param   fp      output file pointer
 * @param   read    slow5_rec pointer
 * @param   format  slow5 format to write entry in
 * @return  number of bytes written, -1 on error
 */
int slow5_rec_fprint(FILE *fp, struct slow5_rec *read, enum slow5_fmt format);
static inline int slow5_rec_print(struct slow5_rec *read, enum slow5_fmt format) {
    return slow5_rec_fprint(stdout, read, format);
}

// Free a read entry
void slow5_rec_free(struct slow5_rec *read);



/**
 * Get a header data attribute for a particular read_group.
 *
 * Returns NULL if the attribute name doesn't exist
 * or the read group is out of range
 * or an input parameter is NULL.
 *
 * @param   attr        attribute name
 * @param   read_group  the read group
 * @param   s5p         slow5 file
 * @return  the attribute's value, or NULL on error
 */
char *slow5_hdr_get(const char *attr, uint32_t read_group, const struct slow5_file *s5p);

/**
 * Add a new header data attribute.
 *
 * All values are set to NULL for each read group.
 *
 * Returns -1 if an input parameter is NULL.
 * Returns -2 if the attribute already exists.
 * Returns 0 other.
 *
 * @param   attr        attribute name
 * @param   s5p         slow5 file
 * @return  0 on success, <0 on error as described above
 */
int slow5_hdr_add(const char *attr, const struct slow5_file *s5p);

/**
 * Set a header data attribute for a particular read_group.
 *
 * Doesn't take memory ownership of the value given.
 *
 * Returns -1 if the attribute name doesn't exist
 * or the read group is out of range
 * or an input parameter is NULL.
 * Returns 0 other.
 *
 * @param   attr        attribute name
 * @param   value       new attribute value
 * @param   read_group  the read group
 * @param   s5p         slow5 file
 * @return  0 on success, -1 on error
 */
int slow5_hdr_set(const char *attr, const char *value, uint32_t read_group, const struct slow5_file *s5p);

/**
 * Get the header as a string in the specified format.
 *
 * Returns NULL if read is NULL
 * or format is FORMAT_UNKNOWN
 * or an internal error occurs.
 *
 * @param   s5p     slow5 file
 * @param   format  slow5 format to write the entry in
 * @return  malloced string to use free() on, NULL on error
 */
char *slow5_hdr_to_str(struct slow5_file *s5p, enum slow5_fmt format);

/**
 * Print the header in the specified format to a file pointer.
 *
 * On success, the number of bytes written is returned.
 * On error, -1 is returned.
 *
 * @param   fp      output file pointer
 * @param   s5p     slow5_rec pointer
 * @param   format  slow5 format to write the entry in
 * @return  number of bytes written, -1 on error
 */
int slow5_hdr_fprint(FILE *fp, struct slow5_file *s5p, enum slow5_fmt format);
static inline int slow5_hdr_print(struct slow5_file *s5p, enum slow5_fmt format) {
    return slow5_hdr_fprint(stdout, s5p, format);
}

#endif
