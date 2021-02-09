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
#include "klib/ksort.h"

KSORT_INIT_STR

// TODO fail with getline if end of file occurs on a non-empty line
// TODO (void) cast if ignoring return value
// TODO strlen of macros at compile time

// Initial string buffer capacity for parsing the data header
#define SLOW5_HEADER_DATA_BUF_INIT_CAP (1024) // 2^10 TODO is this too much? Or put to a page length
// Max length is 6 (−32768) for a int16_t
#define INT16_MAX_LENGTH (6)
// Max length is 10 (4294967295) for a uint32_t
#define UINT32_MAX_LENGTH (10)
// Fixed string buffer capacity for storing signal
#define SLOW5_SIGNAL_BUF_FIXED_CAP (8) // 2^3 since INT16_MAX_LENGTH=6
// Initial capacity for converting the header to a string
#define SLOW5_HEADER_STR_INIT_CAP (1024) // 2^10 TODO is this good? Or put to a page length

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
        fclose(fp);
        s5p = NULL;
    } else {
        s5p = (struct slow5_file *) calloc(1, sizeof *s5p);
        MALLOC_CHK(s5p);

        s5p->fp = fp;
        s5p->format = format;
        s5p->header = header;
        // TODO only use when reading?
        // TODO determine compression?
        s5p->compress = press_init(COMPRESS_NONE);

        if ((s5p->meta.fd = fileno(fp)) == -1) {
            slow5_close(s5p);
            s5p = NULL;
        }
        s5p->meta.pathname = pathname;
        s5p->meta.start_rec_offset = ftello(fp);
    }

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
            if (s5p->index->fp != NULL) {
                assert(fclose(s5p->index->fp) == 0);
            }
            s5p->index->fp = fopen(s5p->index->pathname, "w");
            slow5_idx_write(s5p->index);
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
    char *buf = NULL;

    // Parse slow5 header

    if (format == FORMAT_ASCII) {

        // Buffer for file parsing
        size_t cap = SLOW5_HEADER_DATA_BUF_INIT_CAP;
        buf = (char *) malloc(cap * sizeof *buf);
        char *bufp;
        MALLOC_CHK(buf);
        ssize_t buf_len;
        int err;

        // 1st line
        if ((buf_len = getline(&buf, &cap, fp)) == -1) {
            free(buf);
            free(header);
            return NULL;
        }
        buf[buf_len - 1] = '\0'; // Remove newline for later parsing
        // "#slow5_version"
        bufp = buf;
        char *tok = strsep_mine(&bufp, SEP);
        if (strcmp(tok, HEADER_FILE_VERSION_ID) != 0) {
            free(buf);
            free(header);
            return NULL;
        }
        // Parse file version
        tok = strsep_mine(&bufp, SEP);
        // Parse file version string
        // TODO necessary to parse it now?
        char *toksub;
        if ((toksub = strsep_mine(&tok, ".")) == NULL) { // Major version
            free(buf);
            free(header);
            return NULL;
        }
        header->version.major = ato_uint8(toksub, &err);
        if (err == -1 || (toksub = strsep_mine(&tok, ".")) == NULL) { // Minor version
            free(buf);
            free(header);
            return NULL;
        }
        header->version.minor = ato_uint8(toksub, &err);
        if (err == -1 || (toksub = strsep_mine(&tok, ".")) == NULL) { // Patch version
            free(buf);
            free(header);
            return NULL;
        }
        header->version.patch = ato_uint8(toksub, &err);
        if (err == -1 || strsep_mine(&tok, ".") != NULL) { // No more tokenators
            free(buf);
            free(header);
            return NULL;
        }

        // 3rd line
        if ((buf_len = getline(&buf, &cap, fp)) == -1) {
            free(buf);
            free(header);
            return NULL;
        }
        buf[buf_len - 1] = '\0'; // Remove newline for later parsing
        // "#num_read_groups"
        bufp = buf;
        tok = strsep_mine(&bufp, SEP);
        if (strcmp(tok, HEADER_NUM_GROUPS_ID) != 0) {
            free(buf);
            free(header);
            return NULL;
        }
        // Parse num read groups
        tok = strsep_mine(&bufp, SEP);
        header->num_read_groups = ato_uint32(tok, &err);
        if (err == -1) {
            free(buf);
            free(header);
            return NULL;
        }

        header->data = slow5_hdr_data_init(fp, format, buf, cap, header->num_read_groups, &header->num_attrs, NULL);

    } else if (format == FORMAT_BINARY) {
        const char magic[] = BINARY_MAGIC_NUMBER;

        char buf_magic[sizeof magic]; // TODO is this a vla?
        press_method_t method;
        uint32_t header_size;

        // TODO pack and do one read
        // TODO test if version is recognised

        if (fread(buf_magic, sizeof *magic, sizeof magic, fp) != sizeof magic ||
                memcmp(magic, buf_magic, sizeof *magic * sizeof magic) != 0 ||
                fread(&header->version.major, sizeof header->version.major, 1, fp) != 1 ||
                fread(&header->version.minor, sizeof header->version.minor, 1, fp) != 1 ||
                fread(&header->version.patch, sizeof header->version.patch, 1, fp) != 1 ||
                fread(&method, sizeof method, 1, fp) != 1 ||
                fread(&header->num_read_groups, sizeof header->num_read_groups, 1, fp) != 1 ||
                fseek(fp, BINARY_HEADER_SIZE_OFFSET, SEEK_SET) == -1 ||
                fread(&header_size, sizeof header_size, 1, fp) != 1) {
            free(header);
            return NULL;
        }

        size_t cap = SLOW5_HEADER_DATA_BUF_INIT_CAP;
        buf = (char *) malloc(cap * sizeof *buf);
        MALLOC_CHK(buf);

        // Header data
        uint32_t header_act_size;
        header->data = slow5_hdr_data_init(fp, format, buf, cap, header->num_read_groups, &header->num_attrs, &header_act_size);
        if (header_act_size != header_size) {
            slow5_hdr_data_free(header->data, header->num_read_groups);
            free(buf);
            free(header);
            return NULL;
        }
    }

    free(buf);

    return header;
}

/**
 * Get the header as a string in the specified format.
 *
 * Returns NULL if s5p is NULL
 * or format is FORMAT_UNKNOWN
 * or an internal error occurs.
 *
 * @param   s5p     slow5 file
 * @param   format  slow5 format to write the entry in
 * @param   written number of bytes written to the returned buffer
 * @return  malloced memory storing the slow5 header representation,
 *          to use free() on afterwards
 */
void *slow5_hdr_to_mem(struct slow5_file *s5p, enum slow5_fmt format, size_t *written) {
    char *mem = NULL;

    if (s5p == NULL || format == FORMAT_UNKNOWN) {
        return mem;
    }

    size_t len = 0;
    size_t cap = SLOW5_HEADER_STR_INIT_CAP;
    mem = (char *) malloc(cap * sizeof *mem);
    MALLOC_CHK(mem);
    uint32_t header_size;

    if (format == FORMAT_ASCII) {

        struct slow5_version *version = &s5p->header->version;

        // Relies on SLOW5_HEADER_DATA_BUF_INIT_CAP being bigger than
        // strlen(ASCII_SLOW5_HEADER) + UINT32_MAX_LENGTH + strlen("\0")
        int len_ret = sprintf(mem, ASCII_SLOW5_HEADER_FORMAT,
                version->major,
                version->minor,
                version->patch,
                s5p->header->num_read_groups);
        if (len_ret <= 0) {
            free(mem);
            return NULL;
        }
        len = len_ret;

    } else if (format == FORMAT_BINARY) {

        struct slow5_version *version = &s5p->header->version;

        // Relies on SLOW5_HEADER_DATA_BUF_INIT_CAP
        // being at least 68 + 1 (for '\0') bytes
        const char magic[] = BINARY_MAGIC_NUMBER;
        memcpy(mem, magic, sizeof magic * sizeof *magic);
        len += sizeof magic * sizeof *magic;
        memcpy(mem + len, &version->major, sizeof version->major);
        len += sizeof version->major;
        memcpy(mem + len, &version->minor, sizeof version->minor);
        len += sizeof version->minor;
        memcpy(mem + len, &version->patch, sizeof version->patch);
        len += sizeof version->patch;
        memcpy(mem + len, &s5p->compress->method, sizeof s5p->compress->method);
        len += sizeof s5p->compress->method;
        memcpy(mem + len, &s5p->header->num_read_groups, sizeof s5p->header->num_read_groups);
        len += sizeof s5p->header->num_read_groups;

        memset(mem + len, '\0', BINARY_HEADER_SIZE_OFFSET - len);
        len = BINARY_HEADER_SIZE_OFFSET;

        // Skip header size for later
        len += sizeof header_size;
    }

    // Get unsorted list of header data attributes.
    // Relies on header data having at least one hash map
    // and all maps have the same attributes.
    // Unless user has manually changed things this should be fine.

    khash_t(s2s) *hdr_data = s5p->header->data[0];
    const char **header_attrs = (const char **) malloc(s5p->header->num_attrs * sizeof *header_attrs);
    MALLOC_CHK(header_attrs);

    uint32_t i = 0;
    for (khint_t j = kh_begin(hdr_data); j < kh_end(hdr_data); ++ j) {
        if (kh_exist(hdr_data, j)) {
            header_attrs[i] = kh_key(hdr_data, j);
            ++ i;
        }
    }

    // Sort header data attributes alphabetically
    ks_mergesort(str, s5p->header->num_attrs, header_attrs, 0);

    size_t len_to_cp;
    // Write header data attributes to string
    for (uint64_t j = 0; j < (uint64_t) s5p->header->num_attrs; ++ j) {
        const char *attr = header_attrs[j];

        // Realloc if necessary
        if (len + 1 + strlen(attr) >= cap) { // + 1 for SLOW5_HEADER_DATA_PREFIX_CHAR
            cap *= 2;
            mem = (char *) realloc(mem, cap * sizeof *mem);
            MALLOC_CHK(mem);
        }

        mem[len] = SLOW5_HEADER_DATA_PREFIX_CHAR;
        ++ len;
        memcpy(mem + len, attr, strlen(attr));
        len += strlen(attr);

        for (uint64_t k = 0; k < (uint64_t) s5p->header->num_read_groups; ++ k) {
            const khash_t(s2s) *hdr_data = s5p->header->data[k];
            khint_t pos = kh_get(s2s, hdr_data, attr);

            if (pos != kh_end(hdr_data)) {
                const char *value = kh_value(hdr_data, pos);

                // Realloc if necessary
                if (len + 1 >= cap) { // +1 for SEP_CHAR
                    cap *= 2;
                    mem = (char *) realloc(mem, cap * sizeof *mem);
                    MALLOC_CHK(mem);
                }

                mem[len] = SEP_CHAR;
                ++ len;

                if (value != NULL) {
                    len_to_cp = strlen(value);

                    // Realloc if necessary
                    if (len + len_to_cp >= cap) {
                        cap *= 2;
                        mem = (char *) realloc(mem, cap * sizeof *mem);
                        MALLOC_CHK(mem);
                    }

                    memcpy(mem + len, value, len_to_cp);
                    len += len_to_cp;
                }
            } else {
                // TODO don't think this is possible?
            }
        }

        // Realloc if necessary
        if (len + 1 >= cap) { // +1 for '\n'
            cap *= 2;
            mem = (char *) realloc(mem, cap * sizeof *mem);
            MALLOC_CHK(mem);
        }

        mem[len] = '\n';
        ++ len;
    }
    free(header_attrs);

    // Type header
    // Realloc if necessary
    const char *str_to_cp = ASCII_TYPE_HEADER_MIN "\n";
    len_to_cp = strlen(str_to_cp);
    if (len + len_to_cp >= cap) {
        cap *= 2;
        mem = (char *) realloc(mem, cap * sizeof *mem);
        MALLOC_CHK(mem);
    }
    memcpy(mem + len, str_to_cp, len_to_cp);
    len += len_to_cp;
    // TODO Put other types from auxillary fields

    // Column header
    // Realloc if necessary
    str_to_cp = ASCII_COLUMN_HEADER_MIN "\n";
    len_to_cp = strlen(str_to_cp);
    if (len + len_to_cp >= cap) {
        cap *= 2;
        mem = (char *) realloc(mem, cap * sizeof *mem);
        MALLOC_CHK(mem);
    }
    memcpy(mem + len, str_to_cp, len_to_cp);
    len += len_to_cp;

    if (format == FORMAT_ASCII) {
        // Realloc if necessary
        if (len + 1 >= cap) { // +1 for '\0'
            cap *= 2;
            mem = (char *) realloc(mem, cap * sizeof *mem);
            MALLOC_CHK(mem);
        }

        mem[len] = '\0';
    } else if (format == FORMAT_BINARY) {
        header_size = len - (BINARY_HEADER_SIZE_OFFSET + sizeof header_size);
        memcpy(mem + BINARY_HEADER_SIZE_OFFSET, &header_size, sizeof header_size);
    }

    if (written != NULL) {
        *written = len;
    }

    return (void *) mem;
}

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
int slow5_hdr_fprint(FILE *fp, struct slow5_file *s5p, enum slow5_fmt format) {
    int ret;
    void *hdr;
    size_t hdr_size;

    if (fp == NULL || s5p == NULL || (hdr = slow5_hdr_to_mem(s5p, format, &hdr_size)) == NULL) {
        return -1;
    }

    size_t written = fwrite(hdr, hdr_size, 1, fp);
    if (written != 1) {
        ret = -1;
    } else {
        ret = hdr_size; // TODO is this okay
    }

    free(hdr);
    return ret;
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
 * Add a new header data attribute.
 *
 * All values are set to NULL for each read group.
 *
 * Returns -1 if an input parameter is NULL.
 * Returns -2 if the attribute already exists.
 * Returns -3 some internal error.
 * Returns 0 other.
 *
 * @param   attr        attribute name
 * @param   s5p         slow5 file
 * @return  0 on success, <0 on error as described above
 */
int slow5_hdr_add(const char *attr, const struct slow5_file *s5p) {
    if (attr == NULL || s5p == NULL) {
        return -1;
    }

    // Set key
    int absent;
    char *attr_cp = strdup(attr);
    // TODO cast necessary?
    for (uint64_t i = 0; i < (uint64_t) s5p->header->num_read_groups; ++ i) {
        khash_t(s2s) *header_data = s5p->header->data[i];

        khint_t pos = kh_put(s2s, header_data, attr_cp, &absent);
        if (absent == -1) {
            free(attr_cp);
            return -3;
        } else if (absent == 0) {
            free(attr_cp);
            return -2;
        }
        kh_value(header_data, pos) = NULL;
    }

    ++ s5p->header->num_attrs;

    return 0;
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

    slow5_hdr_data_free(header->data, header->num_read_groups);

    free(header);
}


// slow5 header data

khash_t(s2s) **slow5_hdr_data_init(FILE *fp, enum slow5_fmt format, char *buf, size_t cap, uint32_t num_rgs, uint32_t *num_attrs, uint32_t *hdr_len) {

    uint32_t hdr_len_tmp = 0;

    khash_t(s2s) **hdr_data = (khash_t(s2s) **) malloc(num_rgs * sizeof *hdr_data);
    MALLOC_CHK(hdr_data);

    // TODO check if cast necessary
    for (uint64_t i = 0; i < (uint64_t) num_rgs; ++ i) {
        hdr_data[i] = kh_init(s2s);
        NULL_CHK(hdr_data[i]);
    }


    // Parse slow5 header data

    ssize_t buf_len;

    // Get first line of header data
    assert((buf_len = getline(&buf, &cap, fp)) != -1);
    buf[buf_len - 1] = '\0'; // Remove newline for later parsing
    if (hdr_len != NULL) {
        hdr_len_tmp += buf_len;
    }

    // While the column header hasn't been reached
    while (strncmp(buf, ASCII_TYPE_HEADER_MIN, strlen(ASCII_TYPE_HEADER_MIN)) != 0) {

        // Ensure prefix is there
        assert(buf[0] == SLOW5_HEADER_DATA_PREFIX_CHAR);
        char *shift = buf + strlen(SLOW5_HEADER_DATA_PREFIX); // Remove prefix

        // Get the attribute name
        char *attr = strdup(strsep_mine(&shift, SEP));
        char *val;

        ++ *num_attrs;

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
        if (hdr_len != NULL) {
            hdr_len_tmp += buf_len;
        }
    }
    // Get header
    assert((buf_len = getline(&buf, &cap, fp)) != -1);
    buf[buf_len - 1] = '\0'; // Remove newline for later parsing
    if (hdr_len != NULL) {
        hdr_len_tmp += buf_len;
    }
    assert(strncmp(buf, ASCII_COLUMN_HEADER_MIN, strlen(ASCII_COLUMN_HEADER_MIN)) == 0);

    assert(cap == SLOW5_HEADER_DATA_BUF_INIT_CAP); // TESTING to see if getline has to realloc (if this fails often maybe put a larger buffer size)

    if (hdr_len != NULL) {
        *hdr_len = hdr_len_tmp;
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
    char *read_mem = NULL;

    // Create index if NULL
    if (s5p->index == NULL && (s5p->index = slow5_idx_init(s5p)) == NULL) {
        // index failed to init
        return -2;
    }

    // Get index record
    struct slow5_rec_idx read_index;
    if (slow5_idx_get(s5p->index, read_id, &read_index) == -1) {
        // read_id not found in index
        return -3;
    }

    if (s5p->format == FORMAT_ASCII) {

        // Malloc string to hold the read
        read_mem = (char *) malloc(read_index.size * sizeof *read_mem);
        MALLOC_CHK(read_mem);

        // Read into the string
        // Don't read in newline for parsing
        ssize_t bytes_to_read = (read_index.size - 1) * sizeof *read_mem;
        if (pread(s5p->meta.fd, read_mem, bytes_to_read, read_index.offset) != bytes_to_read) {
            free(read_mem);
            // reading error
            return -4;
        }

        // Null terminate
        read_mem[read_index.size - 1] = '\0';

    } else if (s5p->format == FORMAT_BINARY) {

        // Malloc string to hold the read
        ssize_t read_size = read_index.size * sizeof *read_mem;
        read_mem = (char *) malloc(read_size);
        MALLOC_CHK(read_mem);

        // Read into the string
        if (pread(s5p->meta.fd, read_mem, read_size, read_index.offset) != read_size) {
            free(read_mem);
            // reading error
            return -4;
        }

    }

    if (*read == NULL) {
        // Allocate memory for read
        *read = (struct slow5_rec *) calloc(1, sizeof **read);
        MALLOC_CHK(*read);
    } else {
        // Free previously allocated strings
        free((*read)->read_id);
    }

    if (slow5_rec_parse(read_mem, read_id, *read, s5p->format) == -1) {
        ret = -5;
    }
    free(read_mem);

    return ret;
}

// Return -1 on failure to parse
int slow5_rec_parse(char *read_mem, const char *read_id, struct slow5_rec *read, enum slow5_fmt format) {
    int ret = 0;
    uint64_t prev_len_raw_signal = 0;

    if (format == FORMAT_ASCII) {

        char *tok;
        if ((tok = strsep_mine(&read_mem, SEP)) == NULL) {
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
                    read->read_id_len = strlen(tok);
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
                (tok = strsep_mine(&read_mem, SEP)) != NULL);

        // Check if all main columns parsed and no more extra columns
        if (i != SLOW5_COLS_NUM) {
            ret = -1;
        }

    } else if (format == FORMAT_BINARY) {

        int i = 0;
        bool main_cols_parsed = false;

        size_t size = 0;
        uint64_t offset = 0;

        while (!main_cols_parsed) {

            switch (i) {

                case COL_read_id:
                    size = sizeof read->read_id_len;
                    memcpy(&read->read_id_len, read_mem + offset, size);
                    offset += size;

                    size = read->read_id_len * sizeof *read->read_id;
                    read->read_id = strndup(read_mem + offset, size);
                    offset += size;
                    break;

                case COL_read_group:
                    size = sizeof read->read_group;
                    memcpy(&read->read_group, read_mem + offset, size);
                    offset += size;
                    break;

                case COL_digitisation:
                    size = sizeof read->digitisation;
                    memcpy(&read->digitisation, read_mem + offset, size);
                    offset += size;
                    break;

                case COL_offset:
                    size = sizeof read->offset;
                    memcpy(&read->offset, read_mem + offset, size);
                    offset += size;
                    break;

                case COL_range:
                    size = sizeof read->range;
                    memcpy(&read->range, read_mem + offset, size);
                    offset += size;
                    break;

                case COL_sampling_rate:
                    size = sizeof read->sampling_rate;
                    memcpy(&read->sampling_rate, read_mem + offset, size);
                    offset += size;
                    break;

                case COL_len_raw_signal:
                    size = sizeof read->len_raw_signal;
                    memcpy(&read->len_raw_signal, read_mem + offset, size);
                    offset += size;
                    break;

                case COL_raw_signal:
                    size = read->len_raw_signal * sizeof *read->raw_signal;
                    read->raw_signal = (int16_t *) malloc(size);
                    MALLOC_CHK(read->raw_signal);
                    memcpy(read->raw_signal, read_mem + offset, size);
                    offset += size;
                    break;

                // All columns parsed
                default:
                    main_cols_parsed = true;
                    break;
            }

            ++ i;
        }
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
    // TODO implement binary parsing
    if (read == NULL || s5p == NULL) {
        return -1;
    }

    int ret = 0;
    char *read_mem = NULL;

    if (s5p->format == FORMAT_ASCII) {
        size_t cap = 0;
        ssize_t read_len;
        if ((read_len = getline(&read_mem, &cap, s5p->fp)) == -1) {
            free(read_mem);
            return -2;
        }
        read_mem[read_len - 1] = '\0'; // Remove newline for parsing

        if (*read == NULL) {
            // Allocate memory for read
            *read = (struct slow5_rec *) calloc(1, sizeof **read);
        } else {
            // Free previously allocated read id
            free((*read)->read_id);
        }

        if (slow5_rec_parse(read_mem, NULL, *read, s5p->format) == -1) {
            ret = -3;
        }

        free(read_mem);

    } else if (s5p->format == FORMAT_BINARY) {

        if (*read == NULL) {
            // Allocate memory for read
            *read = (struct slow5_rec *) calloc(1, sizeof **read);
        } else {
            // Free previously allocated read id
            free((*read)->read_id);
        }

        if (fread(&(*read)->read_id_len, sizeof (*read)->read_id_len, 1, s5p->fp) != 1) {
            return -3;
        }

        (*read)->read_id = (char *) malloc(((*read)->read_id_len + 1) * sizeof (*read)->read_id); // +1 for '\0'
        MALLOC_CHK((*read)->read_id);

        if (fread((*read)->read_id, sizeof *(*read)->read_id, (*read)->read_id_len, s5p->fp) != (*read)->read_id_len ||
                fread(&(*read)->read_group, sizeof (*read)->read_group, 1, s5p->fp) != 1 ||
                fread(&(*read)->digitisation, sizeof (*read)->digitisation, 1, s5p->fp) != 1 ||
                fread(&(*read)->offset, sizeof (*read)->offset, 1, s5p->fp) != 1 ||
                fread(&(*read)->range, sizeof (*read)->range, 1, s5p->fp) != 1 ||
                fread(&(*read)->sampling_rate, sizeof (*read)->sampling_rate, 1, s5p->fp) != 1 ||
                fread(&(*read)->len_raw_signal, sizeof (*read)->len_raw_signal, 1, s5p->fp) != 1) {
            free((*read)->read_id);
            return -3;
        }
        (*read)->read_id[(*read)->read_id_len] = '\0'; // Null terminate

        (*read)->raw_signal = (int16_t *) malloc((*read)->len_raw_signal * sizeof (*read)->raw_signal);
        MALLOC_CHK((*read)->raw_signal);

        if (fread((*read)->raw_signal, sizeof *(*read)->raw_signal, (*read)->len_raw_signal, s5p->fp) != (*read)->len_raw_signal) {
            free((*read)->read_id);
            free((*read)->raw_signal);
            return -3;
        }
    }

    return ret;
}

/**
 * Add a read entry to the slow5 file.
 *
 * Return
 *  0   the read was successfully stored
 * -1   read or s5p or read->read_id is NULL
 * -2   the index was not previously init and failed to init
 * -3   duplicate read id
 * -4   writing failure
 *
 * @param   read    slow5_rec ptr
 * @param   s5p     slow5 file
 * @return  error code described above
 */
int slow5_rec_add(struct slow5_rec *read, struct slow5_file *s5p) {
    if (read == NULL || read->read_id == NULL || s5p == NULL) {
        return -1;
    }

    // Create index if NULL
    if (s5p->index == NULL && (s5p->index = slow5_idx_init(s5p)) == NULL) {
        // index failed to init
        return -2;
    }

    // Duplicate read id
    if (slow5_idx_get(s5p->index, read->read_id, NULL) == 0) {
        return -3;
    }

    // Append record to file
    void *mem = NULL;
    size_t bytes;
    if ((mem = slow5_rec_to_mem(read, s5p->format, &bytes)) == NULL) {
        return -4;
    }
    if (fseek(s5p->fp, 0L, SEEK_END) != 0) {
        free(mem);
        return -4;
    }
    uint64_t offset = ftello(s5p->fp);
    if (fwrite(mem, bytes, 1, s5p->fp) != 1) {
        free(mem);
        return -4;
    }
    free(mem);

    // Update index
    slow5_idx_insert(s5p->index, strdup(read->read_id), offset, bytes);

    return 0;
}

/**
 * Remove a read entry at a read_id in a slow5 file.
 *
 * Return
 *  0   the read was successfully stored
 * -1   an input parameter is NULL
 * -2   the index was not previously init and failed to init
 * -3   read_id was not found in the index
 *
 * @param   read_id the read identifier
 * @param   s5p     slow5 file
 * @return  error code described above
 */
int slow5_rec_rm(const char *read_id, struct slow5_file *s5p) {
    if (read_id == NULL || s5p == NULL) {
        return -1;
    }

    // Create index if NULL
    if (s5p->index == NULL && (s5p->index = slow5_idx_init(s5p)) == NULL) {
        // index failed to init
        return -2;
    }

    // Get index record
    struct slow5_rec_idx read_index;
    if (slow5_idx_get(s5p->index, read_id, &read_index) == -1) {
        // read_id not found in index
        return -3;
    }

    // TODO
    // remove record from file
    // update index

    return 0;
}

/**
 * Print a read entry in the correct format with newline character to a file pointer.
 *
 * Error if fp or read is NULL,
 * of if the format is FORMAT_UNKNOWN.
 *
 * On success, the number of bytes written is returned.
 * On error, -1 is returned.
 *
 * @param   fp      output file pointer
 * @param   read    slow5_rec pointer
 * @return  number of bytes written, -1 on error
 */
int slow5_rec_fprint(FILE *fp, struct slow5_rec *read, enum slow5_fmt format) {
    int ret;
    void *read_mem;
    size_t read_size;

    if (fp == NULL || read == NULL || (read_mem = slow5_rec_to_mem(read, format, &read_size)) == NULL) {
        return -1;
    }

    size_t written = fwrite(read_mem, read_size, 1, fp);
    if (written != 1) {
        ret = -1;
    } else {
        ret = read_size; // TODO is this okay
    }

    free(read_mem);
    return ret;
}

/**
 * Get the read entry in the specified format.
 *
 * Returns NULL if read is NULL,
 * or format is FORMAT_UNKNOWN,
 * or the read attribute values are invalid
 *
 * @param   read    slow5_rec pointer
 * @param   format  slow5 format to write the entry in
 * @param   written number of bytes written to the returned buffer
 * @return  malloced string to use free() on, NULL on error
 */
void *slow5_rec_to_mem(struct slow5_rec *read, enum slow5_fmt format, size_t *written) {
    char *mem = NULL;

    if (read == NULL || format == FORMAT_UNKNOWN) {
        return NULL;
    }

    size_t curr_len = 0;

    if (format == FORMAT_ASCII) {

        char *digitisation_str = double_to_str(read->digitisation);
        char *offset_str = double_to_str(read->offset);
        char *range_str = double_to_str(read->range);
        char *sampling_rate_str = double_to_str(read->sampling_rate);
        int curr_len_tmp = asprintf_mine(&mem,
                SLOW5_COLS(GENERATE_FORMAT_STRING_SEP, GENERATE_NULL),
                read->read_id,
                read->read_group,
                digitisation_str,
                offset_str,
                range_str,
                sampling_rate_str,
                read->len_raw_signal);
        free(digitisation_str);
        free(offset_str);
        free(range_str);
        free(sampling_rate_str);
        if (curr_len_tmp > 0) {
            curr_len = curr_len_tmp;
        } else {
            free(mem);
            return NULL;
        }
        MALLOC_CHK(mem);

        // TODO memory optimise
        // <max length> = <current length> + (<max signal length> + ','/'\n') * <number of signals> + '\0'
        // <max length> = <current length> + '\n' + '\0'
        const size_t max_len = read->len_raw_signal != 0 ? curr_len + (INT16_MAX_LENGTH + 1) * read->len_raw_signal + 1 : curr_len + 1 + 1;
        mem = (char *) realloc(mem, max_len * sizeof *mem);
        MALLOC_CHK(mem);

        char sig_buf[SLOW5_SIGNAL_BUF_FIXED_CAP];

        uint64_t i;
        for (i = 1; i < read->len_raw_signal; ++ i) {
            int sig_len = sprintf(sig_buf, FORMAT_STRING_RAW_SIGNAL SEP_RAW_SIGNAL, read->raw_signal[i - 1]);

            memcpy(mem + curr_len, sig_buf, sig_len);
            curr_len += sig_len;
        }
        if (read->len_raw_signal > 0) {
            // Trailing signal
            int len_to_cp = sprintf(sig_buf, FORMAT_STRING_RAW_SIGNAL "\n", read->raw_signal[i - 1]);
            strcpy(mem + curr_len, sig_buf); // Copies null byte as well
            curr_len += len_to_cp;
        } else {
            // Trailing newline
            strcpy(mem + curr_len, "\n"); // Copies null byte as well
            curr_len += 1;
        }

    } else if (format == FORMAT_BINARY) {

        size_t len = sizeof read->read_id_len +
            read->read_id_len * sizeof *read->read_id +
            sizeof read->read_group +
            sizeof read->digitisation +
            sizeof read->offset +
            sizeof read->range +
            sizeof read->sampling_rate +
            sizeof read->len_raw_signal +
            read->len_raw_signal * sizeof read->raw_signal;
        mem = (char *) malloc(len * sizeof *mem);
        MALLOC_CHK(mem);

        memcpy(mem + curr_len, &read->read_id_len, sizeof read->read_id_len);
        curr_len += sizeof read->read_id_len;
        memcpy(mem + curr_len, read->read_id, read->read_id_len * sizeof *read->read_id);
        curr_len += read->read_id_len * sizeof *read->read_id;
        memcpy(mem + curr_len, &read->read_group, sizeof read->read_group);
        curr_len += sizeof read->read_group;
        memcpy(mem + curr_len, &read->digitisation, sizeof read->digitisation);
        curr_len += sizeof read->digitisation;
        memcpy(mem + curr_len, &read->offset, sizeof read->offset);
        curr_len += sizeof read->offset;
        memcpy(mem + curr_len, &read->range, sizeof read->range);
        curr_len += sizeof read->range;
        memcpy(mem + curr_len, &read->sampling_rate, sizeof read->sampling_rate);
        curr_len += sizeof read->sampling_rate;
        memcpy(mem + curr_len, &read->len_raw_signal, sizeof read->len_raw_signal);
        curr_len += sizeof read->len_raw_signal;
        memcpy(mem + curr_len, read->raw_signal, read->len_raw_signal * sizeof *read->raw_signal);
        curr_len += read->len_raw_signal * sizeof *read->raw_signal;
    }

    if (written != NULL) {
        *written = curr_len;
    }

    return (void *) mem;
}

void slow5_rec_free(struct slow5_rec *read) {
    if (read != NULL) {
        free(read->read_id);
        free(read->raw_signal);
        free(read);
    }
}


/**
 * Create the index file for slow5 file.
 * Overrides if already exists.
 *
 * Return -1 on error,
 * 0 on success.
 *
 * @param   s5p slow5 file structure
 * @return  error codes described above
 */
int slow5_idx(struct slow5_file *s5p) {
    char *index_pathname;
    if (s5p == NULL || s5p->meta.pathname == NULL ||
            (index_pathname = get_slow5_idx_path(s5p->meta.pathname)) == NULL) {
        return -1;
    } else if (slow5_idx_to(s5p, index_pathname) == -1) {
        free(index_pathname);
        return -1;
    }

    free(index_pathname);
    return 0;
}


/**
 * Print the binary end of file to a file pointer.
 *
 * On success, the number of bytes written is returned.
 * On error, -1 is returned.
 *
 * @param   fp      output file pointer
 * @return  number of bytes written, -1 on error
 */
ssize_t slow5_eof_fprint(FILE *fp) {
    const char eof[] = BINARY_EOF;

    size_t written;
    if ((written = fwrite(eof, sizeof *eof, sizeof eof, fp)) != sizeof eof) {
        return -1;
    } else {
        return written;
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
    memcpy(str, path, strlen(path));
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
