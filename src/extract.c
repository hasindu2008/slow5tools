#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "error.h"
#include "slow5.h"
#include "slow5idx.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5|BLOW5_FILE] [READ_ID]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Display the read entry for each specified read id from a slow5 or blow5 file.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \

int extract_main(int argc, char **argv, struct program_meta *meta) {

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
        {"help", no_argument, NULL, 'h' },
        {NULL, 0, NULL, 0 }
    };

    char opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "h", long_opts, NULL)) != -1) {

        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 'h':
                if (meta->verbose) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                return EXIT_SUCCESS;
            default: // case '?' 
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing slow5 or blow5 file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;

    } else if (optind == argc - 1) {
        MESSAGE(stderr, "missing read id(s)%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    char *f_in_name = argv[optind];
    slow5idx_t *index_f = slow5idx_load(f_in_name); // TODO add blow5 capabilities

    if (index_f == NULL) {
        // TODO change these to MESSAGE?
        fprintf(stderr, "Error loading index file for %s\n",
                f_in_name);

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    for (int i = optind + 1; i < argc; ++ i){

        int len = 0;
        fprintf(stderr, "Fetching %s\n", argv[i]);
        char *record = slow5idx_fetch(index_f, argv[i], &len);

        if (record == NULL || len < 0) {
            fprintf(stderr, "Error locating %s\n", argv[i]);

            slow5idx_destroy(index_f);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
            
        }

        printf("%s", record);
        free(record);
    }

    slow5idx_destroy(index_f);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
