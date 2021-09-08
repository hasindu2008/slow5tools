/**
 * @file misc.c
 * @brief miscellaneous common functions
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au) Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 31/08/2021
 */
#include "misc.h"
#include "cmd.h"

extern int slow5tools_verbosity_level;


void print_args(int argc, char **argv){

    // Debug: print arguments
    DEBUG("printing the arguments given%s","");
    if(slow5tools_verbosity_level >= LOG_DEBUG){
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
}


void init_opt(opt_t *opt){


    // Input arguments
    opt->arg_fname_in = NULL;
    opt->arg_fname_out = NULL;
    opt->arg_fmt_in = NULL;
    opt->arg_fmt_out = NULL;
    opt->arg_record_press_out = NULL;
    opt->arg_signal_press_out = NULL;
    opt->arg_num_threads = NULL;

    // Default options
    opt->fmt_in = SLOW5_FORMAT_UNKNOWN;
    opt->fmt_out = SLOW5_FORMAT_UNKNOWN;
    opt->f_out = stdout;
    opt->record_press_out = SLOW5_COMPRESS_ZLIB;
    opt->signal_press_out = SLOW5_COMPRESS_NONE;
    opt->num_threads = DEFAULT_NUM_THREADS;
    opt->read_id_batch_capacity = READ_ID_BATCH_CAPACITY;
}

int parse_num_threads(opt_t *opt, int argc, char **argv, struct program_meta *meta){
    // Parse num threads argument
    if (opt->arg_num_threads != NULL) {
        char *endptr;
        long ret = strtol(opt->arg_num_threads, &endptr, 10);

        if (*endptr == '\0') {
            opt->num_threads = ret;
        } else {
            ERROR("invalid number of threads -- '%s'", opt->arg_num_threads);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);
            return -1;
        }
    }
    return 0;
}

int parse_format_args(opt_t *opt, int argc, char **argv, struct program_meta *meta){
    // Parse format arguments
    if (opt->arg_fmt_in != NULL) {
        DEBUG("parsing input format%s","");
        opt->fmt_in = parse_name_to_fmt(opt->arg_fmt_in);

        // An error occured
        if (opt->fmt_in == SLOW5_FORMAT_UNKNOWN) {
            ERROR("invalid input format -- '%s'", opt->arg_fmt_in);
            return -1;
        }
    }
    if (opt->arg_fmt_out != NULL) {
        DEBUG("parsing output format%s","");
        opt->fmt_out = parse_name_to_fmt(opt->arg_fmt_out);

        // An error occured
        if (opt->fmt_out == SLOW5_FORMAT_UNKNOWN) {
            ERROR("invalid output format -- '%s'", opt->arg_fmt_out);
            return -1;
        }
    }
    return 0;
}


int auto_detect_formats(opt_t *opt){
    // Autodetect input/output formats
    if (opt->fmt_in == SLOW5_FORMAT_UNKNOWN) {
        DEBUG("auto detecting input file format%s","");
        opt->fmt_in = parse_path_to_fmt(opt->arg_fname_in);
        // Error
        if (opt->fmt_in == SLOW5_FORMAT_UNKNOWN) {
            ERROR("cannot detect file format -- '%s'",opt-> arg_fname_in);
            return -1;
        }
    }
    if (opt->fmt_out == SLOW5_FORMAT_UNKNOWN) {
        if (opt->arg_fname_out == NULL) {
            opt->fmt_out = SLOW5_FORMAT_ASCII;
        }

        else{
            DEBUG("auto detecting output file format%s","");

            opt->fmt_out = parse_path_to_fmt(opt->arg_fname_out);

            // Error
            if (opt->fmt_out == SLOW5_FORMAT_UNKNOWN) {
                ERROR("cannot detect file format -- '%s'", opt->arg_fname_out);
                return -1;
            }
        }
    }
    return 0;

}


int parse_compression_opts(opt_t *opt){
    if (opt->arg_record_press_out != NULL) {
        if (opt->fmt_out != SLOW5_FORMAT_BINARY) {
            ERROR("compression only available for output format '%s'", SLOW5_BINARY_NAME);
            return -1;
        } else {
            opt->record_press_out = name_to_slow5_press_method(opt->arg_record_press_out);

            if (opt->record_press_out == (enum slow5_press_method) -1) {
                ERROR("invalid compression method -- '%s'", opt->arg_record_press_out);
                return -1;
            }
        }
    }

    if (opt->arg_signal_press_out != NULL) {
        if (opt->fmt_out != SLOW5_FORMAT_BINARY) {
            ERROR("compression only available for output format '%s'", SLOW5_BINARY_NAME);
            return -1;
        } else {
            opt->signal_press_out = name_to_slow5_press_method(opt->arg_signal_press_out);

            if (opt->signal_press_out == (enum slow5_press_method) -1) {
                ERROR("invalid compression method -- '%s'", opt->arg_signal_press_out);
                return -1;
            }
        }
    }

    return 0;
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

enum slow5_fmt parse_name_to_fmt(const char *fmt_str) {
    enum slow5_fmt fmt = SLOW5_FORMAT_UNKNOWN;
    for (size_t i = 0; i < sizeof PARSE_FORMAT_META / sizeof PARSE_FORMAT_META[0]; ++ i) {
        const struct parse_fmt_meta meta = PARSE_FORMAT_META[i];
        if (strcmp(meta.name, fmt_str) == 0) {
            fmt = meta.format;
            break;
        }
    }
    return fmt;
}

enum slow5_fmt parse_path_to_fmt(const char *fname) {
    enum slow5_fmt fmt = SLOW5_FORMAT_UNKNOWN;
    for (int i = strlen(fname) - 1; i >= 0; -- i) {
        if (fname[i] == '.') {
            const char *ext = fname + i;
            for (size_t j = 0; j < sizeof PARSE_FORMAT_META / sizeof PARSE_FORMAT_META[0]; ++ j) {
                const struct parse_fmt_meta meta = PARSE_FORMAT_META[j];
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

int check_aux_fields_in_header(slow5_hdr *slow5_header, const char *attr, int verbose){
    if(slow5_header->aux_meta->num == 0){
        if(verbose){
            ERROR("Header does not have auxiliary fields%s", "");
        }
        return -1;
    }
    khint_t pos = kh_get(slow5_s2ui32, slow5_header->aux_meta->attr_to_pos, attr);
    if(pos == kh_end(slow5_header->aux_meta->attr_to_pos)){
        if(verbose){
            ERROR("Auxiliary field '%s' not found.", attr);
        }
        return -1;
    }
    return 0;
}
