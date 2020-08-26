#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    char *f_name = argv[1];

    struct stat f_stat;
    if (stat(f_name, &f_stat) == -1) {
        return EXIT_FAILURE;
    }

    if (S_ISDIR(f_stat.st_mode)) {
        return EXIT_SUCCESS; 
    } else {
        return EXIT_FAILURE;
    }
}
