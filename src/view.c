#include "slow5_misc.h"
#include "error.h"
#include "cmd.h"
#include "misc.h"
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
    "    -f, --from=[FORMAT]        specify input file format\n" \
    "    -t, --to=[FORMAT]          specify output file format\n" \
    "    -c, --compress=[METHOD]    specify output compression method -- zlib (only available for format blow5)\n" \
    "    -o, --output=[FILE]        output to FILE -- stdout\n" \
    "    -h, --help                 display this message and exit\n"                                               \
    "    -@, --threads [INT]        number of threads [default: 4]\n"                                              \
    "    -K --batchsize                     the number of records on the memory at once. [default: 4096]\n" \
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

struct Record {
    int len;
    void* buffer;
};
/* core data structure that has information that are global to all the threads */
typedef struct {
    int32_t num_thread;
    std::string output_dir;
    std::vector<std::vector<size_t>> list;
    int lossy;
    int64_t n_batch;    // number of files
    slow5_fmt format_out;
    slow5_press_method_t press_method;
} global_thread;

/* argument wrapper for the multithreaded framework used for data processing */
typedef struct {
    global_thread * core;
    int32_t starti;
    int32_t endi;
    int32_t thread_index;
} pthread_arg;

int slow5_convert_parallel(struct slow5_file *from, FILE *to_fp, enum slow5_fmt to_format, slow5_press_method_t to_compress, size_t num_threads, int64_t batch_size);
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

slow5_press_method_t name_to_slow5_press_method(const char *name) {
    slow5_press_method_t comp = (slow5_press_method_t) -1;

    if (strcmp(name, "none") == 0) {
        comp = SLOW5_COMPRESS_NONE;
    } else if (strcmp(name, "zlib") == 0) {
        comp = SLOW5_COMPRESS_ZLIB;
    }

    return comp;
}

//static double init_realtime = 0;

int view_main(int argc, char **argv, struct program_meta *meta) {
    int view_ret = EXIT_SUCCESS;

    //init_realtime = slow5_realtime();

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
        {"compress",    required_argument,  NULL, 'c'},
        {"from",        required_argument,  NULL, 'f'},
        {"help",        no_argument,        NULL, 'h'},
        {"output",      required_argument,  NULL, 'o'},
        {"to",          required_argument,  NULL, 't'},
        {"threads",     required_argument,  NULL, '@' },
        {"batchsize", required_argument, NULL, 'K'},
        {NULL, 0, NULL, 0}
    };

    // Default options
    FILE *f_out = stdout;
    enum view_fmt fmt_in = VIEW_FORMAT_UNKNOWN;
    enum view_fmt fmt_out = VIEW_FORMAT_UNKNOWN;
    slow5_press_method_t press_out = SLOW5_COMPRESS_ZLIB;

    // Input arguments
    char *arg_fname_in = NULL;
    char *arg_fname_out = NULL;
    char *arg_fmt_in = NULL;
    char *arg_fmt_out = NULL;
    char *arg_press_out = NULL;
    char *arg_num_threads = NULL;
    size_t num_threads = DEFAULT_NUM_THREADS;
    int64_t read_id_batch_capacity = READ_ID_BATCH_CAPACITY;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "c:f:ho:t:@:K:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'c':
                arg_press_out = optarg;
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
                if (meta->verbosity_level >= LOG_VERBOSE) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'o':
                arg_fname_out = optarg;
                break;
            case 't':
                arg_fmt_out = optarg;
                break;
            case '@':
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
            MESSAGE(stderr, "invalid number of threads -- '%s'", arg_num_threads);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    // Parse format arguments
    if (arg_fmt_in != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_VERBOSE) {
            VERBOSE("parsing input format%s","");
        }

        fmt_in = name_to_view_fmt(arg_fmt_in);

        // An error occured
        if (fmt_in == VIEW_FORMAT_UNKNOWN) {
            MESSAGE(stderr, "invalid input format -- '%s'", arg_fmt_in);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if (arg_fmt_out != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_VERBOSE) {
            VERBOSE("parsing output format%s","");
        }

        fmt_out = name_to_view_fmt(arg_fmt_out);

        // An error occured
        if (fmt_out == VIEW_FORMAT_UNKNOWN) {
            MESSAGE(stderr, "invalid output format -- '%s'", arg_fmt_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    // Check for an input file to parse
    if (optind >= argc) { // TODO use stdin if no file given
        MESSAGE(stderr, "missing input file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    } else if (optind != argc - 1) { // TODO handle more than 1 file?
        MESSAGE(stderr, ">1 input file%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    } else { // Save input filename
        arg_fname_in = argv[optind];
    }

    // Autodetect input/output formats
    if (fmt_in == VIEW_FORMAT_UNKNOWN) {
        if (meta != NULL && meta->verbosity_level >= LOG_VERBOSE) {
            VERBOSE("auto detecting input file format%s","");
        }

        fmt_in = path_to_view_fmt(arg_fname_in);

        // Error
        if (fmt_in == VIEW_FORMAT_UNKNOWN) {
            MESSAGE(stderr, "cannot detect file format -- '%s'", arg_fname_in);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if (fmt_out == VIEW_FORMAT_UNKNOWN) {
        if (arg_fname_out == NULL) {
            // Error
            //MESSAGE(stderr, "missing output file or format%s", "");
            //fprintf(stderr, HELP_SMALL_MSG, argv[0]);
            //EXIT_MSG(EXIT_FAILURE, argv, meta);
            //return EXIT_FAILURE;
            fmt_out = VIEW_FORMAT_SLOW5_ASCII;
        }

        else{
            if (meta != NULL && meta->verbosity_level >= LOG_VERBOSE) {
                VERBOSE("auto detecting output file format%s","");
            }

            fmt_out = path_to_view_fmt(arg_fname_out);

            // Error
            if (fmt_out == VIEW_FORMAT_UNKNOWN) {
                MESSAGE(stderr, "cannot detect file format -- '%s'", arg_fname_out);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }
    }


    if (arg_press_out != NULL) {
        if (fmt_out != VIEW_FORMAT_SLOW5_BINARY) {
            MESSAGE(stderr, "compression only available for output format '%s'", SLOW5_BINARY_NAME);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            press_out = name_to_slow5_press_method(arg_press_out);

            if (press_out == (slow5_press_method_t) -1) {
                MESSAGE(stderr, "invalid compression method -- '%s'", arg_press_out);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }
    }

    // Parse output argument
    if (arg_fname_out != NULL) {
        if (meta != NULL && meta->verbosity_level >= LOG_VERBOSE) {
            VERBOSE("opening output file%s","");
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
        if (slow5_convert_parallel(s5p, f_out, (enum slow5_fmt) fmt_out, press_out, num_threads, read_id_batch_capacity) != 0) {
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
        if (meta != NULL && meta->verbosity_level >= LOG_VERBOSE) {
            VERBOSE("closing output file%s","");
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

// Return
// 0    success
// -1   input invalid
// -2   failure
int slow5_convert_parallel(struct slow5_file *from, FILE *to_fp, enum slow5_fmt to_format, slow5_press_method_t to_compress, size_t num_threads, int64_t batch_size) {
    if (from == NULL || to_fp == NULL || to_format == SLOW5_FORMAT_UNKNOWN) {
        return -1;
    }

    if (slow5_hdr_fwrite(to_fp, from->header, to_format, to_compress) == -1) {
        return -2;
    }

    //
    double time_get_to_mem = 0;
    double time_parse = 0;
    double time_rec_to_mem = 0;
    double time_write = 0;
    int flag_end_of_file = 0;
    while(1) {
        int64_t record_count = 0;
        std::vector<char *> mem_records;
        std::vector<size_t> mem_bytes;
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
                mem_records.push_back(mem);
                mem_bytes.push_back(bytes);
                record_count++;
            }
        }
        time_get_to_mem += slow5_realtime() - realtime;
        realtime = slow5_realtime();
        std::vector<struct slow5_rec *> read_records;
        struct slow5_rec *read = NULL;
        for (size_t i = 0; i < mem_records.size(); i++) {
            if (slow5_rec_depress_parse(&mem_records[i], &mem_bytes[i], NULL, &read, from) != 0) {
                return EXIT_FAILURE;
            } else {
                read_records.push_back(read);
                free(mem_records[i]);
                read = NULL;
            }
        }
        time_parse += slow5_realtime() - realtime;
        realtime = slow5_realtime();

        std::vector<Record>db(record_count);
        struct slow5_press *press_ptr = slow5_press_init(to_compress);
        size_t len;
        for (size_t i = 0; i < read_records.size(); i++) {
            if ((db[i].buffer = slow5_rec_to_mem(read_records[i], from->header->aux_meta, to_format, press_ptr, &len)) == NULL) {
                slow5_press_free(press_ptr);
                slow5_rec_free(read_records[i]);
                return EXIT_FAILURE;
            }
            db[i].len = len;
            slow5_rec_free(read_records[i]);
        }
        slow5_press_free(press_ptr);
        time_rec_to_mem += slow5_realtime() - realtime;
        realtime = slow5_realtime();

        for (size_t i = 0; i < db.size(); i++) {
            fwrite(db[i].buffer,1,db[i].len,to_fp);
            free(db[i].buffer);
        }
        time_write += slow5_realtime() - realtime;
        if(flag_end_of_file == 1){
            break;
        }
    }
    if (to_format == SLOW5_FORMAT_BINARY) {
        if (slow5_eof_fwrite(to_fp) == -1) {
            return -2;
        }
    }

    INFO("time_get_to_mem\t%.3fs", time_get_to_mem);
    INFO("time_depress_parse\t%.3fs", time_parse);
    INFO("time_get_to_rec\t%.3fs", time_rec_to_mem);
    INFO("time_write\t%.3fs", time_write);

    /*
    // Setup multithreading structures
    global_thread core;
    core.num_thread = num_threads;
    core.list = list;
    core.lossy = lossy;
    core.n_batch = slow5_files.size();
    core.format_out = to_format;
    core.press_method = to_compress;

    //measure read_group number assigning using multi-threads time
    realtime0 = slow5_realtime();

    // Fetch records for read ids in the batch
    work_data(&core);

    fprintf(stderr, "[%s] Assigning new read group numbers using %ld threads - took %.3fs\n", __func__, num_threads, slow5_realtime() - realtime0);

    //
    struct slow5_rec *read = NULL;
    int ret;
    struct slow5_press *press_ptr = slow5_press_init(to_compress);
    while ((ret = slow5_get_next(&read, from)) == 0) {
        if (slow5_rec_fwrite(to_fp, read, from->header->aux_meta, to_format, press_ptr) == -1) {
            slow5_press_free(press_ptr);
            slow5_rec_free(read);
            return -2;
        }
    }
    slow5_press_free(press_ptr);
    slow5_rec_free(read);
    if (ret != SLOW5_ERR_EOF) {
        return -2;
    }

    if (to_format == SLOW5_FORMAT_BINARY) {
        if (slow5_eof_fwrite(to_fp) == -1) {
            return -2;
        }
    }
    */

    return 0;
}