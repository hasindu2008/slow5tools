#ifndef PRESS_H
#define PRESS_H

#include <zlib.h>
#include <stdio.h>
#include <stdint.h>

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
    z_stream strm;
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


// Init and free compression stream
struct press *press_init(press_method_t method);
void press_free(struct press *compress);

// fwrite but with compression
size_t fwrite_press(struct press *compress, const void *ptr, size_t size, size_t nmemb, FILE *fp);
// fwrite but with gzip compression
size_t fwrite_gzip(struct gzip_stream *gzip, const void *ptr, size_t size, size_t nmemb, FILE *fp);

// fprintf but with compression
int fprintf_press(struct press *compress, FILE *fp, const char *format, ...);
// vfprintf but with gzip compression
int vfprintf_gzip(struct gzip_stream *gzip, FILE *fp, const char *format, va_list ap);

// Write the compression footer on the immediate next call to fprintf_press
void press_footer_next(struct press *compress);

// sprintf and vsprintf but dynamically allocates strp memory
int asprintf_mine(char **strp, const char *fmt, ...);
int vasprintf_mine(char **strp, const char *fmt, va_list ap);

// Decompress a zlib compressed string of size n
// Return it and set n to the new size
unsigned char *z_inflate_buf(const char *comp_str, size_t *n);
// Write zlib compressed ptr to f_out
int z_deflate_write(z_stream *strmp, const void *ptr, uLong size, FILE *f_out, int flush);

#endif
