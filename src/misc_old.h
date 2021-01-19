#ifdef __cplusplus
extern "C" {
#endif

#ifndef MISC_H
#define MISC_H

#include <zlib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_NUM_THREADS (4)

#define READ_ID_INIT_CAPACITY (128)
#define READ_ID_BATCH_CAPACITY (4096)

#define GZIP_WBITS (16)

unsigned char *z_inflate_buf(const char *comp_str, size_t *n);

#endif

#ifdef __cplusplus
}
#endif