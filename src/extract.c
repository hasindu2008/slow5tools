#include <getopt.h>
#include <stdio.h>

#include "slow5.h"
#include "thread.h"
#include "cmd.h"

#define DEFAULT_NUM_THREADS (4)

#define READ_ID_INIT_CAPACITY (128)
#define READ_ID_BATCH_CAPACITY (4096)

#define USAGE_MSG "Usage: %s [OPTION]... SLOW5|BLOW5_FILE [READ_ID]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Display the read entry for each specified read id from a slow5 file.\n" \
    "With no READ_ID, read from standard input newline separated read ids.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -t, --threads=[INT]    number of threads -- 4\n" \
    "    -h, --help             display this message and exit.\n" \

void work_per_single_read(core_t *core, db_t *db, int32_t i) {

    char *id = db->read_id[i];

    int len = 0;
    //fprintf(stderr, "Fetching %s\n", id); // TODO print here or during ordered loop later?
    slow5_rec_t *record=NULL;

    len = slow5_get(id,&record,core->fp);

    if (record == NULL || len < 0) {
        fprintf(stderr, "Error locating %s\n", id);
        ++ db->n_err;
    }

    db->read_record[i].buf = record;
    db->read_record[i].len = len;

    free(id);
}

bool fetch_record(slow5_file_t *fp, const char *read_id,
        char **argv, struct program_meta *meta) {

    bool success = true;

    int len = 0;
    fprintf(stderr, "Fetching %s\n", read_id);
    slow5_rec_t *record=NULL;

    len = slow5_get(read_id, &record,fp);

    if (record == NULL || len < 0) {
        fprintf(stderr, "Error locating %s\n", read_id);
        success = false;

    } else {
        slow5_rec_fwrite(stdout,record,fp->header->aux_meta,FORMAT_ASCII,NULL);
        slow5_rec_free(record);
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
        {"threads", required_argument, NULL, 't' },
        {"help", no_argument, NULL, 'h' },
        {NULL, 0, NULL, 0 }
    };

    bool read_stdin = false;

    // Default options
    int32_t num_threads = DEFAULT_NUM_THREADS;

    // Input arguments
    char *arg_num_threads = NULL;

    int opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "t:h", long_opts, NULL)) != -1) {

        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 't':
                arg_num_threads = optarg;
                break;
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

    // Parse num threads argument
    if (arg_num_threads != NULL) {
        char *endptr;
        long ret = strtol(arg_num_threads, &endptr, 10);

        if (*endptr == '\0') {
            num_threads = ret;
        } else {
            MESSAGE(stderr, "invalid number of threads -- '%s'", arg_num_threads);
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

    slow5_file_t *fp = slow5_open(f_in_name, "r");
    int ret_idx = slow5_idx_load(fp);

    if (ret_idx < 0) {
        // TODO change these to MESSAGE?
        fprintf(stderr, "Error loading index file for %s\n",
                f_in_name);

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    bool ret = EXIT_SUCCESS;

    if (read_stdin) {

        // Time spend reading slow5
        double read_time = 0;

        // Setup multithreading structures

        core_t core;
        core.num_thread = num_threads;
        core.fp = fp;

        db_t db = { 0 };
        size_t cap_ids = READ_ID_INIT_CAPACITY;
        db.read_id = (char **) malloc(cap_ids * sizeof *db.read_id);
        db.read_record = (struct Record *) malloc(cap_ids * sizeof *db.read_record);

        bool end_of_file = false;
        while (!end_of_file) {

            size_t num_ids = 0;

            while (num_ids < READ_ID_BATCH_CAPACITY) {

                char *buf = NULL;
                size_t cap_buf = 0;
                ssize_t nread;
                if ((nread = getline(&buf, &cap_buf, stdin)) == -1) {
                    end_of_file = true;
                    free(buf);
                    break;
                }

                size_t len_buf = nread - 1; // Ignore '\n'
                char *curr_id = strndup(buf, len_buf);
                curr_id[len_buf] = '\0'; // Add string terminator '\0'
                free(buf); // Free buffer

                if (num_ids >= cap_ids) {
                    // Double read id list capacity
                    cap_ids *= 2;
                    db.read_id = (char **) realloc(db.read_id, cap_ids * sizeof *db.read_id);
                    db.read_record = (struct Record *) realloc(db.read_record, cap_ids * sizeof *db.read_record);
                }
                db.read_id[num_ids] = curr_id;
                ++ num_ids;
            }

            db.n_batch = num_ids;

            // Measure reading time
            double start = slow5_realtime();

            // Fetch records for read ids in the batch
            work_db(&core, &db);

            double end = slow5_realtime();
            read_time += end - start;

            MESSAGE(stderr, "Fetched %ld reads - %ld failed",
                    num_ids - db.n_err, db.n_err);

            // Print records
            for (size_t i = 0; i < num_ids; ++ i) {
                slow5_rec_t *record = db.read_record[i].buf;
                int len = db.read_record[i].len;

                if (record == NULL || len < 0) {
                    ret = EXIT_FAILURE;
                } else {
                    slow5_rec_fwrite(stdout,record,fp->header->aux_meta,FORMAT_ASCII,NULL);
                    slow5_rec_free(record);
                }
            }

        }

        // Print total time to read slow5
        MESSAGE(stderr, "read time = %.3f sec", read_time);

        // Free everything
        free(db.read_id);
        free(db.read_record);

    } else {

        for (int i = optind + 1; i < argc; ++ i){
            bool success = fetch_record(fp, argv[i], argv, meta);
            if (!success) {
                ret = EXIT_FAILURE;
            }
        }
    }

    slow5_close(fp);

    EXIT_MSG(ret, argv, meta);
    return ret;
}
