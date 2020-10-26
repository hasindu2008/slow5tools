#include <stdio.h>

int main(void) {
    int magic[2];
    printf("magic: %zu\n*magic: %zu\n", sizeof magic, sizeof *magic);

    return 0;
}
