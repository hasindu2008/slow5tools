#include <getopt.h>
#include <stdio.h>

#include <slow5/slow5.h>
#include "thread.h"
#include "cmd.h"
#include "misc.h"
#include <string>

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
    "    -t, --threads=[INT]                number of threads -- 4\n" \
    "    -c, --compress [compression_type]  convert to compressed blow5. [default: zlib]\n" \
    "    --to [STR]                         output in the format specified in STR. slow5 for SLOW5 ASCII. blow5 for SLOW5 binary (BLOW5) [default: BLOW5]\n" \
    "    -K --batchsize                     the number of records on the memory at once. [default: 4096]\n" \
    "    -o, --output [FILE]                output contents to FILE [default: stdout]\n" \
    "    -l --list [FILE]                   list of read ids provided as a single-column text file with one read id per line.\n" \
    "    -h, --help                         display this message and exit.\n" \

void work_per_single_read(core_t *core, db_t *db, int32_t i) {

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
            slow5_press_method_t method = {core->press_method, SLOW5_COMPRESS_NONE};
            struct slow5_press* compress = slow5_press_init(method); /* TODO add signal compression */
            db->read_record[i].buffer = slow5_rec_to_mem(record,core->fp->header->aux_meta, core->format_out, compress, &record_size);
            db->read_record[i].len = record_size;
            slow5_press_free(compress);
        }
        slow5_rec_free(record);
    }
    free(id);
}

bool fetch_record(slow5_file_t *fp, const char *read_id, char **argv, program_meta *meta, slow5_fmt format_out,
                  slow5_press_method press_method, bool benchmark, FILE *slow5_file_pointer) {

    bool success = true;

    int len = 0;
    fprintf(stderr, "Fetching %s\n", read_id);
    slow5_rec_t *record=NULL;

    len = slow5_get(read_id, &record,fp);

    if (record == NULL || len < 0) {
        fprintf(stderr, "Error locating %s\n", read_id);
        success = false;

    } else {
        if (benchmark == false){
            slow5_press_method_t method = {press_method, SLOW5_COMPRESS_NONE};
            struct slow5_press* compress = slow5_press_init(method); /* TODO add signal compression */
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
        if (meta->verbosity_level >= LOG_VERBOSE) {
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
        {"to", required_argument, NULL, 'b'},    //0
        {"compress", required_argument, NULL, 'c'},  //1
        {"batchsize", required_argument, NULL, 'K'},  //2
        {"output", required_argument, NULL, 'o'}, //3
        {"list", required_argument, NULL, 'l'},  //4
        {"threads", required_argument, NULL, 't' }, //5
        {"help", no_argument, NULL, 'h' }, //6
        {"benchmark", no_argument, NULL, 'e' }, //7
        {NULL, 0, NULL, 0 }
    };

    bool read_stdin = false;
    bool benchmark = false;

    // Default options
    int32_t num_threads = DEFAULT_NUM_THREADS;

    // Input arguments
    char *arg_num_threads = NULL;
    char *arg_fname_out = NULL;
    char *arg_fname_in = NULL;

    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum slow5_press_method pressMethod = SLOW5_COMPRESS_ZLIB;
    int compression_set = 0;

    int64_t read_id_batch_capacity = READ_ID_BATCH_CAPACITY;

    int opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "o:b:c:K:l:t:he", long_opts, NULL)) != -1) {

        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 'b':
                if(strcmp(optarg,"slow5")==0){
                    format_out = SLOW5_FORMAT_ASCII;
                }else if(strcmp(optarg,"blow5")==0){
                    format_out = SLOW5_FORMAT_BINARY;
                }else{
                    ERROR("Incorrect output format%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'c':
                compression_set = 1;
                if(strcmp(optarg,"none")==0){
                    pressMethod = SLOW5_COMPRESS_NONE;
                }else if(strcmp(optarg,"zlib")==0){
                    pressMethod = SLOW5_COMPRESS_ZLIB;
                }else{
                    ERROR("Incorrect compression type%s", "");
                    exit(EXIT_FAILURE);
                }
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
                if (meta->verbosity_level >= LOG_VERBOSE) {
                    VERBOSE("displaying large help message%s","");
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
    if(compression_set == 0 && format_out == SLOW5_FORMAT_ASCII){
        pressMethod = SLOW5_COMPRESS_NONE;
    }
    // compression option is only effective with -b blow5
    if(compression_set == 1 && format_out == SLOW5_FORMAT_ASCII){
        ERROR("%s","Compression option (-c) is only available for SLOW5 binary format.");
        return EXIT_FAILURE;
    }

    std::string output_file;
    std::string extension;
    if(arg_fname_out){
        output_file = std::string(arg_fname_out);
        extension = output_file.substr(output_file.length()-6, output_file.length());
    }

    if(arg_fname_out && format_out==SLOW5_FORMAT_ASCII && extension!=".slow5"){
        ERROR("Output file extension '%s' does not match with the output format:FORMAT_ASCII", extension.c_str());
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }else if(arg_fname_out && format_out==SLOW5_FORMAT_BINARY && extension!=".blow5"){
        ERROR("Output file extension '%s' does not match with the output format:FORMAT_BINARY", extension.c_str());
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
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
    slow5_press_method_t method = {pressMethod, SLOW5_COMPRESS_NONE};
    if(slow5_hdr_fwrite(slow5_file_pointer, slow5file->header, format_out, method) == -1){ /* TODO add signal compression */
        ERROR("Could not read the read ids from %s\n", arg_fname_in);
        exit(EXIT_FAILURE);
    }
    int ret_idx = slow5_idx_load(slow5file);

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
        core.fp = slow5file;
        core.format_out = format_out;
        core.press_method = pressMethod;
        core.benchmark = benchmark;

        db_t db = { 0 };
        int64_t cap_ids = READ_ID_INIT_CAPACITY;
        db.read_id = (char **) malloc(cap_ids * sizeof *db.read_id);
        db.read_record = (struct Record *) malloc(cap_ids * sizeof *db.read_record);

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
        MESSAGE(stderr, "read time = %.3f sec", read_time);

        // Free everything
        free(db.read_id);
        free(db.read_record);

    } else {

        for (int i = optind + 1; i < argc; ++ i){
            bool success = fetch_record(slow5file, argv[i], argv, meta, format_out, pressMethod, benchmark, slow5_file_pointer);
            if (!success) {
                ret = EXIT_FAILURE;
            }
        }
    }

    slow5_close(slow5file);
    fclose(read_list_in);

    EXIT_MSG(ret, argv, meta);
    return ret;
}
