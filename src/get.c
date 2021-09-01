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
#include <string>

#define READ_ID_INIT_CAPACITY (128)

#define USAGE_MSG "Usage: %s [OPTION]... SLOW5|BLOW5_FILE [READ_ID]...\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Display the read entry for each specified read id from a slow5 file.\n" \
    "With no READ_ID, read from standard input newline separated read ids.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -t, --threads=[INT]                number of threads -- 4\n" \
    "    --to=[FORMAT]                      specify output file format\n" \
    "    -c, --compress=[REC_METHOD]        specify record compression method -- zlib (only available for format blow5)\n" \
    "    -s, --sig-compress=[SIG_METHOD]    specify signal compression method -- none (only available for format blow5)\n" \
    "    -K --batchsize                     the number of records on the memory at once. [default: 4096]\n" \
    "    -o, --output [FILE]                output contents to FILE [default: stdout]\n" \
    "    -l --list [FILE]                   list of read ids provided as a single-column text file with one read id per line.\n" \
    "    -h, --help                         display this message and exit.\n" \
    HELP_FORMATS_METHODS

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
            slow5_rec_fwrite(slow5_file_pointer,record,fp->header->aux_meta, format_out, compress);
            slow5_press_free(compress);
        }
        slow5_rec_free(record);
    }

    return success;
}

int get_main(int argc, char **argv, struct program_meta *meta) {

    // Debug: print arguments
    if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("printing the arguments given%s","");
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

    // Default options
    int32_t num_threads = DEFAULT_NUM_THREADS;
    int64_t read_id_batch_capacity = READ_ID_BATCH_CAPACITY;

    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum slow5_fmt extension_format = SLOW5_FORMAT_BINARY;
    enum slow5_press_method record_press_out = SLOW5_COMPRESS_ZLIB;
    enum slow5_press_method signal_press_out = SLOW5_COMPRESS_NONE;
    int compression_set = 0;
    int format_out_set = 0;

    // Input arguments
    char *arg_num_threads = NULL;
    char *arg_fname_out = NULL;
    char *arg_fname_in = NULL;
    char *arg_fmt_out = NULL;
    char *arg_record_press_out = NULL;
    char *arg_signal_press_out = NULL;

    int opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "o:b:c:K:l:t:he", long_opts, NULL)) != -1) {

        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 'b':
                format_out_set = 1;
                arg_fmt_out = optarg;
                break;
            case 'c':
                compression_set = 1;
                arg_record_press_out = optarg;
                break;
            case 's':
                compression_set = 1;
                arg_signal_press_out = optarg;
                break;
            case 't':
                arg_num_threads = optarg;
                break;
            case 'e':
                benchmark = true;
                break;
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'K':
                read_id_batch_capacity = atoi(optarg);
                if(read_id_batch_capacity < 0){
                    fprintf(stderr, "batchsize cannot be negative\n");
                    fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                    EXIT_MSG(EXIT_FAILURE, argv, meta);
                    return EXIT_FAILURE;
                }
                break;
            case 'l':
                arg_fname_in = optarg;
                break;
            case 'h':
                if (meta->verbosity_level >= LOG_DEBUG) {
                    DEBUG("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }
    if (arg_fmt_out) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("parsing output format%s","");
        }
        format_out = parse_name_to_fmt(arg_fmt_out);
        // An error occured
        if (format_out == SLOW5_FORMAT_UNKNOWN) {
            ERROR("invalid output format -- '%s'", arg_fmt_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if(arg_fname_out){
        extension_format = parse_path_to_fmt(arg_fname_out);
        if(format_out_set==0){
            format_out = extension_format;
        }
    }
    if(arg_fname_out && format_out!=extension_format){
        ERROR("Output file extension does not match with the output format%s",".");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(compression_set == 0 && format_out == SLOW5_FORMAT_ASCII){
        record_press_out = SLOW5_COMPRESS_NONE;
        signal_press_out = SLOW5_COMPRESS_NONE;
    }
    // compression option is only effective with -b blow5
    if(compression_set == 1 && format_out == SLOW5_FORMAT_ASCII){
        ERROR("%s","Compression option (-c/-s) is only available for SLOW5 binary format.");
        return EXIT_FAILURE;
    }
    if (arg_record_press_out != NULL) {
        record_press_out = name_to_slow5_press_method(arg_record_press_out);
        if (record_press_out == (enum slow5_press_method) -1) {
            ERROR("invalid record compression method -- '%s'", arg_record_press_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if (arg_signal_press_out != NULL) {
        signal_press_out = name_to_slow5_press_method(arg_signal_press_out);
        if (signal_press_out == (enum slow5_press_method) -1) {
            ERROR("invalid signal compression method -- '%s'", arg_signal_press_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    FILE* slow5_file_pointer = stdout;
    if(arg_fname_out){
        slow5_file_pointer = fopen(arg_fname_out, "wb");
        if (!slow5_file_pointer) {
            ERROR("Output file %s could not be opened - %s.", arg_fname_out, strerror(errno));
            return EXIT_FAILURE;
        }
    }else{
        std::string stdout_s = "stdout";
        arg_fname_out = &stdout_s[0];
    }

    // Parse num threads argument
    if (arg_num_threads != NULL) {
        char *endptr;
        long ret = strtol(arg_num_threads, &endptr, 10);

        if (*endptr == '\0') {
            num_threads = ret;
        } else {
            ERROR("invalid number of threads -- '%s'", arg_num_threads);
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

    } else if (optind == argc - 1) {
        read_stdin = true;
    }

    FILE* read_list_in = stdin;
    if(arg_fname_in){
        read_list_in = fopen(arg_fname_in, "r");
        if (!read_list_in) {
            ERROR("Output file %s could not be opened - %s.", arg_fname_in, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    char *f_in_name = argv[optind];

    slow5_file_t *slow5file = slow5_open(f_in_name, "r");
    if (!slow5file) {
        ERROR("cannot open %s. \n", f_in_name);
        exit(EXIT_FAILURE);
    }
    slow5_press_method_t press_out = {record_press_out,signal_press_out};
    if(slow5_hdr_fwrite(slow5_file_pointer, slow5file->header, format_out, press_out) == -1){
        ERROR("Could not read the read ids from %s\n", arg_fname_in);
        exit(EXIT_FAILURE);
    }
    int ret_idx = slow5_idx_load(slow5file);

    if (ret_idx < 0) {
        ERROR("Error loading index file for %s\n", f_in_name);

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
        core.fp = slow5file;
        core.format_out = format_out;
        core.press_method = press_out;
        core.benchmark = benchmark;

        db_t db = { 0 };
        int64_t cap_ids = READ_ID_INIT_CAPACITY;
        db.read_id = (char **) malloc(cap_ids * sizeof *db.read_id);
        db.read_record = (raw_record_t*) malloc(cap_ids * sizeof *db.read_record);

        bool end_of_file = false;
        while (!end_of_file) {

            int64_t num_ids = 0;

            while (num_ids < read_id_batch_capacity) {

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

            VERBOSE("Fetched %ld reads - %ld failed",
                    num_ids - db.n_err, db.n_err);

            // Print records
            if(benchmark == false){
                for (int64_t i = 0; i < num_ids; ++ i) {
                    void *buffer = db.read_record[i].buffer;
                    int len = db.read_record[i].len;

                    if (buffer == NULL || len < 0) {
                        ret = EXIT_FAILURE;
                    } else {
                        fwrite(buffer,1,len,slow5_file_pointer);
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
            bool success = fetch_record(slow5file, argv[i], argv, meta, format_out, press_out, benchmark, slow5_file_pointer);
            if (!success) {
                ret = EXIT_FAILURE;
            }
        }
    }

    if (format_out == SLOW5_FORMAT_BINARY) {
        slow5_eof_fwrite(slow5file->fp);
    }
    slow5_close(slow5file);
    fclose(read_list_in);

    EXIT_MSG(ret, argv, meta);
    return ret;
}
