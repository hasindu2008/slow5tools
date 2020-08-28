#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

//from nanopolish idnex
static std::vector< std::string > list_directory(const char *file_name)
{
    std::vector< std::string > res;
    DIR* dir;
    struct dirent *ent;

    dir = opendir(file_name);
    if(not dir) {
        return res;
    }
    while((ent = readdir(dir)) != nullptr) {
        res.push_back(ent->d_name);
    }
    closedir(dir);
    return res;
}

int main(int argc, char **argv) {

    auto list = list_directory(argv[1]);

    for (const auto& file : list) {
        printf("%s\n", file.c_str());
    }

    return EXIT_SUCCESS;
}
