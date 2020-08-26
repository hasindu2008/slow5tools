#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string>

int main(int argc, char **argv) {
    std::string f_name = argv[1];
    auto dir = opendir(f_name.c_str());
    if(not dir) {
        return EXIT_FAILURE;
    }
    closedir(dir);
    return EXIT_SUCCESS;
}
