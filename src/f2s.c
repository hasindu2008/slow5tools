// Sasha Jenner
// TODO add --version flag?

#include "error.h"
#include <unistd.h> 
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "slow5.h"

#define USAGE_MSG "Usage: %s [OPTION]... [FAST5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Convert fast5 file(s) to slow5.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -d, --max-depth=[NUM]\n" \
    "        Set the maximum depth to search directories for fast5 files.\n" \
    "        NUM must be a non-negative integer.\n" \
    "        Default: No maximum depth.\n" \
    "\n" \
    "        E.g. NUM=1: Read the files within a specified directory but\n" \
    "        not those within subdirectories.\n" \
    "\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \
    "\n" \
    "    -o, --output=[SLOW5_FILE]\n" \
    "        Output slow5 contents to SLOW5_FILE.\n" \
    "        Default: Stdout.\n" \

// adapted from https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file 
bool is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == -1) {
        ERROR("Stat failed to retrive file information%s", "");
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}

void recurse_dir(const char *f_path, FILE *f_out) {
    if (is_dir(f_path)) {

    } else {
        // Open FAST5 and convert to SLOW5 into f_out
    }
}

int f2s_main(int argc, char **argv, struct program_meta *meta) {

    // Debug: print arguments
    if (meta != NULL && meta->debug) {
        if (meta->verbose) {
            VERBOSE("printing the arguments given%s","");
        }

        fprintf(stderr, DEBUG_PREFIX "argv=[",
                __FILE__, __func__, __LINE__);
        for (int i = 0; i < argc; ++ i) {
            fprintf(stderr, "\"%s\"", argv[i]);
            if (i == argc - 1) {
                fprintf(stderr, "]");
            } else {
                fprintf(stderr, ", ");
            }
        }
        fprintf(stderr, NO_COLOUR);
    }

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, USAGE_MSG HELP_SMALL_MSG, argv[0], argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
        {"max-depth", required_argument, NULL, 'd' },
        {"help", no_argument, NULL, 'h' },
        {"output", required_argument, NULL, 'o' },
        {NULL, 0, NULL, 0 }
    };

    // Default options
    long max_depth = -1;
    FILE *f_out = stdout;

    // Input arguments
    char *arg_max_depth = NULL;
    char *arg_fname_out = NULL;

    char opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "d:ho:", long_opts, NULL)) != -1) {

        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 'd':
                arg_max_depth = optarg;
                break;
            case 'h':
                if (meta->verbose) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                return EXIT_SUCCESS;
            case 'o':
                arg_fname_out = optarg; 
                break; 
            default: // case '?' 
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    // Parse maximum depth argument
    if (arg_max_depth != NULL) {

        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing maximum depth%s","");
        }

        // Check it is a number
        
        // Cannot be empty 
        size_t arg_len = strlen(arg_max_depth);
        if (arg_len == 0) {
            MESSAGE(stderr, "invalid max depth -- '%s'", arg_max_depth);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }

        for (size_t i = 0; i < arg_len; ++ i) {
            // Not a digit and first char is not a '+'
            if (!isdigit((unsigned char) arg_max_depth[i]) && 
                    !(i == 0 && arg_max_depth[i] == '+')) {
                MESSAGE(stderr, "invalid max depth -- '%s'", arg_max_depth);
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);

                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }

        // Parse argument
        max_depth = strtol(arg_max_depth, NULL, 10);
        // Check for overflow
        if (errno == ERANGE) {
            WARNING("Overflow of max depth '%s'. Setting to %ld instead.", 
                    arg_max_depth, max_depth);
        }
    }

    // Parse output argument
    if (arg_fname_out != NULL) { 

        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing output filename%s","");
        }

        // Create new file or
        // Truncate existing file
        FILE *new_file = fopen(arg_fname_out, "w");

        // An error occured
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.", 
                  arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
            
        } else {
            f_out = new_file;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }


    // Do the converting
    fprintf(f_out, SLOW5_HEADER);

    for (int i = optind; i < argc; ++ i) {
        // Recursive way
        recurse_dir(argv[i], f_out);

        // TODO iterative way
    }


    if (f_out != stdout) {
        // Close output file
        if (fclose(f_out) == EOF) {
            ERROR("File '%s' failed on closing - %s.",
                  arg_fname_out, strerror(errno));
        } 
    }

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
