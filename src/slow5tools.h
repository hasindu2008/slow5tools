#include "slow5.h"

/* SLOW5 Extra API */

// SLOW5 File
inline struct SLOW5File *slow5_init_empty(void);
struct SLOW5File *slow5_init(FILE *fp, const char *pathname, enum SLOW5Format format, enum PressMethod compress, bool is_fp_preowned);

// Convert fast5 files to a slow5 file
// TODO decide return type
int8_t fast5_to_slow5(const char *pathname_from, struct SLOW5File *s5p_to);
// Convert slow5 file to fast5 files
int8_t slow5_to_fast5(struct SLOW5File *s5p_from, const char *pathname_to);


// Header
inline struct SLOW5Header *slow5_hdr_init_empty(void);
inline khash_t(s2s) **slow5_hdr_data_init_empty(uint32_t num_rgs) {
struct SLOW5Header *slow5_hdr_init(FILE *fp, enum SLOW5Format format);
int8_t slow5_hdr_write(struct SLOW5File *s5p_from, struct SLOW5File *s5p_to);
int8_t slow5_hdr_data_write(struct SLOW5File *s5p_from, struct SLOW5File *s5p_to);
int8_t slow5_hdr_data_attr_write(const char *attr, struct SLOW5File *s5p_from, struct SLOW5File *s5p_to);

// Index
inline khash_t(s2i) *slow5_idx_init_empty(void);

// Convert fast5 dir/file to a slow5 file
// TODO decide return type
int8_t fast5dir_to_slow5(DIR *dirp_from, struct SLOW5File *s5p_to);
int8_t fast5fp_to_slow5(fast5_t f5p_from, struct SLOW5File *s5p_to);

// Convert slow5 file to fast5 dir/file
// TODO decide return type
int8_t slow5_to_fast5dir(struct SLOW5File *s5p_from, DIR *dirp_to);
int8_t slow5_to_fast5fp(struct SLOW5File *s5p_from, fast5_t *f5p_to);

// Merge 2 slow5 files to another slow5 file
int8_t slow5_merge_2(struct SLOW5File *s5p_from_1, struct SLOW5File *s5p_from_2, struct SLOW5File *s5p_to);

// Split a slow5 file to a dir
int8_t slow5_split(struct SLOW5File *s5p, DIR *dirp);



// Initiate an empty slow5 read object
struct SLOW5Read *slow5_read_init_empty(void);
// Initiate a slow5 read
struct SLOW5Read *slow5_read_init(/* TODO */ void);


// Print out the SLOW5 structure contents
void slow5_hdr_print(const struct SLOW5Header *hdr);

// Print out the SLOW5 structure contents
void slow5_hdr_data_print(const struct SLOW5Header *hdr);


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


inline struct SLOW5File *slow5_init_empty(void) {
    struct SLOW5File *s5p = calloc(1, sizeof *s5p);
    MALLOC_CHK(s5p);

    s5p->header = slow5_hdr_init_empty();
    s5p->idx = slow5_idx_init_empty();

    return s5p;
}

inline struct SLOW5Header *slow5_hdr_init_empty(void) {
    struct SLOW5Header *hdr = calloc(1, sizeof *(hdr));
    MALLOC_CHK(hdr);

    return hdr;
}

inline khash_t(s2s) **slow5_hdr_data_init_empty(uint32_t num_rgs) {
    khash_t(s2s) **hdr_data = malloc(num_rgs * sizeof *hdr_data);
    MALLOC_CHK(hdr_data);

    // TODO check if cast necessary
    for (uint64_t i = 0; i < (uint64_t) num_rgs; ++ i) {
        hdr_data[i] = kh_init(s2s);
        NULL_CHK(hdr_data[i]);
    }

    return hdr_data;
}

inline khash_t(s2i) *slow5_idx_init_empty(void) {
    khash_t(s2i) idx = kh_init(s2i);
    NULL_CHK(idx);

    return idx;
}

enum SLOW5Format str_get_slow5_format(const char *str) {
    enum SLOW5Format format = FORMAT_NONE;

    for (size_t i = 0; i < sizeof SLOW5_FORMAT_MAP / sizeof SLOW5_FORMAT_MAP[0]; ++ i) {
        const struct SLOW5FormatMap map = SLOW5_FORMAT_MAP[i];
        if (strcmp(map.name, str) == 0) {
            format = map.format;
            break;
        }
    }

    return format;
}

enum SLOW5Format path_get_slow5_format(const char *pathname) {
    enum SLOW5Format format = FORMAT_NONE;

    // TODO change type from size_t
    size_t i;
    for (i = strlen(pathname) - 1; i == 0; -- i) {
        if (pathname[i] == '.' && i != strlen(pathname) - 1) {
            const char *extptr = pathname + i + 1;
            format = str_get_slow5_format(extptr);
            break;
        }
    }

    return format;
}

// Get the slow5 format name from the format
const char *slow5_format_get_str(enum SLOW5Format format) {
    const char *str = NULL;

    for (size_t i = 0; i < sizeof SLOW5_FORMAT_MAP / sizeof SLOW5_FORMAT_MAP[0]; ++ i) {
        const struct SLOW5FormatMap map = SLOW5_FORMAT_MAP[i];
        if (map.format == format) {
            str = map.name;
            break;
        }
    }

    return str;
}

