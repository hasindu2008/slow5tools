#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {

    int ret;
    z_stream strm;

    char inp[] = "Hello, world!\n";
    uInt inp_sz = strlen(inp) + 1; // +1 for '\0'

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = inp_sz;
    strm.next_in = inp;

    ret = deflateInit2(&strm, 
            Z_DEFAULT_COMPRESSION, 
            Z_DEFLATED,
            MAX_WBITS | 16, // Gzip compatible compression
            8, // Default memory
            Z_DEFAULT_STRATEGY);

    if (ret != Z_OK) {
        fprintf(stderr, "failed: deflateInit2\n");
        return EXIT_FAILURE;
    }

    uLong out_sz = deflateBound(&strm, inp_sz);
    char out[out_sz];

    strm.avail_out = out_sz;
    strm.next_out = out;

    ret = deflate(&strm, Z_FINISH);

    if (ret != Z_STREAM_END) {
        fprintf(stderr, "failed: deflate\n");
        return EXIT_FAILURE;
    }

    fwrite(out, sizeof *out, strm.total_out, stdout);

    deflateEnd(&strm);

    return EXIT_SUCCESS;
}
