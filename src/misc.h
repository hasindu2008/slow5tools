#ifndef MISC_H
#define MISC_H

#include <zlib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define GZIP_WBITS (16)

unsigned char *z_inflate_buf(const char *comp_str, size_t *n);

#endif
