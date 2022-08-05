/**
 * @file get.c
 * @brief get read(record) given the read_id from a SLOW5 file
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au) Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 27/02/2021
 */
#include <getopt.h>
#include <stdio.h>

#include <slow5/slow5.h>
#include "thread.h"
#include "cmd.h"
#include "misc.h"

#define READ_ID_INIT_CAPACITY (128)

#define USAGE_MSG "Usage: %s [OPTIONS] [SLOW5_FILE] [READ_ID]...\n"
#define HELP_LARGE_MSG \
    "Display the read entry for each specified read id from a slow5 file.\n" \
    "With no READ_ID, read from standard input newline separated read ids.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    --to FORMAT                   specify output file format\n" \
    "    -o, --output [FILE]           output contents to FILE [default: stdout]\n" \
    HELP_MSG_PRESS \
    HELP_MSG_THREADS \
    HELP_MSG_BATCH \
    "    -l --list [FILE]              list of read ids provided as a single-column text file with one read id per line.\n" \
    HELP_MSG_HELP \
    HELP_FORMATS_METHODS

extern int slow5tools_verbosity_level;

void work_per_single_read_get(core_t *core, db_t *db, int32_t i) {

    char *id = db->read_id[i];

    int len = 0;
    //fprintf(stderr, "Fetching %s\n", id); // TODO print here or during ordered loop later?
    slow5_rec_t *record=NULL;

    len = slow5_get(id,&record,core->fp);

    if (record == NULL || len < 0) {
        fprintf(stderr, "Error locating %s\n", id);
        ++ db->n_err;
    }else {
        if (core->benchmark == false){
            size_t record_size;
            struct slow5_press* compress = slow5_press_init(core->press_method);
            if(!compress){
                ERROR("Could not initialize the slow5 compression method%s","");
                exit(EXIT_FAILURE);
            }
            db->read_record[i].buffer = slow5_rec_to_mem(record,core->fp->header->aux_meta, core->format_out, compress, &record_size);
            db->read_record[i].len = record_size;
            slow5_press_free(compress);
        }
        slow5_rec_free(record);
    }
    free(id);
}

bool fetch_record(slow5_file_t *fp, const char *read_id, char **argv, program_meta *meta, slow5_fmt format_out,
                  slow5_press_method_t press_method, bool benchmark, FILE *slow5_file_pointer) {

    bool success = true;

    int len = 0;
    //fprintf(stderr, "Fetching %s\n", read_id);
    slow5_rec_t *record=NULL;

    len = slow5_get(read_id, &record,fp);

    if (record == NULL || len < 0) {
        WARNING("Error locating %s", read_id);
        success = false;

    } else {
        if (benchmark == false){
            struct slow5_press* compress = slow5_press_init(press_method);
            if(!compress){
                ERROR("Could not initialize the slow5 compression method%s","");
                exit(EXIT_FAILURE);
            }
            slow5_rec_fwrite(slow5_file_pointer,record,fp->header->aux_meta, format_out, compress);
            slow5_press_free(compress);
        }
        slow5_rec_free(record);
    }

    return success;
}

int get_main(int argc, char **argv, struct program_meta *meta) {

    // Debug: print arguments
    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
        {"to",          required_argument, NULL, 'b'},    //0
        {"compress",    required_argument, NULL, 'c'},  //1
        {"sig-compress",required_argument,  NULL, 's'}, //2
        {"batchsize",   required_argument, NULL, 'K'},  //3
        {"output",      required_argument, NULL, 'o'}, //4
        {"list",        required_argument, NULL, 'l'},  //5
        {"threads",     required_argument, NULL, 't' }, //6
        {"help",        no_argument, NULL, 'h' }, //7
        {"benchmark",   no_argument, NULL, 'e' }, //8
        {NULL, 0, NULL, 0 }
    };

    bool read_stdin = false;
    bool benchmark = false;

    opt_t user_opts;
    init_opt(&user_opts);

    // Input arguments
    char* read_list_file_in = NULL;

    int opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "o:b:c:s:K:l:t:he", long_opts, NULL)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'b':
                user_opts.arg_fmt_out = optarg;
                break;
            case 'c':
                user_opts.arg_record_press_out = optarg;
                break;
            case 's':
                user_opts.arg_signal_press_out = optarg;
                break;
            case 't':
                user_opts.arg_num_threads = optarg;
                break;
            case 'e':
                benchmark = true;
                break;
            case 'o':
                user_opts.arg_fname_out = optarg;
                break;
            case 'K':
                user_opts.arg_batch = optarg;
                break;
            case 'l':
                read_list_file_in = optarg;
                break;
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
    if(parse_num_threads(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if(parse_batch_size(&user_opts,argc,argv) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if(parse_format_args(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(auto_detect_formats(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if(parse_compression_opts(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if(benchmark && user_opts.arg_fname_out){
        ERROR("Benchmark does not support writing records out%s", "");
        return EXIT_FAILURE;
    }

    // Parse output argument
    if (user_opts.arg_fname_out != NULL) {
        DEBUG("opening output file%s","");
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(user_opts.arg_fname_out, "w");

        // An error occurred
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  user_opts.arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            user_opts.f_out = new_file;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        ERROR("missing slow5 or blow5 file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;

    } else if (optind == argc - 1) {
        read_stdin = true;
    }

    FILE* read_list_in = stdin;
    if(read_list_file_in){
        read_list_in = fopen(read_list_file_in, "r");
        if (!read_list_in) {
            ERROR("Output file %s could not be opened - %s.", read_list_file_in, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    char *f_in_name = argv[optind];


    slow5_file_t *slow5file = slow5_open(f_in_name, "r");
    if (!slow5file) {
        ERROR("cannot open %s. \n", f_in_name);
        return EXIT_FAILURE;
    }

    slow5_press_method_t press_out = {user_opts.record_press_out,user_opts.signal_press_out};
    if(benchmark == false){
        if(slow5_hdr_fwrite(user_opts.f_out, slow5file->header, user_opts.fmt_out, press_out) == -1){
            ERROR("Could not write the output header%s\n", "");
            return EXIT_FAILURE;
        }
    }

    int ret_idx = slow5_idx_load(slow5file);
    if (ret_idx < 0) {
        ERROR("Error loading index file for %s\n", f_in_name);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if (read_stdin) {
        // Time spend reading slow5
        double read_time = 0;

        // Setup multithreading structures
        core_t core;
        core.num_thread = user_opts.num_threads;
        core.fp = slow5file;
        core.format_out = user_opts.fmt_out;
        core.press_method = press_out;
        core.benchmark = benchmark;

        db_t db = { 0 };
        int64_t cap_ids = READ_ID_INIT_CAPACITY;
        db.read_id = (char **) malloc(cap_ids * sizeof(char*));
        db.read_record = (raw_record_t*) malloc(cap_ids * sizeof(raw_record_t));
        MALLOC_CHK(db.read_id);
        MALLOC_CHK(db.read_record);
        bool end_of_file = false;
        while (!end_of_file) {
            int64_t num_ids = 0;
            while (num_ids < user_opts.read_id_batch_capacity) {
                char *buf = NULL;
                size_t cap_buf = 0;
                ssize_t nread;
                if ((nread = getline(&buf, &cap_buf, read_list_in)) == -1) {
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
                    db.read_record = (raw_record_t*) realloc(db.read_record, cap_ids * sizeof *db.read_record);
                }
                db.read_id[num_ids] = curr_id;
                ++ num_ids;
            }

            db.n_batch = num_ids;

            // Measure reading time
            double start = slow5_realtime();

            // Fetch records for read ids in the batch
            work_db(&core, &db, work_per_single_read_get);

            double end = slow5_realtime();
            read_time += end - start;

            VERBOSE("Fetched %ld reads of %ld", num_ids - db.n_err, num_ids);

            // Print records
            if(benchmark == false){
                for (int64_t i = 0; i < num_ids; ++ i) {
                    void *buffer = db.read_record[i].buffer;
                    int len = db.read_record[i].len;
                    if (buffer == NULL || len < 0) {
                        ERROR("Could not write the fetched read.%s","");
                        return EXIT_FAILURE;
                    } else {
                        fwrite(buffer,1,len,user_opts.f_out);
                        free(buffer);
                    }
                }
            }
        }
        // Print total time to read slow5
        VERBOSE("read time = %.3f sec", read_time);
        // Free everything
        free(db.read_id);
        free(db.read_record);
    } else {
        for (int i = optind + 1; i < argc; ++ i){
            bool success = fetch_record(slow5file, argv[i], argv, meta, user_opts.fmt_out, press_out, benchmark, user_opts.f_out);
            if (!success) {
                ERROR("Could not fetch records.%s","");
                return EXIT_FAILURE;
            }
        }
    }

    if(benchmark == false){
        if (user_opts.fmt_out == SLOW5_FORMAT_BINARY) {
                slow5_eof_fwrite(user_opts.f_out);
            }
    }

    slow5_close(slow5file);
    fclose(read_list_in);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
