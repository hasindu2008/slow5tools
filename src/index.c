#include "slow5.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5|BLOW5_FILE]\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Create a slow5 or blow5 index file.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \

int index_main(int argc, char **argv, struct program_meta *meta) {

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

    // Check for only one file
    } else if (optind < argc - 1) {
        MESSAGE(stderr, "too many files given%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    char *f_in_name = argv[optind];

    // Build index file TODO make blow5 indexable
    if (slow5idx_build(f_in_name) != 0) {
        fprintf(stderr, "Error running slow5idx_build on %s\n",
                f_in_name);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
