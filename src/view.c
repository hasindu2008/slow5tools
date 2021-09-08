/**
 * @file view.c
 * @brief view SLOW5 as BLOW5 and vice versa
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au) Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 27/02/2021
 */
#include "slow5_misc.h"
#include "error.h"
#include "cmd.h"
#include "misc.h"
#include "thread.h"
#include <slow5/slow5.h>
#include "slow5_extra.h"
#include <getopt.h>

#define USAGE_MSG "Usage: %s [OPTION]... [FILE]...\n"
#define HELP_LARGE_MSG \
    "View a fast5 or slow5 FILE in another format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    HELP_MSG_OUTPUT_FILE \
    HELP_MSG_PRESS \
    HELP_MSG_THREADS \
    HELP_MSG_BATCH \
    HELP_MSG_OUTPUT_FORMAT\
    "    --from FORMAT                 specify input file format [auto]\n" \
    HELP_FORMATS_METHODS

extern int slow5tools_verbosity_level;

int slow5_convert_parallel(struct slow5_file *from, FILE *to_fp, enum slow5_fmt to_format, slow5_press_method_t to_compress, size_t num_threads, int64_t batch_size, struct program_meta *meta);

void depress_parse_rec_to_mem(core_t *core, db_t *db, int32_t i) {
    //
    struct slow5_rec *read = NULL;
    if (slow5_rec_depress_parse(&db->mem_records[i], &db->mem_bytes[i], NULL, &read, core->fp) != 0) {
        exit(EXIT_FAILURE);
    } else {
        free(db->mem_records[i]);
    }
    struct slow5_press *press_ptr = slow5_press_init(core->press_method);
    size_t len;
    if ((db->read_record[i].buffer = slow5_rec_to_mem(read, core->fp->header->aux_meta, core->format_out, press_ptr, &len)) == NULL) {
        slow5_press_free(press_ptr);
        slow5_rec_free(read);
        exit(EXIT_FAILURE);
    }
    slow5_press_free(press_ptr);
    db->read_record[i].len = len;
    slow5_rec_free(read);
}

int view_main(int argc, char **argv, struct program_meta *meta) {
    int view_ret = EXIT_SUCCESS;

    // Debug: print arguments
    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
        {"compress",        required_argument,  NULL, 'c'},
        {"sig-compress",    required_argument,  NULL, 's'},
        {"from",            required_argument,  NULL, 'f'},
        {"help",            no_argument,        NULL, 'h'},
        {"output",          required_argument,  NULL, 'o'},
        {"to",              required_argument,  NULL, 'b'},
        {"threads",         required_argument,  NULL, 't' },
        {"batchsize",       required_argument, NULL, 'K'},
        {NULL, 0, NULL, 0}
    };

    opt_t user_opts;
    init_opt(&user_opts);

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "s:c:f:ho:b:t:K:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);

        switch (opt) {
            case 's':
                user_opts.arg_signal_press_out = optarg;
                break;
            case 'c':
                user_opts.arg_record_press_out = optarg;
                break;
            case 'f':
                user_opts.arg_fmt_in = optarg;
                break;
            case 'K':
                user_opts.arg_batch = optarg;
                break;
            case 'h':
                DEBUG("displaying large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'o':
                user_opts.arg_fname_out = optarg;
                break;
            case 'b':
                user_opts.arg_fmt_out = optarg;
                break;
            case 't':
                user_opts.arg_num_threads = optarg;
                break;
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

    // Check for an input file to parse
    if (optind >= argc) { // TODO use stdin if no file given
        ERROR("missing input file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    } else if (optind != argc - 1) { // TODO handle more than 1 file?
        ERROR(">1 input file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    } else { // Save input filename
        user_opts.arg_fname_in = argv[optind];
    }
    if(auto_detect_formats(&user_opts, 0) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if (user_opts.fmt_out == SLOW5_FORMAT_UNKNOWN) {
        user_opts.fmt_out = SLOW5_FORMAT_ASCII;
    }
    if(parse_compression_opts(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
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

    // Do the conversion
    if ((user_opts.fmt_in == SLOW5_FORMAT_ASCII || user_opts.fmt_in == SLOW5_FORMAT_BINARY) &&
            (user_opts.fmt_out == SLOW5_FORMAT_ASCII || user_opts.fmt_out == SLOW5_FORMAT_BINARY)) {

        struct slow5_file *s5p = slow5_open_with(user_opts.arg_fname_in, "r", (enum slow5_fmt) user_opts.fmt_in);

        if (s5p == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  user_opts.arg_fname_in, strerror(errno));
            view_ret = EXIT_FAILURE;
        }

        // TODO if output is the same format just duplicate file
        slow5_press_method_t press_out = {user_opts.record_press_out,user_opts.signal_press_out};
        if (slow5_convert_parallel(s5p, user_opts.f_out, (enum slow5_fmt) user_opts.fmt_out, press_out, user_opts.num_threads, user_opts.read_id_batch_capacity, meta) != 0) {
            ERROR("Conversion failed.%s", "");
            view_ret = EXIT_FAILURE;
        }

        // TODO if output is the same format just duplicate file
//        if (slow5_convert(s5p, f_out, (enum slow5_fmt) fmt_out, press_out) != 0) {
//            ERROR("Conversion failed.%s", "");
//            view_ret = EXIT_FAILURE;
//        }

        if (slow5_close(s5p) == EOF) {
            ERROR("File '%s' failed on closing - %s.",
                  user_opts.arg_fname_in, strerror(errno));
            view_ret = EXIT_FAILURE;
        }

    } else {
        view_ret = EXIT_FAILURE;
    }

    // Close output file
    if (user_opts.arg_fname_out != NULL) {
        DEBUG("closing output file%s","");

        if (fclose(user_opts.f_out) == EOF) {
            ERROR("File '%s' failed on closing - %s.",
                  user_opts.arg_fname_out, strerror(errno));

            view_ret = EXIT_FAILURE;
        }
    }

    if (view_ret == EXIT_FAILURE) {
        EXIT_MSG(EXIT_SUCCESS, argv, meta);
    }
    return view_ret;
}

int slow5_convert_parallel(struct slow5_file *from, FILE *to_fp, enum slow5_fmt to_format, slow5_press_method_t to_compress, size_t num_threads, int64_t batch_size, struct program_meta *meta) {
    if (from == NULL || to_fp == NULL || to_format == SLOW5_FORMAT_UNKNOWN) {
        return -1;
    }

    if (slow5_hdr_fwrite(to_fp, from->header, to_format, to_compress) == -1) {
        return -2;
    }

    double time_get_to_mem = 0;
    double time_thread_execution = 0;
    double time_write = 0;
    int flag_end_of_file = 0;
    while(1) {

        db_t db = { 0 };
        db.mem_records = (char **) malloc(batch_size * sizeof *db.read_id);
        db.mem_bytes = (size_t *) malloc(batch_size * sizeof *db.read_id);

        int64_t record_count = 0;
        size_t bytes;
        char *mem;
        double realtime = slow5_realtime();
        while (record_count < batch_size) {
            if (!(mem = (char *) slow5_get_next_mem(&bytes, from))) {
                if (slow5_errno != SLOW5_ERR_EOF) {
                    return EXIT_FAILURE;
                } else {
                    flag_end_of_file = 1;
                    break;
                }
            } else {
                db.mem_records[record_count] = mem;
                db.mem_bytes[record_count] = bytes;
//                mem_records.push_back(mem);
//                mem_bytes.push_back(bytes);
                record_count++;
            }
        }
        time_get_to_mem += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        // Setup multithreading structures
        core_t core;
        core.num_thread = num_threads;
        core.fp = from;
        core.format_out = to_format;
        core.press_method = to_compress;

        db.n_batch = record_count;
        db.read_record = (raw_record_t*) malloc(record_count * sizeof *db.read_record);
        work_db(&core,&db,depress_parse_rec_to_mem);
        time_thread_execution += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        for (int64_t i = 0; i < record_count; i++) {
            fwrite(db.read_record[i].buffer,1,db.read_record[i].len,to_fp);
            free(db.read_record[i].buffer);
        }
        time_write += slow5_realtime() - realtime;

        // Free everything
        free(db.mem_bytes);
        free(db.mem_records);
        free(db.read_record);

        if(flag_end_of_file == 1){
            break;
        }

    }
    if (to_format == SLOW5_FORMAT_BINARY) {
        if (slow5_eof_fwrite(to_fp) == -1) {
            return -2;
        }
    }

    DEBUG("time_get_to_mem\t%.3fs", time_get_to_mem);
    DEBUG("time_depress_parse\t%.3fs", time_thread_execution);
    DEBUG("time_write\t%.3fs", time_write);

    return EXIT_SUCCESS;
}
