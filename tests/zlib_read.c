#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CHUNK (16384)

int main(int argc, char **argv) {

    const char *f_name = argv[1];
    long start = atol(argv[2]);
    long len = atol(argv[3]);
    z_stream strm;
    int ret;

    int fd = open(f_name, O_RDONLY);

    char *data = malloc(sizeof *data * len);
    if (pread(fd, data, len, start) == -1) {
        printf("error pread\n");
        return EXIT_FAILURE;
    }
    close(fd);

    fwrite(data, 1, len, stdout);
    printf("\n");

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = len;
    strm.next_in = data;

    inflateInit2(&strm, 15 | 16);

    uLong out_sz = CHUNK;
    char *out = malloc(sizeof *out * out_sz);

    do {
        strm.avail_out = out_sz;
        strm.next_out = out;

        ret = inflate(&strm, Z_NO_FLUSH);

        switch (ret) {
            case Z_MEM_ERROR:
                fprintf(stderr, "mem error\n");
                break;
            case Z_BUF_ERROR:
                fprintf(stderr, "buf error\n");
                break;
            case Z_DATA_ERROR:
                fprintf(stderr, "data error\n");
                break;
            case Z_OK:
                fprintf(stderr, "ok\n");
                break;
            case Z_STREAM_END:
                fprintf(stderr, "stream end\n");
                break;
        }

        unsigned have = CHUNK - strm.avail_out;
        fwrite(out, 1, have, stdout);

    } while (strm.avail_out == 0);

    free(data);
    inflateEnd(&strm);
    
    free(out);

    if (ret == Z_OK || ret == Z_STREAM_END) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
