#include "slow5.h"

#define USAGE_MSG "Usage: %s [OPTION]... SLOW5|BLOW5_FILE [READ_ID]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Display the read entry for each specified read id from a slow5 or blow5 file.\n" \
    "With no READ_ID, read standard input for newline separated read ids.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \

bool fetch_record(slow5idx_t *index_f, const char *read_id, 
        char **argv, struct program_meta *meta) {

    bool success = true;

    int len = 0;
    fprintf(stderr, "Fetching %s\n", read_id);
    char *record = slow5idx_fetch(index_f, read_id, &len);

    if (record == NULL || len < 0) {
        fprintf(stderr, "Error locating %s\n", read_id);
        success = false;

    } else {
        fwrite(record, len, 1, stdout);
        free(record);
    }

    return success;
}

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
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
        {"help", no_argument, NULL, 'h' },
        {NULL, 0, NULL, 0 }
    };

    bool read_stdin = false;

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
        read_stdin = true;
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

    bool ret = EXIT_SUCCESS;

    if (read_stdin) {
        size_t cap_ids = 10;
        size_t num_ids = 0;
        char **ids = (char **) malloc(cap_ids * sizeof *ids);

        char *buf = NULL;
        size_t cap_buf = 0;
        while (getline(&buf, &cap_buf, stdin) != -1) {

            size_t len_buf = strlen(buf);
            char *curr_id = strndup(buf, len_buf);
            curr_id[len_buf - 1] = '\0'; // Removing '\n'

            if (num_ids >= cap_ids) { 
                // Double read id list capacity
                cap_ids *= 2;
                ids = (char **) realloc(ids, cap_ids * sizeof *ids);
            }
            ids[num_ids] = curr_id;
            ++ num_ids;

            // Reset for next line
            free(buf);
            buf = NULL;
            cap_buf = 0;
        }
        // Free even after failure
        free(buf);
        buf = NULL;

        for (size_t i = 0; i < num_ids; ++ i) {
            bool success = fetch_record(index_f, ids[i], argv, meta);
            if (!success) {
                ret = EXIT_FAILURE;
            }
        }

    } else {

        for (int i = optind + 1; i < argc; ++ i){
            bool success = fetch_record(index_f, argv[i], argv, meta);
            if (!success) {
                ret = EXIT_FAILURE;
            }
        }
    }

    slow5idx_destroy(index_f);

    EXIT_MSG(ret, argv, meta);
    return ret;
}
