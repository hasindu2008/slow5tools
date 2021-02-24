#include "misc.h"
#include "cmd.h"
#include "error.h"
#include <getopt.h>

#define USAGE_MSG "Usage: %s [OPTION]... [FROM_FILE]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Convert a fast5 or slow5 file to a specified format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -f, --from=[FROM_FORMAT]   specify the FROM_FILE format\n" \
    "    -o, --output=[TO_FILE]     output to TO_FILE -- stdout\n" \
    "    -t, --to=[TO_FORMAT]       specify the TO_FILE format\n" \
    "FORMATS:\n" \
    "    blow5\n" \
    "    fast5  (not implemented yet)\n" \
    "    slow5\n"

static double init_realtime = 0;

int view_main(int argc, char **argv, struct program_meta *meta) {
    init_realtime = realtime();

    //int ret; // For checking return values of functions

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

    /*
    static struct option long_opts[] = {
        {"from",    required_argument, NULL, 'f'},
        {"output",  required_argument, NULL, 'o'},
        {"to",      required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };
    */

    /*
    // Default options
    FILE *f_out = stdout;
    enum FormatOut format_out = OUT_ASCII;
    FILE *f_idx = NULL;

    // Input arguments
    char *arg_fname_out = NULL;
    char *arg_dir_out = NULL;
    char *arg_fname_idx = NULL;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "bchi:o:d:", long_opts, &longindex)) != -1) {
        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'b':
                format_out = OUT_BINARY;
                break;
            case 'c':
                format_out = OUT_COMP;
                break;
            case 'h':
                if (meta->verbose) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                return EXIT_SUCCESS;
            case 'i':
                arg_fname_idx = optarg;
                break;
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'd':
                arg_dir_out = optarg;
                break;
            case  0 :
                if (longindex == 5) {
                    iop = atoi(optarg);
                    if (iop < 1) {
                        ERROR("Number of I/O processes should be larger than 0. You entered %d", iop);
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    if(iop>1 && !arg_dir_out){
        ERROR("output directory should be specified when using multiprocessing iop=%d",iop);
        return EXIT_FAILURE;
    }

    // Parse output argument
    if (arg_fname_out != NULL) {
        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing output filename%s","");
        }
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(arg_fname_out, "w");

        // An error occured
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            f_out = new_file;
        }
    }

    // Parse index argument
    if (arg_fname_idx != NULL) {
        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing index filename%s","");
        }
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(arg_fname_idx, "w");

        // An error occured
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_idx, strerror(errno));
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            f_idx = new_file;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if (format_out == OUT_COMP) {
    }
    // Output slow5 header
    switch (format_out) {
        case OUT_ASCII:
//            fprintf(f_out, SLOW5_FILE_FORMAT);
//            fprintf(f_out, SLOW5_HEADER);
            break;
        case OUT_BINARY:
            fprintf(f_out, BLOW5_FILE_FORMAT);
            fprintf(f_out, SLOW5_HEADER);
            break;
        case OUT_COMP:
            // Initialise zlib stream structure
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;

            ret = deflateInit2(&strm,
                    Z_DEFAULT_COMPRESSION,
                    Z_DEFLATED,
                    MAX_WBITS | GZIP_WBITS, // Gzip compatible compression
                    Z_MEM_DEFAULT,
                    Z_DEFAULT_STRATEGY);
            if (ret != Z_OK) {
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }

            char header[] = BLOW5_FILE_FORMAT SLOW5_HEADER;
            ret = z_deflate_write(&strm, header, strlen(header), f_out, Z_FINISH);
            if (ret != Z_STREAM_END) {
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
            ret = deflateReset(&strm);
            if (ret != Z_OK) {
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
            break;
    }

    double realtime0 = realtime();
    reads_count readsCount;
    std::vector<std::string> fast5_files;

    for (int i = optind; i < argc; ++ i) {
        if(iop==1){
            // Recursive way
            recurse_dir(argv[i], f_out, format_out, &strm, f_idx, &readsCount, arg_dir_out, meta);
        }else{
            find_all_5(argv[i], fast5_files, FAST5_EXTENSION);
        }
    }

    if(iop==1){
        MESSAGE(stderr, "total fast5: %lu, bad fast5: %lu", readsCount.total_5, readsCount.bad_5_file);
    }else{
        fprintf(stderr, "[%s] %ld fast5 files found - took %.3fs\n", __func__, fast5_files.size(), realtime() - realtime0);
        f2s_iop(f_out, format_out, &strm, f_idx, iop, fast5_files, arg_dir_out, meta, &readsCount);
    }

    if (format_out == OUT_COMP) {
        ret = deflateEnd(&strm);

        if (ret != Z_OK) {
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    // Close output file
    if (arg_fname_out != NULL && fclose(f_out) == EOF) {
        ERROR("File '%s' failed on closing - %s.",
              arg_fname_out, strerror(errno));

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if (arg_fname_idx != NULL && fclose(f_idx) == EOF) {
        ERROR("File '%s' failed on closing - %s.",
              arg_fname_idx, strerror(errno));

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    */

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
