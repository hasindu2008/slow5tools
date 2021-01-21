#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h> // TODO use better error handling?
#include "slow5.h"
#include "slow5idx.h"
#include "press.h"
#include "error.h"
#include "misc.h"

// TODO fail with getline if end of file occurs on a non-empty line
// TODO (void) cast if ignoring return value

// TODO use kstring?
// String buffer capacity for parsing the data header
#define SLOW5_HEADER_DATA_BUF_INIT_CAP (1024) // 2^10 TODO is this too much? Or put to a page length
#define SLOW5_INDEX_BUF_INIT_CAP (64) // 2^6 TODO is this too little?


/* Private to this file */

// slow5 file
static inline struct slow5_file *slow5_init_empty(void);
static struct slow5_file *slow5_init(FILE *fp, const char *pathname, enum slow5_fmt format, bool is_fp_preowned);

// slow5 header
static inline struct slow5_hdr *slow5_hdr_init_empty(void);
static struct slow5_hdr *slow5_hdr_init(FILE *fp, enum slow5_fmt format);
static inline void slow5_hdr_free(struct slow5_hdr *header);

// slow5 header data
static inline khash_t(s2s) **slow5_hdr_data_init_empty(uint32_t num_rgs);
static khash_t(s2s) **slow5_hdr_data_init(FILE *fp, enum slow5_fmt format, char *buf, size_t buf_cap, uint32_t num_rgs) {
static inline void slow5_hdr_data_free(khash_t(s2s) **hdr_data, uint32_t num_rgs);

// slow5 index
static inline khash_t(s2i) *slow5_idx_init_empty(void);
static khash_t(s2i) *slow5_idx_init(FILE *index_fp);
static void slow5_idx_free(khash_t(s2i) *index);

// slow5 extension parsing
static enum slow5_fmt name_get_slow5_fmt(const char *name);
static enum slow5_fmt path_get_slow5_fmt(const char *path);
static const char *slow5_fmt_get_name(enum slow5_fmt format);
static const char *get_slow5_idx_path(const char *path);

/* Definitions */

// slow5 file

static inline struct slow5_file *slow5_init_empty(void) {
    struct slow5_file *s5p = (struct slow5_file *) calloc(1, sizeof *s5p);
    MALLOC_CHK(s5p);

    return s5p;
}

// Pathname can be NULL if format is not unknown
static struct slow5_file *slow5_init(FILE *fp, const char *pathname, enum slow5_fmt format, bool is_fp_preowned) {
    NULL_CHK(fp);

    struct slow5_file *s5p = slow5_init_empty();
    s5p->fp = fp;
    s5p->is_fp_preowned = is_fp_preowned;

    if (format == FORMAT_UNKNOWN) {
        // Attempt to determine format
        // from pathname
        NULL_CHK(pathname);
        format = pathname_get_slow5_fmt(pathname);
        assert(format != FORMAT_UNKNOWN);
    }
    s5p->format = format;

    // TODO only use when reading?
    // TODO determine compression?
    //s5p->compress = press_init(compress);
    s5p->header = slow5_hdr_init(fp, format);
    //s5p->index = slow5_idx_init(index_fp);

    return s5p;
}

inline struct slow5_file *slow5_open(const char *pathname, const char *mode) {
    return slow5_open_with(pathname, mode, FORMAT_UNKNOWN);
}

inline struct slow5_file *slow5_open_with(const char *pathname, const char *mode, enum slow5_fmt format) {
    return slow5_init(fopen(pathname, mode), pathname, format, false);
}

inline struct slow5_file *slow5_init_fp(FILE *fp, enum slow5_fmt format) {
    // Only determine pathname if format is declared as unknown
    if (format == FORMAT_UNKNOWN) {
        return slow5_init(fp, get_pathname(fp), format, true);
    } else {
        return slow5_init(fp, NULL, format, true);
    }
}

// Can be used with slow5_open or slow5_init_fp
void slow5_free(struct slow5_file *s5p) {
    NULL_CHK(s5p);

    if (!s5p->is_fp_preowned) {
        assert(fclose(s5p->fp) == 0);
    }

    press_free(s5p->compress);
    slow5_hdr_free(s5p->header);
    slow5_idx_free(s5p->index);

    free(s5p);
}


// slow5 header

static inline struct slow5_hdr *slow5_hdr_init_empty(void) {
    struct slow5_hdr *hdr = (struct slow5_hdr *) calloc(1, sizeof *(hdr));
    MALLOC_CHK(hdr);

    return hdr;
}

static struct slow5_hdr *slow5_hdr_init(FILE *fp, enum slow5_fmt format) {
    struct slow5_hdr *header = slow5_hdr_init_empty();

    // Parse slow5 header
    switch (format) {

        case FORMAT_ASCII: {

            // Buffer for file parsing
            size_t cap = SLOW5_HEADER_DATA_BUF_INIT_CAP;
            char *buf = (char *) malloc(cap * sizeof *buf);
            MALLOC_CHK(buf);
            ssize_t buf_len;

            // 1st line
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later format parsing
            // "#file_format"
            char *tok = strtok_solo(buf, SEP);
            assert(strcmp(tok, FILE_FORMAT_HEADER_ID) == 0);
            // Parse format name
            tok = strtok_solo(NULL, SEP);
            assert(format == str_get_slow5_fmt(tok));

            // 2nd line
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later parsing
            // "#file_version"
            tok = strtok_solo(buf, SEP);
            assert(strcmp(tok, FILE_VERSION_HEADER_ID) == 0);
            // Parse file version
            tok = strtok_solo(NULL, SEP);
            header->version_str = strdup(tok);
            // Parse file version string
            // TODO necessary to parse it now?
            assert((tok = strtok_solo(tok, ".")) != NULL); // Major version
            header->version.major = ato_uint8(tok);
            assert((tok = strtok_solo(NULL, ".")) != NULL); // Minor version
            header->version.minor = ato_uint8(tok);
            assert((tok = strtok_solo(NULL, ".")) != NULL); // Patch version
            header->version.patch = ato_uint8(tok);
            assert(strtok_solo(NULL, ".") == NULL); // No more tokenators

            // 3rd line
            assert((buf_len = getline(&buf, &cap, fp)) != -1);
            buf[buf_len - 1] = '\0'; // Remove newline for later parsing
            // "#num_read_groups"
            tok = strtok_solo(buf, SEP);
            assert(strcmp(tok, NUM_GROUPS_HEADER_ID) == 0);
            // Parse num read groups
            tok = strtok_solo(NULL, SEP);
            header->num_read_groups = ato_uint32(tok);

            // Header data
            header->data = slow5_hdr_data_init(fp, format, buf, cap, header->num_read_groups);

            free(buf);

        } break;

        case FORMAT_BINARY: // TODO
            break;
    }
}

void slow5_hdr_free(struct slow5_hdr *header) {
    //NULL_CHK(header); TODO needed or not?

    // Free version string
    if (header->version_str != NULL) {
        free(header->version_str);
        header->version_str = NULL;
    }

    slow5_hdr_data_free(header->data, header->num_read_groups);

    free(header);
}


// slow5 header data

static inline khash_t(s2s) **slow5_hdr_data_init_empty(uint32_t num_rgs) {
    khash_t(s2s) **hdr_data = (khash_t(s2s) **) malloc(num_rgs * sizeof *hdr_data);
    MALLOC_CHK(hdr_data);

    // TODO check if cast necessary
    for (uint64_t i = 0; i < (uint64_t) num_rgs; ++ i) {
        hdr_data[i] = kh_init(s2s);
        NULL_CHK(hdr_data[i]);
    }

    return hdr_data;
}

static khash_t(s2s) **slow5_hdr_data_init(FILE *fp, enum slow5_fmt format, char *buf, size_t buf_cap, uint32_t num_rgs) {

    khash_t(s2s) **hdr_data = slow5_hdr_data_init_empty(num_rgs);

    // Parse slow5 header data
    switch (format) {

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
                char *attr = strdup(strtok_solo(shift, SEP));
                char *val;

                // Iterate through the values
                uint32_t i = 0;
                while ((val = strtok_solo(NULL, SEP)) != NULL && i <= num_rgs - 1) {

                    // Set key
                    int absent;
                    khint_t pos = kh_put(s2s, hdr_data[i], attr, &absent);
                    assert(absent != -1);

                    // Set value
                    kh_val(hdr_data[i], pos) = strdup(val);

                    ++ i;

                    printf("%s -> %s\n", attr, val); // TESTING
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

static inline void slow5_hdr_data_free(khash_t(s2s) **hdr_data, uint32_t num_rgs) {

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


// slow5 index

static inline khash_t(s2i) *slow5_idx_init_empty(void) {
    khash_t(s2i) *index = kh_init(s2i);
    NULL_CHK(index);

    return index;
}

static khash_t(s2i) *slow5_idx_init(FILE *index_fp) {
    khash_t(s2i) *index = slow5_idx_init_empty();

    // Buffer for file parsing
    ssize_t cap = SLOW5_INDEX_BUF_INIT_CAP;
    char *buf = (char *) malloc(cap * sizeof *buf);
    MALLOC_CHK(buf);

    // Header
    assert(getline(&buf, &cap, index_fp) != -1);
    assert(strcmp(buf, SLOW5_INDEX_HEADER) == 0);

    // Index data
    ssize_t buf_len;
    while ((buf_len = getline(&buf, &cap, index_fp)) != -1) {
        buf[buf_len - 1] = '\0'; // Remove newline for later parsing

        char *read_id = strdup(strtok_solo(buf, SEP));

        char *offset_str = strtok_solo(NULL, SEP);
        uint64_t offset = ato_uint64(offset_str);

        char *size_str = strtok_solo(NULL, SEP);
        uint64_t size = ato_uint64(size_str);

        // Set key
        int absent;
        khint_t pos = kh_put(s2i, index, read_id, &absent);
        assert(absent != -1);

        // Set value
        struct slow5_rec_idx *read_index = &kh_val(index, pos);
        read_index->offset = offset;
        read_index->size = size;
    }

    assert(cap == SLOW5_INDEX_BUF_INIT_CAP); // TESTING to see if getline has to realloc (if this fails often maybe put a larger buffer size)
    free(buf);

    return index;
}

static void slow5_idx_free(khash_t(s2i) *index) {

    // Free index map
    for (khint_t i = kh_begin(index); i < kh_end(index); ++ i) {
        if (kh_exist(index, i)) {
            free((void *) kh_key(index, i)); // TODO avoid void *
        }
    }

    kh_destroy(s2i, index);
}


// slow5 record

struct slow5_rec *slow5_get(const char *read_id, const struct slow5_file *s5p) {
    NULL_CHK(read_id);
    NULL_CHK(s5p);
    struct slow5_rec *read = NULL;

    kh_get(s2i, s5p->index, read_id);

    switch (s5p->format) {

        case FORMAT_ASCII: {

            while (!finished) {

                // Buffer for read parsing
                char *buf = NULL;
                size_t cap = 0;
                ssize_t len;

                read = (struct slow5_rec *) calloc(1, sizeof *read);

                // Parse read id
                assert((len = getdelim(&buf, &cap, '\t', stream)) != -1);
                char *read_id = buf;

                // Parse rest of line
                buf = NULL;
                cap = 0;
                assert((len = getline(&buf, &cap, stream)) != -1);

                switch (i) {
                    case COL_read_id:
                        break;
                    case COL_read_group:
                        break;
                    case COL_digitisation:
                        break;
                    case COL_offset:
                        break;
                    case COL_range:
                        break;
                    case COL_sampling_rate:
                        break;
                    case COL_len_raw_signal:
                        break;
                    case COL_raw_signal:
                        break;

                    // All columns parsed
                    default:
                        finished = true;
                        break;
                }

                ++ i;
            }
        } break;

        case FORMAT_BINARY: // TODO
            break;
    }

    return read;
}

void slow5_read(struct SLOW5 *slow5, enum slow5_fmt format, FILE *stream) {
    NULL_CHK(slow5);
    NULL_CHK(stream);

    // Autodetect file type
    // TODO
    if (format == FORMAT_NONE) {
        //format = stream_get_slow5_fmt(stream);
    }

    // Setup compression
    /* TODO put into a slow5_write function?
    if (slow5->format == FORMAT_BINARY) {
        slow5->compress = press_init(COMPRESS_GZIP);
    }
    */

    slow5_read_hdr(slow5->header, format, stream);
    slow5_read_rec(slow5->reads, format, stream);
}

void slow5_print_hdr(const struct slow5_header *hdr) {
    printf("format='%s'\n", slow5_fmt_get_str(hdr->format));
    printf("version_str='%s'\n", hdr->version_str);
    printf("version={%hu,%hu,%hu}\n",
            hdr->version.major,
            hdr->version.minor,
            hdr->version.patch);
    printf("num_rgs=%u\n", hdr->num_read_groups);
}


// slow5 extension parsing

static enum slow5_fmt name_get_slow5_fmt(const char *name) {
    enum slow5_fmt format = FORMAT_NONE;

    for (size_t i = 0; i < sizeof SLOW5_FORMAT_META / sizeof SLOW5_FORMAT_META[0]; ++ i) {
        const struct slow5_fmt_meta meta = SLOW5_FORMAT_META[i];
        if (strcmp(meta.name, name) == 0) {
            format = meta.format;
            break;
        }
    }

    return format;
}

static enum slow5_fmt path_get_slow5_fmt(const char *path) {
    enum slow5_fmt format = FORMAT_NONE;

    // TODO change type from size_t
    size_t i;
    for (i = strlen(path) - 1; i == 0; -- i) {
        if (path[i] == '.' && i != strlen(path) - 1) {
            const char *ext = path + i + 1;
            format = name_get_slow5_format(ext);
            break;
        }
    }

    return format;
}

// Get the slow5 format name from the format
static const char *slow5_fmt_get_name(enum slow5_fmt format) {
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

int main(void) {
    struct slow5_file *slow5_f;

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
    FILE *f_in = fopen("../test/data/exp/one_fast5/exp_1.slow5", "r");
    //FILE *f_in = fopen("../test/data/err/version_too_large.slow5", "r");
    FILE *f_out = fopen("hi.blow5", "w");

    struct SLOW5 *slow5 = slow5_init(FORMAT_ASCII, f_in);
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
    slow5_print_hdr(slow5->header);
    slow5_destroy(&slow5);

    fclose(f_in);
    fclose(f_out);
}

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
