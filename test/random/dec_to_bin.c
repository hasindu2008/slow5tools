#include <stdio.h>

int main(void) {
    int x = 1287;

    fwrite(&x, sizeof x, 1, stdout);

    return 0;
}
