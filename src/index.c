/**
 * @file index.c
 * @brief index a SLOW5 file
 * @author Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 27/02/2021
 */
#include <stdio.h>
#include <getopt.h>

#include <slow5/slow5.h>
#include "error.h"
#include "cmd.h"
#include "misc.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5|BLOW5_FILE]\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Create a slow5 or blow5 index file.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \

extern int slow5tools_verbosity_level;

int index_main(int argc, char **argv, struct program_meta *meta) {

    // Debug: print arguments
    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
        {"help", no_argument, NULL, 'h' },
        {NULL, 0, NULL, 0 }
    };

    int opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "h", long_opts, NULL)) != -1) {

        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);

        switch (opt) {
            case 'h':
                DEBUG("displaying large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        ERROR("missing slow5 or blow5 file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;

    // Check for only one file
    } else if (optind < argc - 1) {
        ERROR("too many files given%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    char *f_in_name = argv[optind];
    slow5_file_t *file=slow5_open(f_in_name,"r");
    F_CHK(file,f_in_name);

    if (slow5_idx_create(file) != 0) {
        fprintf(stderr, "Error running slow5idx_build on %s\n",
                f_in_name);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    slow5_close(file);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
