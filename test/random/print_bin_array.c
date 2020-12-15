#include <stdio.h>
#include <stdlib.h>

int main(void) {

    float *x = malloc(10 * sizeof *x);
    for (int i = 0; i < 10; ++ i) {
        x[i] = 1162.0f + i;
    }

    int *y = malloc(10 * sizeof *y);
    for (int i = 0; i < 10; ++ i) {
        y[i] = (int) x[i];
    }

    fwrite(y, sizeof *y, 10, stdout); 

    free(x);
    free(y);

    return 0;
}
