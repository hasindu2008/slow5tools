#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#define CHUNK (16384)
#define GZIP_WBITS (16)

/* Input
 * 1: filepath
 * 2: offset
 * 3: length
 *
 * Reads in buffer,
 * decompresses to buffer
 * prints out buffer
 */
int main(int argc, char **argv) {

    const char *f_name = argv[1];
    long offset = atol(argv[2]);
    long size = atol(argv[3]);

    z_stream strm;
    int ret;
    size_t n_cur = 0;
    uint8_t *out = NULL;

    int fd;
    if ((fd = open(f_name, O_RDONLY)) == -1) {
        printf("open error\n");
        return EXIT_FAILURE;
    }

    uint8_t *data = malloc(size);
    if (data == NULL) {
        printf("malloc data error\n");
        return EXIT_FAILURE;
    }
    ssize_t read;
    if ((read = pread(fd, data, size, offset)) != size) {
        printf("pread error: returned %zu\n", read);
        return EXIT_FAILURE;
    }
    close(fd);

    /*
    fwrite(data, 1, len, stdout);
    printf("\n");
    */

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = size;
    strm.next_in = data;

    if (inflateInit2(&strm, GZIP_WBITS) != Z_OK) {
        printf("inflate init error\n");
        return EXIT_FAILURE;
    }

    do {
        out = (uint8_t *) realloc(out, n_cur + CHUNK);
        if (out == NULL) {
            printf("realloc out error\n");
            return EXIT_FAILURE;
        }

        strm.avail_out = CHUNK;
        strm.next_out = out + n_cur;

        ret = inflate(&strm, Z_NO_FLUSH);

        switch (ret) {
            case Z_MEM_ERROR:
                fprintf(stderr, "zlib mem error\n");
                break;
            case Z_BUF_ERROR:
                fprintf(stderr, "zlib buf error\n");
                break;
            case Z_DATA_ERROR:
                fprintf(stderr, "zlib data error\n");
                break;
            case Z_OK:
                fprintf(stderr, "zlib ok\n");
                break;
            case Z_STREAM_END:
                fprintf(stderr, "zlib stream end\n");
                break;
        }

        n_cur += CHUNK - strm.avail_out;

    } while (strm.avail_out == 0);

    fwrite(out, 1, n_cur, stdout);

    free(out);
    free(data);

    if (ret == Z_OK || ret == Z_STREAM_END) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
