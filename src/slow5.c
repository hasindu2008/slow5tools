#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h> // TODO use better error handling?
#include "slow5.h"
#include "press.h"
#include "error.h"
#include "misc.h"

// String buffer capacity for parsing the SLOW5 header
// TODO use kstring?
#define SLOW5_HEADER_BUF_CAP (128) // 2^7
// String buffer capacity for parsing the data header
#define SLOW5_HEADER_DATA_BUF_CAP (1028) // 2^10 TODO is this too much? Or put to a page length
//#define SLOW5_HEADER_DATA_RGS_INIT_CAP (32) // 2^5 // TODO necessary?

/* Private */

static struct SLOW5File *slow5_init(FILE *fp, const char *pathname, enum SLOW5Format format, enum PressMethod compress, bool is_fp_preowned);



inline struct SLOW5File *slow5_open(const char *pathname, const char *mode) {
    return slow5_open_with(pathname, mode, FORMAT_UNKNOWN, COMPRESS_NONE);
}

inline struct SLOW5File *slow5_open_with(const char *pathname, const char *mode, enum SLOW5Format format, enum PressMethod compress) {
    return slow5_init(fopen(pathname, mode), pathname, format, compress, false);
}

inline struct SLOW5File *slow5_init_fp(FILE *fp, enum SLOW5Format format, enum PressMethod compress) {
    return slow5_init(fp, get_pathname(fp), format, compress, true);
}

static struct SLOW5File *slow5_init(FILE *fp, const char *pathname, enum SLOW5Format format, enum PressMethod compress, bool is_fp_preowned) {
    NULL_CHK(fp);

    struct SLOW5File *s5p = slow5_init_empty();
    s5p->fp = fp;
    s5p->is_fp_preowned = is_fp_preowned;

    if (format == FORMAT_UNKNOWN) {
        // Attempt to determine format
        // from pathname
        NULL_CHK(pathname);
        format = pathname_get_slow5_format(pathname);
        assert(format != FORMAT_UNKNOWN);
    }
    s5p->format = format;

    // TODO only use when reading?
    // TODO determine compression?
    s5p->compress = press_init(compress);
    s5p->header = slow5_hdr_init(fp, format);

    return s5p;
}

void slow5_free(struct SLOW5File *slow5) {
    NULL_CHK(slow5);

    // Free version string
    if (slow5->header->version_str != NULL) {
        free(slow5->header->version_str);
        slow5->header->version_str = NULL;
    }

    // Free header data map
    for (uint32_t i = 0; i < slow5->header->num_read_groups; ++ i) {
        khash_t(s2s) *hdr_data = slow5->header->data[i];
        for (khint_t j = kh_begin(hdr_data); j < kh_end(hdr_data); ++ j) {
            if (kh_exist(hdr_data, j)) {
                if (i == 0) {
                    free((void *) kh_key(hdr_data, j)); // TODO avoid void *
                }
                free((void *) kh_val(hdr_data, j)); // TODO avoid void *
            }
        }
        kh_destroy(s2s, hdr_data);
    }
    free(slow5->header->data);

    // Free header
    free(slow5->header);
    slow5->header = NULL; // TODO test if header is actually set to NULL

    // Free reads
    kh_destroy(s2r, slow5->reads);
    slow5->reads = NULL;

    free(slow5);
}

/*
static inline void assert_f_has_str(char *buf, FILE *stream, const char *exp) {
    size_t len = strlen(exp);
    size_t ret = fread(buf, sizeof *buf, len, stream);
    assert(ret == len);
    buf[len] = '\0';
    assert(strcmp(buf, exp) == 0);
}
*/

static void slow5_read_hdr(struct SLOW5Header *header, enum SLOW5Format format, FILE *stream) {

    switch (format) {

        case FORMAT_ASCII: {

            // Buffer for file parsing
            char *buf = malloc(SLOW5_HEADER_DATA_BUF_CAP * sizeof *buf);
            MALLOC_CHK(buf);

            // 1st line
            assert(fgets(buf, SLOW5_HEADER_BUF_CAP, stream) != NULL);
            assert(buf[strlen(buf) - 1] == '\n');
            buf[strlen(buf) - 1] = '\0'; // Remove newline for later format parsing
            // "#file_format"
            char *tok = strtok_solo(buf, SEP);
            assert(strcmp(tok, FILE_FORMAT_HEADER_ID) == 0);
            // Parse format name
            tok = strtok_solo(NULL, SEP);
            header->format = str_get_slow5_format(tok);
            assert(header->format != FORMAT_NONE);
            assert(format == header->format);

            // 2nd line
            assert(fgets(buf, SLOW5_HEADER_BUF_CAP, stream) != NULL);
            assert(buf[strlen(buf) - 1] == '\n');
            buf[strlen(buf) - 1] = '\0'; // Remove newline for later parsing
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
            assert(fgets(buf, SLOW5_HEADER_BUF_CAP, stream) != NULL);
            assert(buf[strlen(buf) - 1] == '\n');
            buf[strlen(buf) - 1] = '\0'; // Remove newline for later parsing
            // "#num_read_groups"
            tok = strtok_solo(buf, SEP);
            assert(strcmp(tok, NUM_GROUPS_HEADER_ID) == 0);
            // Parse num read groups
            tok = strtok_solo(NULL, SEP);
            header->num_read_groups = ato_uint32(tok);

            // Header data
            header->data = malloc(header->num_read_groups * sizeof *(header->data));
            MALLOC_CHK(header->data);
            assert(fgets(buf, SLOW5_HEADER_DATA_BUF_CAP, stream) != NULL);
            assert(buf[strlen(buf) - 1] == '\n');
            bool first_loop = true;
            // While the column header hasn't been reached
            while (strncmp(buf, ASCII_COLUMN_HEADER_MIN, strlen(ASCII_COLUMN_HEADER_MIN)) != 0) {
                // Ensure prefix is there
                assert(strncmp(buf, SLOW5_HEADER_DATA_PREFIX, strlen(SLOW5_HEADER_DATA_PREFIX)) == 0);

                char *shift = buf + strlen(SLOW5_HEADER_DATA_PREFIX); // Remove prefix
                shift[strlen(shift) - 1] = '\0'; // Remove newline for later parsing

                char *attr = strdup(strtok_solo(shift, SEP));
                char *val;

                uint32_t i = 0;
                bool first_rg = true;
                while ((val = strtok_solo(NULL, SEP)) != NULL) {
                    if (first_loop) {
                        header->data[i] = kh_init(s2s);
                        NULL_CHK(header->data[i]);
                    }

                    // Set key
                    int absent = 0;
                    khint_t pos;
                    if (first_rg) {
                        pos = kh_put(s2s, header->data[i], attr, &absent);
                        first_rg = false;
                    } else {
                        pos = kh_put(s2s, header->data[i], attr, &absent);
                    }
                    assert(absent != -1);

                    // Set value
                    kh_val(header->data[i], pos) = strdup(val);

                    ++ i;
                    printf("%s -> %s\n", attr, val); // TESTING
                }
                // Ensure that read group number of entries are read
                assert(i == header->num_read_groups);

                assert(fgets(buf, SLOW5_HEADER_DATA_BUF_CAP, stream) != NULL);
                assert(buf[strlen(buf) - 1] == '\n');

                first_loop = false; // TODO minor but is this better; if (first) { ... } ?
            }

            free(buf);
        } break;

        case FORMAT_BINARY: // TODO
            break;
    }
}

static void slow5_read_rec(khash_t(s2r) *reads, enum SLOW5Format format, FILE *stream) {

    switch (format) {

        case FORMAT_ASCII: {

            uint8_t i = 0;
            bool finished = false;
            while (!finished) {

                // Buffer for read parsing
                char *buf = NULL;
                size_t cap = 0;
                ssize_t len;

                struct SLOW5Read *read = calloc(1, sizeof *read);

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
}

void slow5_read(struct SLOW5 *slow5, enum SLOW5Format format, FILE *stream) {
    NULL_CHK(slow5);
    NULL_CHK(stream);

    // Autodetect file type
    // TODO
    if (format == FORMAT_NONE) {
        //format = stream_get_slow5_format(stream);
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

void slow5_print_hdr(const struct SLOW5Header *hdr) {
    printf("format='%s'\n", slow5_format_get_str(hdr->format));
    printf("version_str='%s'\n", hdr->version_str);
    printf("version={%hu,%hu,%hu}\n",
            hdr->version.major,
            hdr->version.minor,
            hdr->version.patch);
    printf("num_rgs=%u\n", hdr->num_read_groups);
}

/*
struct SLOW5File *slow5_open_format(enum SLOW5Format format, const char *pathname, const char *mode) {
    struct SLOW5File *slow5 = NULL;

    if (pathname != NULL && mode != NULL) {
        FILE *fp = fopen(pathname, mode);

        // Try autodetect using extension if format is FORMAT_NONE
        if (format == FORMAT_NONE) {
            format = path_get_slow5_format(pathname);
        }

        slow5 = slow5_init(format, fp);
    }

    return slow5;
}

struct SLOW5File *slow5_open(const char *pathname, const char *mode) {
    return slow5_open_format(FORMAT_NONE, pathname, mode);
}


// TODO Return type of fclose is int but we want to use a specific size int?
int slow5_close(struct SLOW5File *slow5) {
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

uint8_t slow5_write_hdr(struct SLOW5File *slow5) {
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

void slow5_write_hdr_data_attr(struct SLOW5File *slow5, const char *attr, const char *data) {

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
    struct SLOW5File *slow5_f;

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
     * SLOW5File *f_in = slow5_fopen("hi.slow5", "r");
     * SLOW5File *f_out = slow5_fopen("hi.blow5", "w");
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
