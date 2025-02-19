/**
 * @file degrade.c
 * @brief degrade and convert S/BLOW5 files
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au), Sasha Jenner (me AT sjenner DOT com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 23/09/2024
 */
#include "slow5_misc.h"
#include "error.h"
#include "cmd.h"
#include "misc.h"
#include "thread.h"
#include "degrade.h"
#include <slow5/slow5.h>
#include "slow5_extra.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>

#define USAGE_MSG "Usage: %s [OPTIONS] [FILE]\n"
#define HELP_LARGE_MSG \
    "Irreversibly degrade and convert slow5/blow5 FILEs.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    HELP_MSG_OUTPUT_FORMAT_VIEW\
    HELP_MSG_OUTPUT_FILE \
    "    -c, --compress REC_MTD        record compression method [zlib] (only for blow5 format)\n" \
    "    -s, --sig-compress SIG_MTD    signal compression method [ex-zd] (only for blow5 format)\n" \
    HELP_MSG_THREADS \
    HELP_MSG_BATCH \
    "        --from FORMAT             specify input file format [auto]\n" \
    "    -b, --bits INT                specify the number of least significant bits to eliminate [auto]\n" \
    HELP_MSG_HELP \
    HELP_FORMATS_METHODS

extern int slow5tools_verbosity_level;

static inline int slow5_hdr_is_dataset(const struct slow5_hdr *h,
                                       const struct dataset *d);
static inline int slow5_rec_is_dataset(const struct slow5_rec *r,
                                       const struct dataset *d);
static inline uint8_t slow5_suggest_qts(const struct slow5_file *p,
                                        struct dataset *d);
static inline void slow5_hdrcmp_log(const char *a, uint32_t i, const char *x,
                                    const char *v);
static int slow5_convert_parallel(struct slow5_file *from, FILE *to_fp, enum slow5_fmt to_format, slow5_press_method_t to_compress, size_t num_threads, int64_t batch_size, struct program_meta *meta, uint8_t b, const struct dataset *d);
static int slow5_get_dataset(const struct slow5_file *p, struct dataset *d);
static int slow5_hdr_get_dataset(const struct slow5_hdr *h, struct dataset *d);
static int slow5_hdrcmp(const struct slow5_hdr *h, const char *a,
                        const char *x);
static int slow5_hdrcmp_sample_freq(const struct slow5_hdr *h, const char *f);
static int slow5_reccmp(const struct slow5_rec *r, float dig, float sr);
static int8_t parse_bits(const char *s);
static void depress_parse_rec_to_mem(core_t *core, db_t *db, int32_t i);

/*
 * Return a suggestion for the number of bits to use with qts degradation given
 * a slow5 file. Set d to the matching dataset.
 * d must not be NULL. Return 0 on error.
 */
static inline uint8_t slow5_suggest_qts(const struct slow5_file *p,
                                        struct dataset *d)
{
    if (!slow5_get_dataset(p, d))
        return d->q;
    return 0;
}

/*
 * Return whether or not the slow5 header represents the dataset.
 * h and d must not be NULL. Return 0 if false, 1 if true.
 */
static inline int slow5_hdr_is_dataset(const struct slow5_hdr *h,
                                       const struct dataset *d)
{
    return slow5_hdrcmp(h, SLOW5_HEADER_DEVICE_TYPE, d->dev) &&
           slow5_hdrcmp_sample_freq(h, d->freq) &&
           slow5_hdrcmp(h, SLOW5_HEADER_SEQUENCING_KIT, d->sqk) &&
           slow5_hdrcmp(h, SLOW5_HEADER_EXPERIMENT_TYPE, d->xpm);
}

/*
 * Return whether or not the slow5 record represents the dataset.
 * r and d must not be NULL. Return 0 if false, 1 if true.
 */
static inline int slow5_rec_is_dataset(const struct slow5_rec *r,
                                       const struct dataset *d)
{
    return slow5_reccmp(r, d->dig, d->rate);
}

/*
 * Log that header attribute a for read group i equals v instead of x.
 */
static inline void slow5_hdrcmp_log(const char *a, uint32_t i, const char *x,
                                    const char *v)
{
    if (!v) {
        DEBUG("Header at '%s' differs: missing but expected '%s'", a, x);
    } else {
        DEBUG("Header at '%s' differs (read group %" PRIu32 "): '%s' but expected '%s'",
             a, i, v, x);
    }
}

/*
 * Get the dataset which matches the slow5 file.
 * d must not be NULL. Return -1 on error, 0 on success and set d.
 */
static int slow5_get_dataset(const struct slow5_file *p, struct dataset *d)
{
    if (!p) {
        ERROR("Argument '%s' cannot be NULL", SLOW5_TO_STR(p));
        return -1;
    }

    return slow5_hdr_get_dataset(p->header, d);
}

/*
 * Get the dataset which matches the slow5 header.
 * d must not be NULL. Return -1 on error, 0 on success and set d.
 */
static int slow5_hdr_get_dataset(const struct slow5_hdr *h, struct dataset *d)
{
    uint i;
    const struct dataset ds[] = DATASETS;

    if (!h) {
        ERROR("Argument '%s' cannot be NULL", SLOW5_TO_STR(h));
        return -1;
    }

    i = 0;
    while (i < SLOW5_LENGTH(ds) && !slow5_hdr_is_dataset(h, ds + i)) {
        VERBOSE("Not detected: %s", ds[i].name);
        i++;
    }

    if (i == SLOW5_LENGTH(ds)) {
        ERROR("No suitable bits suggestion%s", "");
        return -1;
    }

    INFO("Detected: %s", ds[i].name);
    *d = ds[i];
    return 0;
}

/*
 * Check if the header data at attribute a is equal to x for all read groups.
 * All arguments must not be NULL. Return 0 if unequal, 1 if equal.
 */
static int slow5_hdrcmp(const struct slow5_hdr *h, const char *a,
                        const char *x)
{
    const char *v;
    uint32_t i;

    for (i = 0; i < h->num_read_groups; i++) {
        v = slow5_hdr_get(a, i, h);
        if (!v || strcmp(v, x)) {
            slow5_hdrcmp_log(a, i, x, v);
            return 0;
        }
    }

    return 1;
}

/*
 * Return whether or not a slow5 header's sample frequency/rate equals f.
 * Allow one to be missing from the header.
 * All arguments must not be NULL. Return 0 if false, 1 if true.
 */
static int slow5_hdrcmp_sample_freq(const struct slow5_hdr *h, const char *f)
{
    if (SLOW5_HEADER_IS_MISSING(h, SLOW5_HEADER_SAMPLE_FREQUENCY))
        return slow5_hdrcmp(h, SLOW5_HEADER_SAMPLE_RATE, f);

    return slow5_hdrcmp(h, SLOW5_HEADER_SAMPLE_FREQUENCY, f) &&
           (SLOW5_HEADER_IS_MISSING(h, SLOW5_HEADER_SAMPLE_RATE) ||
            slow5_hdrcmp(h, SLOW5_HEADER_SAMPLE_RATE, f));
}

/*
 * Return whether or not a slow5 record has the expected digitisation and
 * sampling rate. r must not be NULL. Return 0 if false, 1 if true.
 */
static int slow5_reccmp(const struct slow5_rec *r, float dig, float sr)
{
    if (r->digitisation != dig) {
        ERROR("Digitisation differs: %f but expected %f", r->digitisation,
              (float) dig);
    } else if (r->sampling_rate != sr) {
        ERROR("Sampling rate differs: %f but expected %f", r->sampling_rate,
              (float) sr);
    } else {
        return 1;
    }

    return 0;
}

/*
 * Parse the number of bits argument and return its value. Return -2 on error,
 * -1 if "auto", number of bits otherwise.
 */
static int8_t parse_bits(const char *s)
{
    char *p;
    long b;

    if (!s || *s == '\0') {
        ERROR("Invalid bits argument '%s'", s);
        return -2;
    }

    b = strtol(s, &p, 10);
    if (!*p) {
        if (b < 1 || b > 16) {
            ERROR("Invalid bits argument '%ld': outside of range 1-16", b);
            return -2;
        }
    } else if (!strcmp(s, "auto")) {
        b = -1;
    } else {
        ERROR("Invalid bits argument '%s'", s);
        return -2;
    }

    return (int8_t) b;
}

static void depress_parse_rec_to_mem(core_t *core, db_t *db, int32_t i) {
    //
    struct slow5_rec *read = NULL;
    const struct dataset *d;

    if (slow5_rec_depress_parse(&db->mem_records[i], &db->mem_bytes[i], NULL, &read, core->fp) != 0) {
        exit(EXIT_FAILURE);
    } else {
        free(db->mem_records[i]);
    }

    if (core->param) {
        d = (const struct dataset *) core->param;
        if (!slow5_rec_is_dataset(read, d)) {
            ERROR("Read with ID '%s' does not match %s", read->read_id,
                  d->name);
            exit(EXIT_FAILURE);
        }
    }

    slow5_rec_qts_round(read, (uint8_t) core->lossy);

    struct slow5_press *press_ptr = slow5_press_init(core->press_method);
    if(!press_ptr){
        ERROR("Could not initialize the slow5 compression method%s","");
        exit(EXIT_FAILURE);
    }
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

int degrade_main(int argc, char **argv, struct program_meta *meta) {
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
        {"to",              required_argument,  NULL, 'T'},
        {"threads",         required_argument,  NULL, 't' },
        {"batchsize",       required_argument, NULL, 'K'},
        {"bits",            required_argument, NULL, 'b'},
        {NULL, 0, NULL, 0}
    };

    opt_t user_opts;
    init_opt(&user_opts);

    user_opts.signal_press_out = SLOW5_COMPRESS_EX_ZD;

    int opt;
    int longindex = 0;
    int8_t b = -1;
    struct dataset d;
    struct dataset *dp = NULL;

    // Parse options
    while ((opt = getopt_long(argc, argv, "s:c:f:ho:T:t:K:b:", long_opts, &longindex)) != -1) {
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
            case 'T':
                user_opts.arg_fmt_out = optarg;
                break;
            case 't':
                user_opts.arg_num_threads = optarg;
                break;
            case 'b':
                b = parse_bits(optarg);
                if (b == -2) {
                    EXIT_MSG(EXIT_FAILURE, argv, meta);
                    return EXIT_FAILURE;
                } else if (b > 4) {
                    WARNING("%s", "bits > 4: basecalling accuracy may be adversely affected!");
                }
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
        ERROR("more than 1 input file is given%s", "");
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

    WARNING("This tool performs lossy compression which is an irreversible operation. Just making sure it is intended. %s", "");

    // Do the conversion
    if ((user_opts.fmt_in == SLOW5_FORMAT_ASCII || user_opts.fmt_in == SLOW5_FORMAT_BINARY) &&
            (user_opts.fmt_out == SLOW5_FORMAT_ASCII || user_opts.fmt_out == SLOW5_FORMAT_BINARY)) {

        struct slow5_file *s5p = slow5_open_with(user_opts.arg_fname_in, "r", (enum slow5_fmt) user_opts.fmt_in);

        if (s5p == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  user_opts.arg_fname_in, strerror(errno));
            view_ret = EXIT_FAILURE;
        }

        if (b == -1) {
            b = slow5_suggest_qts(s5p, &d);
            if (!b) {
                ERROR("%s", "Use option -b to manually specify");
                view_ret = EXIT_FAILURE;
                goto err;
            }
            dp = &d;
            INFO("Eliminating %" PRId8 " bits", b);
        }

        // TODO if output is the same format just duplicate file
        slow5_press_method_t press_out = {user_opts.record_press_out,user_opts.signal_press_out};
        if (slow5_convert_parallel(s5p, user_opts.f_out, (enum slow5_fmt) user_opts.fmt_out, press_out, user_opts.num_threads, user_opts.read_id_batch_capacity, meta, (uint8_t) b, dp) != 0) {
            ERROR("File conversion failed.%s", "");
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

err:
    if (view_ret == EXIT_FAILURE) {
        EXIT_MSG(EXIT_FAILURE, argv, meta);
    }
    return view_ret;
}

static int slow5_convert_parallel(struct slow5_file *from, FILE *to_fp, enum slow5_fmt to_format, slow5_press_method_t to_compress, size_t num_threads, int64_t batch_size, struct program_meta *meta, uint8_t b, const struct dataset *d) {
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
        db.mem_records = (char **) malloc(batch_size * sizeof(char*));
        db.mem_bytes = (size_t *) malloc(batch_size * sizeof(size_t));
        MALLOC_CHK(db.mem_records);
        MALLOC_CHK(db.mem_bytes);
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
        core.lossy = (int) b;
        core.param = (void *) d;

        db.n_batch = record_count;
        db.read_record = (raw_record_t*) malloc(record_count * sizeof *db.read_record);
        MALLOC_CHK(db.read_record);
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

    return 0;
}
