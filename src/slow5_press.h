#ifndef SLOW5_PRESS_H
#define SLOW5_PRESS_H

#include <zlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "slow5_misc.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compression methods
enum press_method {
    COMPRESS_NONE,
    COMPRESS_GZIP
};
typedef uint8_t press_method_t;


// Gzip related definitions
// To obtain gzip deflation and inflation set windowBits=MAX_WBITS|GZIP_WBITS
// Used in deflateInit2 and inflateInit2 from zlib

#define NUM_MAGIC_BYTES (2)
static const int GZIP_MAGIC_NUM[] = { 0x1f, 0x8b };

#define GZIP_WBITS (16)
#define Z_MEM_DEFAULT (8)
#define Z_OUT_CHUNK (16384) // 2^14

// Gzip stream
struct gzip_stream {
    z_stream strm_inflate;
    z_stream strm_deflate;
    int flush;
};


// Compression streams
union press_stream {
    struct gzip_stream *gzip;
};

// Compression object
struct press {
    press_method_t method;
    union press_stream *stream;
};
typedef struct press press_t;

/* --- Init / free press structure --- */
struct press *press_init(press_method_t method);
void press_free(struct press *comp);

/* --- Compress / decompress a ptr to some memory --- */
void *ptr_compress(struct press *comp, const void *ptr, size_t count, size_t *n);
static inline void *str_compress(struct press *comp, const char *str, size_t *n) {
    return ptr_compress(comp, str, strlen(str) + 1, n); // Include '\0'
}
void *ptr_depress(struct press *comp, const void *ptr, size_t count, size_t *n);
void *ptr_depress_multi(press_method_t method, const void *ptr, size_t count, size_t *n);

/* --- Compress / decompress a ptr to some file --- */
size_t fwrite_compress(struct press *comp, const void *ptr, size_t size, size_t nmemb, FILE *fp);
size_t fwrite_depress(struct press *comp, const void *ptr, size_t size, size_t nmemb, FILE *fp); // TODO
static inline size_t print_compress(struct press *comp, const void *ptr, size_t size, size_t nmemb) {
    return fwrite_compress(comp, ptr, size, nmemb, stdout);
}
static inline size_t print_depress(struct press *comp, const void *ptr, size_t size, size_t nmemb) {
    return fwrite_depress(comp, ptr, size, nmemb, stdout);
}
static inline size_t fwrite_str_compress(struct press *comp, const char *str, FILE *fp) {
    return fwrite_compress(comp, str, sizeof *str, strlen(str), fp); // Don't include '\0'
}
static inline size_t print_str_compress(struct press *comp, const char *str) {
    return fwrite_str_compress(comp, str, stdout);
}

/* --- Decompress to a ptr from some file --- */
void *fread_depress(struct press *comp, size_t count, FILE *fp, size_t *n);
void *pread_depress(struct press *comp, int fd, size_t count, off_t offset, size_t *n);
void *pread_depress_multi(press_method_t method, int fd, size_t count, off_t offset, size_t *n);

/* --- Compress with format string to some file --- */
int fprintf_compress(struct press *comp, FILE *fp, const char *format, ...);
int printf_compress(struct press *comp, const char *format, ...);

/* --- Write compression footer on immediate next compression call --- */
void compress_footer_next(struct press *comp);

#ifdef __cplusplus
}
#endif

#endif
