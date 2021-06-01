#include "slow5_misc.h"
#include "error.h"
#include "cmd.h"
#include <slow5/slow5.h>
#include <getopt.h>

#define USAGE_MSG "Usage: %s [OPTION]... [FILE]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "View a fast5 or slow5 FILE in another format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -f, --from=[FORMAT]        specify input file format\n" \
    "    -t, --to=[FORMAT]          specify output file format\n" \
    "    -c, --compress=[METHOD]    specify output compression method -- gzip (only available for format blow5)\n" \
    "    -o, --output=[FILE]        output to FILE -- stdout\n" \
    "    -h, --help                 display this message and exit\n" \
    "FORMATS:\n" \
    "    slow5\n" \
    "    blow5\n" \
    "    fast5  (not implemented yet)\n" \
    "METHODS:\n" \
    "    none\n" \
    "    gzip -- default\n"

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
    { VIEW_FORMAT_SLOW5_ASCII,  ASCII_NAME,     ASCII_EXTENSION     },
    { VIEW_FORMAT_SLOW5_BINARY, BINARY_NAME,    BINARY_EXTENSION    },
};

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

press_method_t name_to_press_method(const char *name) {
    press_method_t comp = (press_method_t) -1;

    if (strcmp(name, "none") == 0) {
        comp = COMPRESS_NONE;
    } else if (strcmp(name, "gzip") == 0) {
        comp = COMPRESS_GZIP;
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
        {NULL, 0, NULL, 0}
    };

    // Default options
    FILE *f_out = stdout;
    enum view_fmt fmt_in = VIEW_FORMAT_UNKNOWN;
    enum view_fmt fmt_out = VIEW_FORMAT_UNKNOWN;
    press_method_t press_out = COMPRESS_GZIP;

    // Input arguments
    char *arg_fname_in = NULL;
    char *arg_fname_out = NULL;
    char *arg_fmt_in = NULL;
    char *arg_fmt_out = NULL;
    char *arg_press_out = NULL;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "c:f:ho:t:", long_opts, &longindex)) != -1) {
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
            default: // case '?'
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
            MESSAGE(stderr, "missing output file or format%s", "");
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }

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


    if (arg_press_out != NULL) {
        if (fmt_out != VIEW_FORMAT_SLOW5_BINARY) {
            MESSAGE(stderr, "compression only available for output format '%s'", BINARY_NAME);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            press_out = name_to_press_method(arg_press_out);

            if (press_out == (press_method_t) -1) {
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
        if (slow5_convert(s5p, f_out, (enum slow5_fmt) fmt_out, press_out) != 0) {
            ERROR("Conversion failed.%s", "");
            view_ret = EXIT_FAILURE;
        }

        if (slow5_close(s5p) == EOF) {
            ERROR("File '%s' failed on closing - %s.",
                  arg_fname_in, strerror(errno));
            view_ret = EXIT_FAILURE;
        }

    } else { // TODO support fast5
        MESSAGE(stderr, "fast5 conversion not implemented%s", "");
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
