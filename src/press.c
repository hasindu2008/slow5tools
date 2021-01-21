#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "press.h"
#include "error.h"

// Init compression stream
struct Press *press_init(enum PressMethod press_method) {

    struct Press *press = NULL;

    press = malloc(sizeof *press);
    press->press_method = press_method;

    switch (press_method) {

        case COMPRESS_NONE:
            press->press_stream = NULL;
            break;

        case COMPRESS_GZIP: {
            union PressStream *press_stream;
            struct GzipStream *gzip_stream;
            int ret;

            gzip_stream = malloc(sizeof *gzip_stream);
            gzip_stream->strm.zalloc = Z_NULL;
            gzip_stream->strm.zfree = Z_NULL;
            gzip_stream->strm.opaque = Z_NULL;
            gzip_stream->flush = Z_NO_FLUSH;

            ret = deflateInit2(&(gzip_stream->strm),
                    Z_DEFAULT_COMPRESSION,
                    Z_DEFLATED,
                    MAX_WBITS | GZIP_WBITS, // Gzip compatible compression
                    Z_MEM_DEFAULT,
                    Z_DEFAULT_STRATEGY);

            // Error occurred
            if (ret != Z_OK) {
                free(gzip_stream);

            } else {
                press_stream = malloc(sizeof *press_stream);

                press_stream->gzip_stream = gzip_stream;
                press->press_stream = press_stream;
            }

        } break;
    }

    return press;
}

// free compression stream
void press_free(struct Press **press) {

    if (press != NULL && *press != NULL) {

        switch ((*press)->press_method) {

            case COMPRESS_NONE:
                break;

            case COMPRESS_GZIP: {
                union PressStream *press_stream;
                struct GzipStream *gzip_stream;
                int ret;

                press_stream = (*press)->press_stream;
                gzip_stream = press_stream->gzip_stream;

                ret = deflateEnd(&(gzip_stream->strm));
                // Error occurred
                if (ret != Z_OK) {
                    // TODO
                }

                free(gzip_stream);
                press_stream->gzip_stream = NULL;
                free(press_stream);
                (*press)->press_stream = NULL;
            } break;
        }

        free(*press);
        *press = NULL;
    }
}


int fprintf_press(struct Press *press, FILE *stream, const char *format, ...) {
    int ret;

    if (press != NULL) {
        va_list ap;
        va_start(ap, format);

        switch (press->press_method) {

            case COMPRESS_NONE:
                ret = vfprintf(stream, format, ap);
                break;

            case COMPRESS_GZIP: {
                union PressStream *press_stream = press->press_stream;

                if (press_stream != NULL) {
                    ret = vfprintf_gzip(press_stream->gzip_stream, stream, format, ap);
                }

            } break;
        }

        va_end(ap);
    }

    return ret;
}

int vfprintf_gzip(struct GzipStream *gzip_stream, FILE *stream, const char *format, va_list ap) {

    int ret = -1; // TODO change this to a proper error code

    if (gzip_stream != NULL) {
        char *buf;

        vasprintf(&buf, format, ap);
        ret = z_deflate_write(&(gzip_stream->strm), buf, strlen(buf), stream, gzip_stream->flush); // Can also return -1 I think
        if (gzip_stream->flush == Z_FINISH) {
            gzip_stream->flush = Z_NO_FLUSH;
        }

        free(buf);
    }

    return ret;
}


size_t fwrite_press(struct Press *press, const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t ret;

    if (press != NULL) {
        switch (press->press_method) {

            case COMPRESS_NONE:
                ret = fwrite(ptr, size, nmemb, stream);
                break;

            case COMPRESS_GZIP: {
                union PressStream *press_stream = press->press_stream;

                if (press_stream != NULL) {
                    ret = fwrite_gzip(press_stream->gzip_stream, ptr, size, nmemb, stream);
                }

            } break;
        }
    }

    return ret;
}

size_t fwrite_gzip(struct GzipStream *gzip_stream, const void *ptr, size_t size, size_t nmemb, FILE *stream) {

    int ret = -1; // TODO change this to a proper error code

    if (gzip_stream != NULL) {
        ret = z_deflate_write(&(gzip_stream->strm), ptr, size * nmemb, stream, gzip_stream->flush); // Can also return -1 I think
        if (gzip_stream->flush == Z_FINISH) {
            gzip_stream->flush = Z_NO_FLUSH;
        }
    }

    return ret;
}


void press_footer_next(struct Press *press) {

    if (press != NULL && press->press_stream != NULL) {
        union PressStream *press_stream = press->press_stream;

        switch (press->press_method) {

            case COMPRESS_NONE:
                break;
            case COMPRESS_GZIP: {

                struct GzipStream *gzip_stream = press_stream->gzip_stream;

                if (gzip_stream != NULL) {
                    gzip_stream->flush = Z_FINISH;
                }

            } break;
        }
    }
}


// From https://stackoverflow.com/questions/3774417/sprintf-with-automatic-memory-allocation
int vasprintf(char **strp, const char *fmt, va_list ap) {
    va_list ap1;
    size_t size;
    char *buffer;

    va_copy(ap1, ap);
    size = vsnprintf(NULL, 0, fmt, ap1) + 1;
    va_end(ap1);
    buffer = calloc(1, size);

    if (!buffer)
        return -1;

    *strp = buffer;

    return vsnprintf(buffer, size, fmt, ap);
}

// From https://stackoverflow.com/questions/3774417/sprintf-with-automatic-memory-allocation
int asprintf(char **strp, const char *fmt, ...) {
    int error;
    va_list ap;

    va_start(ap, fmt);
    error = vasprintf(strp, fmt, ap);
    va_end(ap);

    return error;
}



/* Decompress a zlib-compressed string
 *
 * @param       compressed string
 * @param       ptr to size of compressed string, updated to size of returned malloced string
 * @return      malloced string
 */
unsigned char *z_inflate_buf(const char *comp_str, size_t *n) {

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = *n;
    strm.next_in = (Bytef *) comp_str;

    *n = 0;

    uLong prev_sz = 0;
    uLong out_sz = 16328;
    unsigned char *out = (unsigned char *) malloc(sizeof *out * out_sz);

    int ret = inflateInit2(&strm, GZIP_WBITS);

    if (ret != Z_OK) {
        free(out);
        return NULL;
    }

    do {
        strm.avail_out = out_sz;
        strm.next_out = out + prev_sz;

        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);

        switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                free(out);
                (void) inflateEnd(&strm);
                return NULL;
        }


        unsigned have = out_sz - strm.avail_out;
        prev_sz += have;

        if (strm.avail_out == 0) {
            out = (unsigned char *) realloc(out, sizeof *out * (prev_sz + out_sz));
        }

    } while (strm.avail_out == 0);

    *n = prev_sz;
    (void) inflateEnd(&strm);

    return out;
}

/* Gzip compress some data to a file.
 * Doesn't close the stream or file.
 *
 * @param       zlib stream
 * @param       data to write
 * @param       size of data
 * @param       file ptr
 * @param       flush input to deflate() function
 * @return      status code from deflate(), Z_OK on success
 */
int z_deflate_write(z_streamp strmp, const void *ptr, uLong size, FILE *f_out, int flush) {
    int ret = Z_OK;

    strmp->avail_in = size;
    strmp->next_in = (Bytef *) ptr;

    uLong out_sz = Z_OUT_CHUNK;
    unsigned char *out = (unsigned char *) malloc(sizeof *out * out_sz);

    do {
        strmp->avail_out = out_sz;
        strmp->next_out = out;

        ret = deflate(strmp, flush);
        if (ret == Z_STREAM_ERROR) {
            ERROR("deflate failed\n%s", ""); // testing
            return ret;
        }

        unsigned have = out_sz - strmp->avail_out;
        if (fwrite(out, sizeof *out, have, f_out) != have || ferror(f_out)) {
            ERROR("fwrite\n%s", ""); // testing
            return Z_ERRNO;
        }

    } while (strmp->avail_out == 0);

    free(out);
    out = NULL;

    // If still input to deflate
    if (strmp->avail_in != 0) {
        ERROR("still more input to deflate\n%s", ""); // testing
        return Z_ERRNO;
    }

    return ret;
}
