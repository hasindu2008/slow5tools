#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

int main(void) {

    float *x = malloc(sizeof *x);
    *x = 32.0f;

    int *y = malloc(sizeof *y);
    *y = (int) *x;

    fwrite(y, sizeof *y, 1, stdout); 
    //write(STDOUT_FILENO, y, sizeof *y);

    free(x);
    free(y);

    return 0;
}
