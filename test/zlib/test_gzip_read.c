#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>

// 3 args
// 1: filename
// 2: start byte
// 3: num bytes to read

int main(int argc, char **argv) {

    const char *f_name = argv[1];
    long start = atol(argv[2]);
    long len = atol(argv[3]);

    gzFile f_in = gzopen(f_name, "r");

    gzseek(f_in, start, SEEK_SET);

    char *buf = malloc((len + 1) * sizeof *buf); // +1 for '\0'
    gzread(f_in, buf, len * sizeof *buf);

    fwrite(buf, len, 1, stdout);
    
    free(buf);
    buf = NULL;

    gzclose_r(f_in);

    return 0;
}
