#include <zlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define GZIP_WBITS (16)

int main(int argc, char **argv) {

    char *fname = argv[1];
    FILE *f = fopen(fname, "r");

    int offset = atoi(argv[2]);
    int len = atoi(argv[3]);

    fseek(f, offset, SEEK_SET);
    unsigned char *buf = malloc(sizeof *buf * len);
    fread(buf, sizeof *buf, len, f);

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = len;
    strm.next_in = buf;

    uLong prev_sz = 0;
    uLong out_sz = 16328;
    unsigned char *out = (unsigned char *) malloc(sizeof *out * out_sz);

    int ret = inflateInit2(&strm, GZIP_WBITS);

    if (ret != Z_OK) {
        return ret;
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
                (void) inflateEnd(&strm);
                printf("uhh\n");
                return ret;
        }


        unsigned have = out_sz - strm.avail_out;

        /*
        fwrite(out + prev_sz, have, sizeof *out, stdout);
        printf("\n");
        */

        prev_sz += have;

        if (strm.avail_out == 0) {
            out = (unsigned char *) realloc(out, sizeof *out * (prev_sz + out_sz));
        }

    } while (strm.avail_out == 0);

    (void) inflateEnd(&strm);

    fwrite(out, prev_sz, sizeof *out, stdout);
    printf("\n");

    free(out);

    return ret;
}
