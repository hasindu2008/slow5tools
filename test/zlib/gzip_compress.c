#include <zlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {

    char inp[] = "Hello, world!\n";
    uLong inp_sz = strlen(inp) + 1;

    uLong out_sz = compressBound(inp_sz);
    char *out = malloc(sizeof *out * out_sz);

    compress((Bytef *) out, &out_sz,
             (Bytef *) inp, inp_sz);

    printf("%lu\n", out_sz);

    fwrite(out, out_sz, 1, stdout);

    free(out);

    return 0;
}
