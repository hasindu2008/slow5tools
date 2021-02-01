#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h> // TODO use better error handling?
#include "slow5.h"
#include "slow5_extra.h"
#include "slow5idx_clean.h"
#include "press.h"
#include "error.h"
#include "misc.h"

// TODO fail with getline if end of file occurs on a non-empty line
// TODO (void) cast if ignoring return value

// String buffer capacity for parsing the data header
#define SLOW5_HEADER_DATA_BUF_INIT_CAP (1024) // 2^10 TODO is this too much? Or put to a page length

inline void slow5_hdr_free(struct slow5_hdr *header);

/* Definitions */

// slow5 file

struct slow5_file *slow5_init(FILE *fp, const char *pathname, enum slow5_fmt format) {
    // Pathname cannot be NULL at this point
    if (fp == NULL) {
        return NULL;
    }

    if (format == FORMAT_UNKNOWN) {

        // Attempt to determine format
        // from pathname
        if ((format = path_get_slow5_fmt(pathname)) == FORMAT_UNKNOWN) {
            fclose(fp);
            return NULL;
        }
    }

    struct slow5_file *s5p;
    struct slow5_hdr *header = slow5_hdr_init(fp, format);
    if (header == NULL) {
        s5p = NULL;
    } else {
        s5p = (struct slow5_file *) calloc(1, sizeof *s5p);
        MALLOC_CHK(s5p);

        s5p->fp = fp;
        s5p->format = format;
        s5p->header = header;

        assert((s5p->meta.fd = fileno(fp)) != -1);
        s5p->meta.pathname = pathname;
    }

    // TODO only use when reading?
    // TODO determine compression?
    //s5p->compress = press_init(compress);

    return s5p;
}

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
struct slow5_file *slow5_open(const char *pathname, const char *mode) {
    return slow5_open_with(pathname, mode, FORMAT_UNKNOWN);
}

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
struct slow5_file *slow5_open_with(const char *pathname, const char *mode, enum slow5_fmt format) {
    if (pathname == NULL || mode == NULL) {
        return NULL;
    } else {
        return slow5_init(fopen(pathname, mode), pathname, format);
    }
}

// Can be used with slow5_open or slow5_init_fp
int slow5_close(struct slow5_file *s5p) {
    int ret;

    if (s5p == NULL) {
        ret = EOF;
    } else {
        ret = fclose(s5p->fp);
        press_free(s5p->compress);
        slow5_hdr_free(s5p->header);
        if (s5p->index != NULL) {
            slow5_idx_free(s5p->index);
        }

        free(s5p);
    }

    return ret;
}


// slow5 header

struct slow5_hdr *slow5_hdr_init(FILE *fp, enum slow5_fmt format) {

    struct slow5_hdr *header = (struct slow5_hdr *) calloc(1, sizeof *(header));
    MALLOC_CHK(header);

    // Parse slow5 header
    switch (format) {

        case FORMAT_UNKNOWN:
            break;

        case FORMAT_ASCII: {

            // Buffer for file parsing
            size_t cap = SLOW5_HEADER_DATA_BUF_INIT_CAP;
            char *buf = (char *) malloc(cap * sizeof *buf);
            char *bufp;
            MALLOC_CHK(buf);
            ssize_t buf_len;
            int err;

            // 1st line
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later format parsing
            // "#file_format"
            bufp = buf;
            char *tok = strsep_mine(&bufp, SEP);
            assert(strcmp(tok, HEADER_FILE_FORMAT_ID) == 0);
            // Parse format name
            tok = strsep_mine(&bufp, SEP);
            assert(format == name_get_slow5_fmt(tok));

            // 2nd line
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later parsing
            // "#file_version"
            bufp = buf;
            tok = strsep_mine(&bufp, SEP);
            assert(strcmp(tok, HEADER_FILE_VERSION_ID) == 0);
            // Parse file version
            tok = strsep_mine(&bufp, SEP);
            header->version_str = strdup(tok);
            // Parse file version string
            // TODO necessary to parse it now?
            char *toksub;
            assert((toksub = strsep_mine(&tok, ".")) != NULL); // Major version
            header->version.major = ato_uint8(toksub, &err);
            assert(err != -1);
            assert((toksub = strsep_mine(&tok, ".")) != NULL); // Minor version
            header->version.minor = ato_uint8(toksub, &err);
            assert(err != -1);
            assert((toksub = strsep_mine(&tok, ".")) != NULL); // Patch version
            header->version.patch = ato_uint8(toksub, &err);
            assert(err != -1);
            assert(strsep_mine(&tok, ".") == NULL); // No more tokenators

            // 3rd line
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later parsing
            // "#num_read_groups"
            bufp = buf;
            tok = strsep_mine(&bufp, SEP);
            assert(strcmp(tok, HEADER_NUM_GROUPS_ID) == 0);
            // Parse num read groups
            tok = strsep_mine(&bufp, SEP);
            header->num_read_groups = ato_uint32(tok, &err);
            assert(err != -1);

            // Header data
            header->data = slow5_hdr_data_init(fp, format, buf, cap, header->num_read_groups);

            free(buf);

        } break;

        case FORMAT_BINARY: // TODO
            break;
    }

    return header;
}

void slow5_hdr_print(const struct slow5_hdr *header) {
    printf("format='TODO'\n"
            "version_str='%s'\n"
            "version={%hu,%hu,%hu}\n"
            "num_rgs=%u\n",
            //slow5_fmt_get_name(hdr->format),
            header->version_str,
            header->version.major,
            header->version.minor,
            header->version.patch,
            header->num_read_groups);
}

/**
 * Get a header data attribute.
 *
 * Returns NULL if the attribute name doesn't exist
 * or an input parameter is NULL.
 *
 * @param   attr    attribute name
 * @param   s5p     slow5 file
 * @return  the attribute's value, or NULL on error
 */
char *slow5_hdr_get(const char *attr, uint32_t read_group, const struct slow5_file *s5p) {
    char *value;

    if (attr == NULL || s5p == NULL || read_group >= s5p->header->num_read_groups) {
        return NULL;
    }

    khash_t(s2s) *hdr_data = s5p->header->data[read_group];

    khint_t pos = kh_get(s2s, hdr_data, attr);
    if (pos == kh_end(hdr_data)) {
        return NULL;
    } else {
        value = kh_value(hdr_data, pos);
    }

    return value;
}

/**
 * Set a header data attribute for a particular read_group.
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
int slow5_hdr_set(const char *attr, const char *value, uint32_t read_group, const struct slow5_file *s5p) {

    if (attr == NULL || value == NULL || s5p == NULL || read_group >= s5p->header->num_read_groups) {
        return -1;
    }

    khash_t(s2s) *hdr_data = s5p->header->data[read_group];

    khint_t pos = kh_get(s2s, hdr_data, attr);
    if (pos == kh_end(hdr_data)) {
        return -1;
    } else {
        free(kh_value(hdr_data, pos));
        char *value_cp = strdup(value);
        MALLOC_CHK(value_cp);
        kh_value(hdr_data, pos) = value_cp;
    }

    return 0;
}

void slow5_hdr_free(struct slow5_hdr *header) {
    NULL_CHK(header); //TODO needed or not?

    // Free version string
    free(header->version_str);

    slow5_hdr_data_free(header->data, header->num_read_groups);

    free(header);
}


// slow5 header data

khash_t(s2s) **slow5_hdr_data_init(FILE *fp, enum slow5_fmt format, char *buf, size_t cap, uint32_t num_rgs) {

    khash_t(s2s) **hdr_data = (khash_t(s2s) **) malloc(num_rgs * sizeof *hdr_data);
    MALLOC_CHK(hdr_data);

    // TODO check if cast necessary
    for (uint64_t i = 0; i < (uint64_t) num_rgs; ++ i) {
        hdr_data[i] = kh_init(s2s);
        NULL_CHK(hdr_data[i]);
    }


    // Parse slow5 header data
    switch (format) {

        case FORMAT_UNKNOWN:
            break;

        case FORMAT_ASCII: {

            ssize_t buf_len;

            // Get first line of header data
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later parsing

            // While the column header hasn't been reached
            while (strncmp(buf, ASCII_COLUMN_HEADER_MIN, strlen(ASCII_COLUMN_HEADER_MIN)) != 0) {

                // Ensure prefix is there
                assert(strncmp(buf, SLOW5_HEADER_DATA_PREFIX, strlen(SLOW5_HEADER_DATA_PREFIX)) == 0);
                char *shift = buf + strlen(SLOW5_HEADER_DATA_PREFIX); // Remove prefix

                // Get the attribute name
                char *attr = strdup(strsep_mine(&shift, SEP));
                char *val;

                // Iterate through the values
                uint32_t i = 0;
                while ((val = strsep_mine(&shift, SEP)) != NULL && i <= num_rgs - 1) {

                    // Set key
                    int absent;
                    khint_t pos = kh_put(s2s, hdr_data[i], attr, &absent);
                    assert(absent != -1);

                    // Set value
                    kh_val(hdr_data[i], pos) = strdup(val);

                    ++ i;
                }
                // Ensure that read group number of entries are read
                assert(i == num_rgs);

                // Get next line
                assert((buf_len = getline(&buf, &cap, fp)) != -1);
                buf[buf_len - 1] = '\0'; // Remove newline for later parsing
            }

            assert(cap == SLOW5_HEADER_DATA_BUF_INIT_CAP); // TESTING to see if getline has to realloc (if this fails often maybe put a larger buffer size)

        } break;

        case FORMAT_BINARY: // TODO
           break;
    }

    return hdr_data;
}

void slow5_hdr_data_free(khash_t(s2s) **hdr_data, uint32_t num_rgs) {

    // Free header data map
    for (uint64_t i = 0; i < (uint64_t) num_rgs; ++ i) {

        for (khint_t j = kh_begin(hdr_data[i]); j < kh_end(hdr_data[i]); ++ j) {
            if (kh_exist(hdr_data[i], j)) {
                if (i == 0) {
                    free((void *) kh_key(hdr_data[i], j)); // TODO avoid void *
                }
                free((void *) kh_val(hdr_data[i], j)); // TODO avoid void *
            }
        }
        kh_destroy(s2s, hdr_data[i]);
    }

    free(hdr_data);
}


// slow5 record

int slow5_get(const char *read_id, struct slow5_rec **read, struct slow5_file *s5p) {
    if (read_id == NULL || read == NULL || s5p == NULL) {
        return -1;
    }

    int ret = 0;
    char *read_str;

    // Create index if NULL
    if (s5p->index == NULL) {
        // Get index pathname
        char *index_pathname = get_slow5_idx_path(s5p->meta.pathname);
        s5p->index = slow5_idx_init(s5p, index_pathname);
        free(index_pathname); // TODO save in struct?
        if (s5p->index == NULL) {
            // index failed to init
            return -2;
        }
    }

    // Get index record
    struct slow5_rec_idx read_index;
    if (slow5_idx_get(s5p->index, read_id, &read_index) == -1) {
        // read_id not found in index
        return -3;
    }

    // Malloc string to hold the read
    ssize_t read_len = (read_index.size + 1) * sizeof *read_str; // + 1 for '\0'
    read_str = (char *) malloc(read_len);
    MALLOC_CHK(read_str);

    // Read into the string
    if (pread(s5p->meta.fd, read_str, read_len - 1, read_index.offset) != read_len - 1) {
        free(read_str);
        // reading error
        return -4;
    }
    read_str[read_len - 1] = '\0';

    if (*read == NULL) {
        // Allocate memory for read
        *read = (struct slow5_rec *) calloc(1, sizeof **read);
        MALLOC_CHK(*read);
    } else {
        // Free previously allocated strings
        free((*read)->str); // TODO use a string buffer
        free((*read)->read_id);
    }

    (*read)->str = strdup(read_str);

    read_str[read_index.size - 1] = '\0'; // Remove newline for later parsing
    if (slow5_rec_parse(read_str, read_id, *read, s5p->format) == -1) {
        ret = -5;
    }
    free(read_str);

    return ret;
}

// Return -1 on failure to parse
int slow5_rec_parse(char *read_str, const char *read_id, struct slow5_rec *read, enum slow5_fmt format) {
    int ret = 0;
    uint64_t prev_len_raw_signal = 0;

    switch (format) {

        case FORMAT_UNKNOWN:
            break;

        case FORMAT_ASCII: {

            char *tok;
            if ((tok = strsep_mine(&read_str, SEP)) == NULL) {
                return -1;
            }

            int i = 0;
            bool main_cols_parsed = false;
            int err;
            do {
                switch (i) {
                    case COL_read_id:
                        // Ensure line matches requested id
                        if (read_id != NULL) {
                            if (strcmp(tok, read_id) != 0) {
                                ret = -1;
                                break;
                            }
                        }
                        read->read_id = strdup(tok);
                        break;

                    case COL_read_group:
                        read->read_group = ato_uint32(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_digitisation:
                        read->digitisation = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_offset:
                        read->offset = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_range:
                        read->range = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_sampling_rate:
                        read->sampling_rate = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_len_raw_signal:
                        if (read->len_raw_signal != 0) {
                            prev_len_raw_signal = read->len_raw_signal;
                        }
                        read->len_raw_signal = ato_uint64(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_raw_signal: {
                        if (read->raw_signal == NULL) {
                            read->raw_signal = (int16_t *) malloc(read->len_raw_signal * sizeof *(read->raw_signal));
                            MALLOC_CHK(read->raw_signal);
                        } else if (prev_len_raw_signal < read->len_raw_signal) {
                            read->raw_signal = (int16_t *) realloc(read->raw_signal, read->len_raw_signal * sizeof *(read->raw_signal));
                            MALLOC_CHK(read->raw_signal);
                        }

                        char *signal_tok;
                        if ((signal_tok = strsep_mine(&tok, SEP_RAW_SIGNAL)) == NULL) {
                            // 0 signals
                            ret = -1;
                            break;
                        }

                        uint64_t j = 0;

                        // Parse raw signal
                        do {
                            (read->raw_signal)[j] = ato_int16(signal_tok, &err);
                            if (err == -1) {
                                ret = -1;
                                break;
                            }
                            ++ j;
                        } while ((signal_tok = strsep_mine(&tok, SEP_RAW_SIGNAL)) != NULL);
                        if (ret != -1 && j != read->len_raw_signal) {
                            ret = -1;
                        }

                    } break;

                    // All columns parsed
                    default:
                        main_cols_parsed = true;
                        break;

                }
                ++ i;

            } while (ret != -1 &&
                    !main_cols_parsed &&
                    (tok = strsep_mine(&read_str, SEP)) != NULL);

            // All columns parsed
            if (i == SLOW5_COLS_NUM) {

            // Remaining columns to parse
            } else if (i == SLOW5_COLS_NUM + 1) {

            // Not all main columns parsed
            } else {
                ret = -1;
            }

        } break;

        case FORMAT_BINARY: // TODO
            break;
    }

    return ret;
}

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
int slow5_get_next(struct slow5_rec **read, struct slow5_file *s5p) {
    if (read == NULL || s5p == NULL) {
        return -1;
    }

    int ret = 0;
    char *read_str = NULL;

    size_t cap = 0;
    ssize_t read_len;
    if ((read_len = getline(&read_str, &cap, s5p->fp)) == -1) {
        free(read_str);
        return -2;
    }

    if (*read == NULL) {
        // Allocate memory for read
        *read = (struct slow5_rec *) calloc(1, sizeof **read);
    } else {
        // Free previously allocated strings
        free((*read)->str);
        free((*read)->read_id);
    }
    (*read)->str = strdup(read_str);

    read_str[read_len - 1] = '\0'; // Remove newline for later parsing

    if (slow5_rec_parse(read_str, NULL, *read, s5p->format) == -1) {
        ret = -3;
    }
    free(read_str);

    return ret;
}

/**
 * Print a read entry in the correct format to a file pointer.
 *
 * On success, the number of bytes written is returned.
 * On error, -1 is returned.
 *
 * @param   fp      output file pointer
 * @param   read    slow5_rec pointer
 * @return  number of bytes written, -1 on error
 */
int slow5_rec_fprint(FILE *fp, struct slow5_rec *read, enum slow5_fmt format) {
    char *read_str;

    if (fp == NULL || read == NULL || (read_str = slow5_rec_to_str(read, format)) == NULL) {
        return -1;
    }

    return fprintf(fp, "%s", read_str);
}

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
/*
char *slow5_rec_to_str(struct slow5_rec *read, enum slow5_fmt format) {
    if (read == NULL || format == FORMAT_UNKNOWN) {
        return NULL;
    }
    char *str;
    // TODO asprintf and realloc after for signal array
    // Then handle auxillary columns
    asprintf_mine(&str, "%s%");
    MALLOC_CHK(str);

    switch (format) {

        case FORMAT_UNKNOWN:
            break;

        case FORMAT_ASCII: {

            int i = 0;
            int err;
            do {
                switch (i) {
                    case COL_read_id:
                        if (read->read_id != NULL) {
                        }
                        read->read_id = strdup(tok);
                        break;

                    case COL_read_group:
                        read->read_group = ato_uint32(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_digitisation:
                        read->digitisation = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_offset:
                        read->offset = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_range:
                        read->range = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_sampling_rate:
                        read->sampling_rate = strtod_check(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_len_raw_signal:
                        if (read->len_raw_signal != 0) {
                            prev_len_raw_signal = read->len_raw_signal;
                        }
                        read->len_raw_signal = ato_uint64(tok, &err);
                        if (err == -1) {
                            ret = -1;
                        }
                        break;

                    case COL_raw_signal: {
                        if (read->raw_signal == NULL) {
                            read->raw_signal = (int16_t *) malloc(read->len_raw_signal * sizeof *(read->raw_signal));
                            MALLOC_CHK(read->raw_signal);
                        } else if (prev_len_raw_signal < read->len_raw_signal) {
                            read->raw_signal = (int16_t *) realloc(read->raw_signal, read->len_raw_signal * sizeof *(read->raw_signal));
                            MALLOC_CHK(read->raw_signal);
                        }

                        char *signal_tok;
                        if ((signal_tok = strsep_mine(&tok, SEP_RAW_SIGNAL)) == NULL) {
                            // 0 signals
                            ret = -1;
                            break;
                        }

                        uint64_t j = 0;

                        // Parse raw signal
                        do {
                            (read->raw_signal)[j] = ato_int16(signal_tok, &err);
                            if (err == -1) {
                                ret = -1;
                                break;
                            }
                            ++ j;
                        } while ((signal_tok = strsep_mine(&tok, SEP_RAW_SIGNAL)) != NULL);
                        if (ret != -1 && j != read->len_raw_signal) {
                            ret = -1;
                        }

                    } break;

                    // All columns parsed
                    default:
                        main_cols_parsed = true;
                        break;

                }
                ++ i;

            } while (ret != -1 &&
                    !main_cols_parsed &&
                    (tok = strsep_mine(&read_str, SEP)) != NULL);

            // All columns parsed
            if (i == SLOW5_COLS_NUM) {

            // Remaining columns to parse
            } else if (i == SLOW5_COLS_NUM + 1) {

            // Not all main columns parsed
            } else {
                ret = -1;
            }

        } break;

        case FORMAT_BINARY: // TODO
            break;
    }
}
*/

void slow5_rec_free(struct slow5_rec *read) {
    if (read != NULL) {
        free(read->str);
        free(read->read_id);
        free(read->raw_signal);
        free(read);
    }
}



// slow5 extension parsing

enum slow5_fmt name_get_slow5_fmt(const char *name) {
    enum slow5_fmt format = FORMAT_UNKNOWN;

    if (name != NULL) {
        for (size_t i = 0; i < sizeof SLOW5_FORMAT_META / sizeof SLOW5_FORMAT_META[0]; ++ i) {
            const struct slow5_fmt_meta meta = SLOW5_FORMAT_META[i];
            if (strcmp(meta.name, name) == 0) {
                format = meta.format;
                break;
            }
        }
    }

    return format;
}

enum slow5_fmt path_get_slow5_fmt(const char *path) {
    enum slow5_fmt format = FORMAT_UNKNOWN;

    // TODO change type from size_t
    if (path != NULL) {
        size_t i;
        for (i = strlen(path) - 1; i >= 0; -- i) {
            if (path[i] == '.') {
                const char *ext = path + i + 1;
                format = name_get_slow5_fmt(ext);
                break;
            }
        }
    }

    return format;
}

// Get the slow5 format name from the format
const char *slow5_fmt_get_name(enum slow5_fmt format) {
    const char *str = NULL;

    for (size_t i = 0; i < sizeof SLOW5_FORMAT_META / sizeof SLOW5_FORMAT_META[0]; ++ i) {
        const struct slow5_fmt_meta meta = SLOW5_FORMAT_META[i];
        if (meta.format == format) {
            str = meta.name;
            break;
        }
    }

    return str;
}

char *get_slow5_idx_path(const char *path) {
    size_t new_len = strlen(path) + strlen(INDEX_EXTENSION);
    char *str = (char *) malloc((new_len + 1) * sizeof *str); // +1 for '\0'
    MALLOC_CHK(str);
    strncpy(str, path, strlen(path));
    strcpy(str + strlen(path), INDEX_EXTENSION);

    return str;
}

/*
struct slow5_file *slow5_open_format(enum slow5_fmt format, const char *pathname, const char *mode) {
    struct slow5_file *slow5 = NULL;

    if (pathname != NULL && mode != NULL) {
        FILE *fp = fopen(pathname, mode);

        // Try autodetect using extension if format is FORMAT_NONE
        if (format == FORMAT_NONE) {
            format = path_get_slow5_fmt(pathname);
        }

        slow5 = slow5_init(format, fp);
    }

    return slow5;
}

struct slow5_file *slow5_open(const char *pathname, const char *mode) {
    return slow5_open_format(FORMAT_NONE, pathname, mode);
}


// TODO Return type of fclose is int but we want to use a specific size int?
int slow5_close(struct slow5_file *slow5) {
    int ret = -1; // TODO choose return code for library

    if (slow5 != NULL) {
        ret = fclose(slow5->fp);
        slow5->fp = NULL;

        if (slow5->compress != NULL) {
            press_destroy(&(slow5->compress));
        }

        free(slow5);
    }

    return ret;
}

uint8_t slow5_write_hdr(struct slow5_file *slow5) {
    uint8_t ret = 0;

    if (slow5 != NULL) {

        switch (slow5->format) {

            case FORMAT_ASCII:
                fprintf(slow5->fp,
                        ASCII_SLOW5_HEADER,
                        slow5->num_read_groups);

                //slow5_write_hdr_data();

                fprintf(slow5->fp,
                        ASCII_COLUMN_HEADER_FULL);
                break;

            case FORMAT_BINARY: {
                fprintf_press(slow5->compress, slow5->fp,
                        BINARY_SLOW5_HEADER,
                        slow5->num_read_groups);

                //slow5_write_hdr_data();

                press_footer_next(slow5->compress);
                fprintf_press(slow5->compress, slow5->fp,
                        ASCII_COLUMN_HEADER_FULL);
            } break;

            default: // TODO handle error
                break;
        }
    }

    return ret;
}

void slow5_write_hdr_data_attr(struct slow5_file *slow5, const char *attr, const char *data) {

    if (slow5 != NULL && slow5->header != NULL) {
        khash_t(s2s) **header_data = slow5->header->header_data;
    }
}
*/

/*
const uint8_t *str_get_slow5_version(const char *str) {
    const uint8_t *version = NULL;

    for (size_t i = 0; i < sizeof SLOW5_VERSION_MAP / sizeof SLOW5_VERSION_MAP[0]; ++ i) {
        const struct SLOW5VersionMap map = SLOW5_VERSION_MAP[i];
        const char *version_name = map.version_str;
        if (strcmp(version_name, str) == 0) {
            version = map.version;
            break;
        }
    }

    return version;
}
*/

//int main(void) {

    /*
    slow5_f = slow5_open("../test/data/out/a.out/test.slow5", "w");
    slow5_write_hdr(slow5_f);
    slow5_close(slow5_f);
    */

    /*
    slow5 = slow5_open("../test/data/out/a.out/test.slow5", "r");
    slow5_read_hdr(slow5);
    slow5_close(slow5);
    */

    /*
     * slow5_file *f_in = slow5_fopen("hi.slow5", "r");
     * slow5_file *f_out = slow5_fopen("hi.blow5", "w");
     */
    //FILE *f_in = fopen("../test/data/err/version_too_large.slow5", "r");
    //FILE *f_out = fopen("hi.blow5", "w");

 //   struct slow5_file *s5p = slow5_open("../test/data/exp/one_fast5/exp_1.slow5", "r");
    //struct SLOW5 *slow5 = slow5_init_empty(void);
    //slow5_read(slow5, FORMAT_ASCII, f_in);

    /*
     * slow5_write(f_in, f_out);
     */
    /*
    struct SLOW5WriteConf *conf = slow5_wconf_init(FORMAT_BINARY, COMPRESS_GZIP);
    slow5_write(slow5, conf, f_out);

    slow5_wconf_destroy(&conf);
    */
/*
    slow5_hdr_print(s5p->header);
    struct slow5_rec *rec = NULL;
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &rec, s5p);
    slow5_rec_free(rec);
    slow5_close(s5p);
    */

    //fclose(f_in);
    //fclose(f_out);
//}

/*
slow5_convert(Format from_format, Format to_format, FILE *from_file, FILE *to_file) {
}

f2s_single(FILE *fast5, FILE *slow5) {
}

f2s() {
    f2s_single();
    merge();
}
*/
