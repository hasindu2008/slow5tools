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
#include <string>
#include <vector>

#define DEFAULT_NUM_THREADS (4)
#define READ_ID_BATCH_CAPACITY (4096)
#define USAGE_MSG "Usage: %s [OPTION]... [FILE]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "View a fast5 or slow5 FILE in another format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    --from=[FORMAT]            specify input file format\n" \
    "    --to=[FORMAT]              specify output file format\n" \
    "    -c, --compress=[METHOD]    specify output compression method -- zlib (only available for format blow5)\n" \
    "    -s, --sig-compress=[METHOD]    specify output compression method -- none (only available for format blow5)\n" \
    "    -o, --output=[FILE]        output to FILE -- stdout\n" \
    "    -h, --help                 display this message and exit\n"                                               \
    "    -t, --threads [INT]        number of threads [default: 4]\n"                                              \
    "    -K --batchsize             the number of records on the memory at once. [default: 4096]\n" \
    "FORMATS:\n" \
    "    slow5\n" \
    "    blow5\n" \
    "METHODS:\n" \
    "    none\n" \
    "    zlib -- default\n"

enum view_fmt {
    // The first formats must match the order of enum slow5_fmt
    VIEW_FORMAT_UNKNOWN,
    VIEW_FORMAT_SLOW5_ASCII,
    VIEW_FORMAT_SLOW5_BINARY,

    VIEW_FORMAT_FAST5
};

struct view_fmt_meta {
    enum view_fmt format;
    const char *name;
    const char *ext;
};

static const struct view_fmt_meta VIEW_FORMAT_META[] = {
    { VIEW_FORMAT_SLOW5_ASCII,  SLOW5_ASCII_NAME,     SLOW5_ASCII_EXTENSION     },
    { VIEW_FORMAT_SLOW5_BINARY, SLOW5_BINARY_NAME,    SLOW5_BINARY_EXTENSION    },
};

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

enum view_fmt name_to_view_fmt(const char *fmt_str) {
    enum view_fmt fmt = VIEW_FORMAT_UNKNOWN;

    for (size_t i = 0; i < sizeof VIEW_FORMAT_META / sizeof VIEW_FORMAT_META[0]; ++ i) {
        const struct view_fmt_meta meta = VIEW_FORMAT_META[i];
        if (strcmp(meta.name, fmt_str) == 0) {
            fmt = meta.format;
            break;
        }
    }

    return fmt;
}

enum view_fmt path_to_view_fmt(const char *fname) {
    enum view_fmt fmt = VIEW_FORMAT_UNKNOWN;

    for (int i = strlen(fname) - 1; i >= 0; -- i) {
        if (fname[i] == '.') {
            const char *ext = fname + i;

            for (size_t j = 0; j < sizeof VIEW_FORMAT_META / sizeof VIEW_FORMAT_META[0]; ++ j) {
                const struct view_fmt_meta meta = VIEW_FORMAT_META[j];
                if (strcmp(ext, meta.ext) == 0) { // TODO comparing the '.' is superfluous
                    fmt = meta.format;
                    break;
                }
            }
            break;
        }
    }


    return fmt;
}

enum slow5_press_method name_to_slow5_press_method(const char *name) {
    enum slow5_press_method comp = (enum slow5_press_method) -1;

    if (strcmp(name, "none") == 0) {
        comp = SLOW5_COMPRESS_NONE;
    } else if (strcmp(name, "zlib") == 0) {
        comp = SLOW5_COMPRESS_ZLIB;
    } else if (strcmp(name, "svb-zd") == 0) {
        comp = SLOW5_COMPRESS_SVB_ZD;
    } else if (strcmp(name, "zstd") == 0) {
        comp = SLOW5_COMPRESS_ZSTD;
    }

    return comp;
}

//static double init_realtime = 0;

int view_main(int argc, char **argv, struct program_meta *meta) {
    int view_ret = EXIT_SUCCESS;

    //init_realtime = slow5_realtime();

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
        {"compress",    required_argument,  NULL, 'c'},
        {"sig-compress",    required_argument,  NULL, 's'},
        {"from",        required_argument,  NULL, 'f'},
        {"help",        no_argument,        NULL, 'h'},
        {"output",      required_argument,  NULL, 'o'},
        {"to",          required_argument,  NULL, 'b'},
        {"threads",     required_argument,  NULL, 't' },
        {"batchsize", required_argument, NULL, 'K'},
        {NULL, 0, NULL, 0}
    };

    // Default options
    FILE *f_out = stdout;
    enum view_fmt fmt_in = VIEW_FORMAT_UNKNOWN;
    enum view_fmt fmt_out = VIEW_FORMAT_UNKNOWN;
    enum slow5_press_method record_press_out = SLOW5_COMPRESS_ZLIB;
    enum slow5_press_method signal_press_out = SLOW5_COMPRESS_NONE;

    // Input arguments
    char *arg_fname_in = NULL;
    char *arg_fname_out = NULL;
    char *arg_fmt_in = NULL;
    char *arg_fmt_out = NULL;
    char *arg_record_press_out = NULL;
    char *arg_signal_press_out = NULL;
    char *arg_num_threads = NULL;
    size_t num_threads = DEFAULT_NUM_THREADS;
    int64_t read_id_batch_capacity = READ_ID_BATCH_CAPACITY;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "s:c:f:ho:b:t:K:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 's':
                arg_signal_press_out = optarg;
                break;
            case 'c':
                arg_record_press_out = optarg;
                break;
            case 'f':
                arg_fmt_in = optarg;
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
            case 'h':
                if (meta->verbosity_level >= LOG_DEBUG) {
                    DEBUG("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'b':
                arg_fmt_out = optarg;
                break;
            case 't':
                arg_num_threads = optarg;
                break;
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
            ERROR("invalid number of threads -- '%s'", arg_num_threads);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    // Parse format arguments
    if (arg_fmt_in != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("parsing input format%s","");
        }

        fmt_in = name_to_view_fmt(arg_fmt_in);

        // An error occured
        if (fmt_in == VIEW_FORMAT_UNKNOWN) {
            ERROR("invalid input format -- '%s'", arg_fmt_in);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if (arg_fmt_out != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("parsing output format%s","");
        }

        fmt_out = name_to_view_fmt(arg_fmt_out);

        // An error occured
        if (fmt_out == VIEW_FORMAT_UNKNOWN) {
            ERROR("invalid output format -- '%s'", arg_fmt_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
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
        arg_fname_in = argv[optind];
    }

    // Autodetect input/output formats
    if (fmt_in == VIEW_FORMAT_UNKNOWN) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("auto detecting input file format%s","");
        }

        fmt_in = path_to_view_fmt(arg_fname_in);

        // Error
        if (fmt_in == VIEW_FORMAT_UNKNOWN) {
            ERROR("cannot detect file format -- '%s'", arg_fname_in);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if (fmt_out == VIEW_FORMAT_UNKNOWN) {
        if (arg_fname_out == NULL) {
            fmt_out = VIEW_FORMAT_SLOW5_ASCII;
        }

        else{
            if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
                DEBUG("auto detecting output file format%s","");
            }

            fmt_out = path_to_view_fmt(arg_fname_out);

            // Error
            if (fmt_out == VIEW_FORMAT_UNKNOWN) {
                ERROR("cannot detect file format -- '%s'", arg_fname_out);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }
    }


    if (arg_record_press_out != NULL) {
        if (fmt_out != VIEW_FORMAT_SLOW5_BINARY) {
            ERROR("compression only available for output format '%s'", SLOW5_BINARY_NAME);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            record_press_out = name_to_slow5_press_method(arg_record_press_out);

            if (record_press_out == (enum slow5_press_method) -1) {
                ERROR("invalid compression method -- '%s'", arg_record_press_out);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }
    }

    if (arg_signal_press_out != NULL) {
        if (fmt_out != VIEW_FORMAT_SLOW5_BINARY) {
            ERROR("compression only available for output format '%s'", SLOW5_BINARY_NAME);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            signal_press_out = name_to_slow5_press_method(arg_signal_press_out);

            if (signal_press_out == (enum slow5_press_method) -1) {
                ERROR("invalid compression method -- '%s'", arg_signal_press_out);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }
    }

    // Parse output argument
    if (arg_fname_out != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opening output file%s","");
        }
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(arg_fname_out, "w");

        // An error occurred
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            f_out = new_file;
        }
    }

    // Do the conversion
    if ((fmt_in == VIEW_FORMAT_SLOW5_ASCII || fmt_in == VIEW_FORMAT_SLOW5_BINARY) &&
            (fmt_out == VIEW_FORMAT_SLOW5_ASCII || fmt_out == VIEW_FORMAT_SLOW5_BINARY)) {

        struct slow5_file *s5p = slow5_open_with(arg_fname_in, "r", (enum slow5_fmt) fmt_in);

        if (s5p == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_in, strerror(errno));
            view_ret = EXIT_FAILURE;
        }

        // TODO if output is the same format just duplicate file
        slow5_press_method_t press_out = {record_press_out,signal_press_out} ;
        if (slow5_convert_parallel(s5p, f_out, (enum slow5_fmt) fmt_out, press_out, num_threads, read_id_batch_capacity, meta) != 0) {
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
                  arg_fname_in, strerror(errno));
            view_ret = EXIT_FAILURE;
        }

    } else {
        view_ret = EXIT_FAILURE;
    }

    // Close output file
    if (arg_fname_out != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("closing output file%s","");
        }

        if (fclose(f_out) == EOF) {
            ERROR("File '%s' failed on closing - %s.",
                  arg_fname_out, strerror(errno));

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

    //
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

    if (meta->verbosity_level >= LOG_DEBUG) {
        DEBUG("time_get_to_mem\t%.3fs", time_get_to_mem);
        DEBUG("time_depress_parse\t%.3fs", time_thread_execution);
        DEBUG("time_write\t%.3fs", time_write);
    }

    return 0;
}
