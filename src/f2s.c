// Sasha Jenner

#include "fast5lite.h"
#include "slow5.h"

#define USAGE_MSG "Usage: %s [OPTION]... [FAST5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Convert fast5 file(s) to slow5 or (compressed) blow5.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -b, --binary           convert to blow5\n" \
    "    -c, --compress         convert to compressed blow5\n" \
    "    -h, --help             display this message and exit\n" \
    "    -i, --index=[FILE]     index converted file to FILE -- not default\n" \
    "    -o, --output=[FILE]    output converted contents to FILE -- stdout\n" \

static double init_realtime = 0;
static uint64_t bad_fast5_file = 0;
static uint64_t total_reads = 0;


// adapted from https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
/*
bool is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == -1) {
        ERROR("Stat failed to retrive file information%s", "");
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}
*/

// Operator function to be called by H5Literate.
herr_t op_func_attr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *operator_data);
// Operator function to be called by H5Aiterate.
herr_t op_func_group (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);
// function to read signal data
int read_dataset(hid_t dset, const char *name, slow5_record_t* slow5_record);
//other functions
int group_check(struct operator_obj *od, haddr_t target_addr);
void reset_attributes(group_flags group_flag, operator_obj* operator_data);
void free_attributes(group_flags group_flag, operator_obj* operator_data);

int z_deflate_write(z_streamp strmp, const void *ptr, uLong size, FILE *f_out, int flush) {
    int ret = Z_OK;

    strmp->avail_in = size;
    strmp->next_in = (Bytef *) ptr;

    uLong out_sz = Z_OUT_CHUNK;
    unsigned char *out = (unsigned char *) malloc(sizeof *out * out_sz);

    do {
        strmp->avail_out = out_sz;
        strmp->next_out = out;

        ret = deflate(strmp, flush);
        if (ret == Z_STREAM_ERROR) {
            ERROR("deflate failed\n%s", ""); // testing
            return ret;
        }

        unsigned have = out_sz - strmp->avail_out;
        if (fwrite(out, sizeof *out, have, f_out) != have || ferror(f_out)) {
            ERROR("fwrite\n%s", ""); // testing
            return Z_ERRNO;
        }

    } while (strmp->avail_out == 0);

    free(out);
    out = NULL;

    // If still input to deflate
    if (strmp->avail_in != 0) {
        ERROR("still more input to deflate\n%s", ""); // testing
        return Z_ERRNO;
    }

    return ret;
}

bool has_fast5_ext(const char *f_path) {
    bool ret = false;

    if (f_path != NULL) {
        size_t f_path_len = strlen(f_path);
        size_t fast5_ext_len = strlen(FAST5_EXTENSION);

        if (f_path_len >= fast5_ext_len &&
                strcmp(f_path + (f_path_len - fast5_ext_len), FAST5_EXTENSION) == 0) {
            ret = true;
        }
    }

    return ret;
}

void write_data(FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx,
        const std::string read_id, const fast5_t f5, const char *fast5_path) {

    off_t start_pos = -1;
    if (f_idx != NULL) {
        start_pos = ftello(f_out);
        fprintf(f_idx, "%s\t%ld\t", read_id.c_str(), start_pos);
    }

    // Interpret file parameter
    switch (format_out) {
        case OUT_ASCII: {

            fprintf(f_out, "%s\t%ld\t%.1f\t%.1f\t%.1f\t%.1f\t", read_id.c_str(),
                    f5.nsample,f5.digitisation, f5.offset, f5.range, f5.sample_rate);

            for (uint64_t j = 0; j < f5.nsample; ++ j) {
                if (j == f5.nsample - 1) {
                    fprintf(f_out, "%hu", f5.rawptr[j]);
                } else {
                    fprintf(f_out, "%hu,", f5.rawptr[j]);
                }
            }

            fprintf(f_out, "\t%d\t%s\t%s\n", 0, ".", fast5_path);
        } break;

        case OUT_BINARY: {
            // write length of string
            size_t read_id_len = read_id.length();
            fwrite(&read_id_len, sizeof read_id_len, 1, f_out);

            // write string
            const char *read_id_c_str = read_id.c_str();
            fwrite(read_id_c_str, sizeof *read_id_c_str, read_id_len, f_out);

            // write other data
            fwrite(&f5.nsample, sizeof f5.nsample, 1, f_out);
            fwrite(&f5.digitisation, sizeof f5.digitisation, 1, f_out);
            fwrite(&f5.offset, sizeof f5.offset, 1, f_out);
            fwrite(&f5.range, sizeof f5.range, 1, f_out);
            fwrite(&f5.sample_rate, sizeof f5.sample_rate, 1, f_out);

            fwrite(f5.rawptr, sizeof *f5.rawptr, f5.nsample, f_out);

            //todo change to variable

            uint64_t num_bases = 0;
            fwrite(&num_bases, sizeof num_bases, 1, f_out);

            const char *sequences = ".";
            size_t sequences_len = strlen(sequences);
            fwrite(&sequences_len, sizeof sequences_len, 1, f_out);
            fwrite(sequences, sizeof *sequences, sequences_len, f_out);

            size_t fast5_path_len = strlen(fast5_path);
            fwrite(&fast5_path_len, sizeof fast5_path_len, 1, f_out);
            fwrite(fast5_path, sizeof *fast5_path, fast5_path_len, f_out);
         } break;

        case OUT_COMP: {
            // write length of string
            size_t read_id_len = read_id.length();
            z_deflate_write(strmp, &read_id_len, sizeof read_id_len, f_out, Z_NO_FLUSH);

            // write string
            const char *read_id_c_str = read_id.c_str();
            z_deflate_write(strmp, read_id_c_str, sizeof *read_id_c_str * read_id_len, f_out, Z_NO_FLUSH);

            // write other data
            z_deflate_write(strmp, &f5.nsample, sizeof f5.nsample, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.digitisation, sizeof f5.digitisation, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.offset, sizeof f5.offset, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.range, sizeof f5.range, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.sample_rate, sizeof f5.sample_rate, f_out, Z_NO_FLUSH);

            z_deflate_write(strmp, f5.rawptr, sizeof *f5.rawptr * f5.nsample, f_out, Z_NO_FLUSH);

            //todo change to variable

            uint64_t num_bases = 0;
            z_deflate_write(strmp, &num_bases, sizeof num_bases, f_out, Z_NO_FLUSH);

            const char *sequences = ".";
            size_t sequences_len = strlen(sequences);
            z_deflate_write(strmp, &sequences_len, sizeof sequences_len, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, sequences, sizeof *sequences * sequences_len, f_out, Z_NO_FLUSH);

            size_t fast5_path_len = strlen(fast5_path);
            z_deflate_write(strmp, &fast5_path_len, sizeof fast5_path_len, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, fast5_path, sizeof *fast5_path * fast5_path_len, f_out, Z_FINISH);

            int ret = deflateReset(strmp);

            if (ret != Z_OK) {
                ERROR("deflateReset failed\n%s", ""); // testing
            }
        } break;
    }

    if (f_idx != NULL) {
        off_t end_pos = ftello(f_out);
        fprintf(f_idx, "%ld\n", end_pos - start_pos);
    }

}

int fast5_to_slow5(const char *fast5_path, FILE *f_out, enum FormatOut format_out,
        z_streamp strmp, FILE *f_idx) {
    total_reads++;


    fast5_file_t fast5_file = fast5_open(fast5_path);

    if (fast5_file.hdf5_file >= 0) {

        //TODO: can optimise for performance
        if (fast5_file.is_multi_fast5) {
            size_t number_reads = 0;
            H5O_info_t infobuf;
            struct operator_obj tracker;
            tracker.group_level = ROOT;
            tracker.prev = NULL;
            H5Oget_info(fast5_file.hdf5_file, &infobuf);
            tracker.addr = infobuf.addr;

            tracker.f_out = f_out;
            tracker.format_out = format_out;
            tracker.strmp = strmp;
            tracker.f_idx = f_idx;
            tracker.fast5_path = fast5_path;

            slow5_header_t slow5_header;
            slow5_record_t slow5_record;
            int flag_context_tags = 0;
            int flag_tracking_id = 0;
            int flag_tracking_id_run_id = 0;

            tracker.slow5_header = &slow5_header;
            tracker.slow5_record = &slow5_record;
            tracker.flag_context_tags = &flag_context_tags;
            tracker.flag_tracking_id = &flag_tracking_id;
            tracker.flag_tracking_id_run_id = &flag_tracking_id_run_id;
            tracker.nreads = &number_reads;

            reset_attributes(ROOT, &tracker);
            reset_attributes(READ, &tracker);
            reset_attributes(RAW, &tracker);
            reset_attributes(CHANNEL_ID, &tracker);
            reset_attributes(CONTEXT_TAGS, &tracker);
            reset_attributes(TRACKING_ID, &tracker);

            //now iterate over read groups. loading records and writing them are done inside op_func_group
            H5Literate(fast5_file.hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, op_func_group, (void *) &tracker);
            free_attributes(ROOT, &tracker);
            free_attributes(CONTEXT_TAGS, &tracker);
            free_attributes(TRACKING_ID, &tracker);


        } else {
            fast5_t f5;
            int32_t ret=fast5_read_single_fast5(fast5_file, &f5);
            if (ret < 0) {
                WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_path);
                bad_fast5_file++;
                fast5_close(fast5_file);
                return 0;
            }

            std::string read_id = fast5_get_read_id_single_fast5(fast5_file);
            if (read_id == "") {
                WARNING("Fast5 file [%s] does not have a read ID and will be skipped", fast5_path);
                bad_fast5_file++;
                fast5_close(fast5_file);
                return 0;
            }

            write_data(f_out, format_out, strmp, f_idx, read_id, f5, fast5_path);
        }
    }
    else{
        WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_path);
        bad_fast5_file++;
        return 0;
    }

    fast5_close(fast5_file);

    return 1;

}

void recurse_dir(const char *f_path, FILE *f_out, enum FormatOut format_out,
        z_streamp strmp, FILE *f_idx) {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(f_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // If it has the fast5 extension
            if (has_fast5_ext(f_path)) {
                // Open FAST5 and convert to SLOW5 into f_out
                fast5_to_slow5(f_path, f_out, format_out, strmp, f_idx);
            }

        } else {
            WARNING("File '%s' failed to open - %s.",
                    f_path, strerror(errno));
        }

    } else {
        fprintf(stderr, "[%s::%.3f*%.2f] Extracting fast5 from %s\n", __func__,
                realtime() - init_realtime, cputime() / (realtime() - init_realtime), f_path);

        // Iterate through sub files
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 &&
                    strcmp(ent->d_name, "..") != 0) {

                // Make sub path string
                // f_path + '/' + ent->d_name + '\0'
                size_t sub_f_path_len = strlen(f_path) + 1 + strlen(ent->d_name) + 1;
                char *sub_f_path = (char *) malloc(sizeof *sub_f_path * sub_f_path_len);
                MALLOC_CHK(sub_f_path);
                snprintf(sub_f_path, sub_f_path_len, "%s/%s", f_path, ent->d_name);

                // Recurse
                recurse_dir(sub_f_path, f_out, format_out, strmp, f_idx);

                free(sub_f_path);
                sub_f_path = NULL;
            }
        }

        closedir(dir);
    }
}

int f2s_main(int argc, char **argv, struct program_meta *meta) {
    init_realtime = realtime();

    int ret; // For checking return values of functions
    z_stream strm; // Declare stream for compression output if specified

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

    static struct option long_opts[] = {
        {"binary", no_argument, NULL, 'b' },
        {"compress", no_argument, NULL, 'c' },
        {"help", no_argument, NULL, 'h' },
        {"index", required_argument, NULL, 'i' },
        {"output", required_argument, NULL, 'o' },
        {NULL, 0, NULL, 0 }
    };

    // Default options
    FILE *f_out = stdout;
    enum FormatOut format_out = OUT_ASCII;
    FILE *f_idx = NULL;

    // Input arguments
    char *arg_fname_out = NULL;
    char *arg_fname_idx = NULL;

    char opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "bchi:o:", long_opts, NULL)) != -1) {

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
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
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
            fprintf(f_out, SLOW5_FILE_FORMAT);
            fprintf(f_out, SLOW5_HEADER);
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

    for (int i = optind; i < argc; ++ i) {
        // Recursive way
        recurse_dir(argv[i], f_out, format_out, &strm, f_idx);

        // TODO iterative way
    }

    MESSAGE(stderr, "total reads: %lu, bad fast5: %lu",
            total_reads, bad_fast5_file);

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

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}

herr_t op_func_attr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data){

    hid_t attribute, attribute_type, native_type;
    herr_t return_val = 0;
    int ret = 0;

    struct operator_obj *operator_data = (struct operator_obj *) op_data;
    // Ensure attribute exists
    ret = H5Aexists(loc_id, name);
    if(ret <= 0) {
        fprintf(stdout,"attribute %s not found\n",name);
    }
    attribute = H5Aopen(loc_id, name, H5P_DEFAULT);
    attribute_type = H5Aget_type(attribute);
    native_type = H5Tget_native_type(attribute_type, H5T_DIR_ASCEND);
    H5T_class_t H5Tclass = H5Tget_class(attribute_type);

    union attribute_data value;
    int flag_value_string = 0;
    switch(H5Tclass){
        case H5T_STRING:
            flag_value_string = 1;
            if(H5Tis_variable_str(attribute_type) > 0) {
                // variable length string
                ret = H5Aread(attribute, native_type, &value.attr_string);
                if(ret < 0) {
                    fprintf(stderr, "error reading attribute %s\n", name);
                    exit(EXIT_FAILURE);
                }

            } else {
                // fixed length string
                size_t storage_size;
                // Get the storage size and allocate
                storage_size = H5Aget_storage_size(attribute);
                value.attr_string = (char*)calloc(storage_size + 1, sizeof(char));

                // finally read the attribute
                ret = H5Aread(attribute, attribute_type, value.attr_string);

                if(ret < 0) {
                    fprintf(stderr, "error reading attribute %s\n", name);
                    exit(EXIT_FAILURE);
                }
            }
            if (value.attr_string && !value.attr_string[0]) {
                fprintf(stderr,"warning: attribute value of %s is an empty string\n",name);
                if(flag_value_string){ // hack to skip the free() at the bottom of the function
                    free(value.attr_string);
                    flag_value_string = 0;
                }
                value.attr_string = (char*)".";
            }
            break;
        case H5T_FLOAT:
            H5Aread(attribute, native_type, &(value.attr_double));
            break;
        case H5T_INTEGER:
            H5Aread(attribute,native_type,&(value.attr_int));
            break;
        case H5T_ENUM:
            H5Aread(attribute, native_type, &(value.attr_uint8_t));
            break;
        default:
            fprintf (stderr,"Unknown: %s\n", name);
    }
    H5Aclose(attribute);
    if(strcmp("file_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->file_type = strdup(value.attr_string);
    }
    else if(strcmp("file_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->file_version = strdup(value.attr_string);
    }
//            READ
    else if(strcmp("run_id",name)==0 && H5Tclass==H5T_STRING){
        if(*(operator_data->flag_tracking_id) == 0 and *(operator_data->flag_tracking_id_run_id) == 0){
            operator_data->slow5_header->tracking_id_run_id = strdup(value.attr_string);
            *(operator_data->flag_tracking_id_run_id) = 1;
        }
        else{
            operator_data->slow5_header->run_id = strdup(value.attr_string);
        }
    }
    else if(strcmp("pore_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->pore_type = strdup(value.attr_string);
    }
//            RAW
    else if(strcmp("start_time",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->start_time = value.attr_int;
    }
    else if(strcmp("duration",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->duration = value.attr_int;
    }
    else if(strcmp("read_number",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->read_number = value.attr_int;
    }
    else if(strcmp("start_mux",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->start_mux = value.attr_int;
    }
    else if(strcmp("read_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_record->read_id = strdup(value.attr_string);
    }
    else if(strcmp("median_before",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->median_before = value.attr_double;
    }
    else if(strcmp("end_reason",name)==0 && H5Tclass==H5T_ENUM){
        operator_data->slow5_record->end_reason = value.attr_uint8_t;
    }
//            CHANNEL_ID
    else if(strcmp("channel_number",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_record->channel_number = strdup(value.attr_string);
    }
    else if(strcmp("digitisation",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->digitisation = value.attr_double;
    }
    else if(strcmp("offset",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->offset = value.attr_double;
    }
    else if(strcmp("range",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->range = value.attr_double;
    }
    else if(strcmp("sampling_rate",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->sampling_rate = value.attr_double;
    }
//            CONTEXT_TAGS
    else if(strcmp("sample_frequency",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->sample_frequency = strdup(value.attr_string);
    }
        //additional attributes in 2.2
    else if(strcmp("barcoding_enabled",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->barcoding_enabled = strdup(value.attr_string);
    }
    else if(strcmp("experiment_duration_set",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->experiment_duration_set = strdup(value.attr_string);
    }
    else if(strcmp("experiment_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->experiment_type = strdup(value.attr_string);
    }
    else if(strcmp("local_basecalling",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->local_basecalling = strdup(value.attr_string);
    }
    else if(strcmp("package",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->package = strdup(value.attr_string);
    }
    else if(strcmp("package_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->package_version = strdup(value.attr_string);
    }
    else if(strcmp("sequencing_kit",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->sequencing_kit = strdup(value.attr_string);
    }
        //additional attributes in 2.0
    else if(strcmp("filename",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->filename = strdup(value.attr_string);
    }
    else if(strcmp("experiment_kit",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->experiment_kit = strdup(value.attr_string);
    }
    else if(strcmp("user_filename_input",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->user_filename_input = strdup(value.attr_string);
    }
//            TRACKING_ID
    else if(strcmp("asic_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_id = strdup(value.attr_string);
    }
    else if(strcmp("asic_id_eeprom",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_id_eeprom = strdup(value.attr_string);
    }
    else if(strcmp("asic_temp",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_temp = strdup(value.attr_string);
    }
    else if(strcmp("auto_update",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->auto_update = strdup(value.attr_string);
    }
    else if(strcmp("auto_update_source",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->auto_update_source = strdup(value.attr_string);
    }
    else if(strcmp("bream_is_standard",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_is_standard = strdup(value.attr_string);
    }
    else if(strcmp("device_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->device_id = strdup(value.attr_string);
    }
    else if(strcmp("exp_script_name",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->exp_script_name = strdup(value.attr_string);
    }
    else if(strcmp("exp_script_purpose",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->exp_script_purpose = strdup(value.attr_string);
    }
    else if(strcmp("exp_start_time",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->exp_start_time = strdup(value.attr_string);
    }
    else if(strcmp("flow_cell_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->flow_cell_id = strdup(value.attr_string);
    }
    else if(strcmp("heatsink_temp",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->heatsink_temp = strdup(value.attr_string);
    }
    else if(strcmp("hostname",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->hostname = strdup(value.attr_string);
    }
    else if(strcmp("installation_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->installation_type = strdup(value.attr_string);
    }
    else if(strcmp("local_firmware_file",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->local_firmware_file = strdup(value.attr_string);
    }
    else if(strcmp("operating_system",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->operating_system = strdup(value.attr_string);
    }
    else if(strcmp("protocol_run_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->protocol_run_id = strdup(value.attr_string);
    }
    else if(strcmp("protocols_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->protocols_version = strdup(value.attr_string);
    }
//        else if(strcmp("tracking_id_run_id",name)==0 && H5Tclass==H5T_STRING){
//            operator_data->slow5_header->tracking_id_run_id = strdup(value.attr_string);
//            }
    else if(strcmp("usb_config",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->usb_config = strdup(value.attr_string);
    }
    else if(strcmp("version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->version = strdup(value.attr_string);
    }
        //additional attributes in 2.0
    else if(strcmp("bream_core_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_core_version = strdup(value.attr_string);
    }
    else if(strcmp("bream_ont_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_ont_version = strdup(value.attr_string);
    }
    else if(strcmp("bream_prod_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_prod_version = strdup(value.attr_string);
    }
    else if(strcmp("bream_rnd_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_rnd_version = strdup(value.attr_string);
    }
        //additional attributes in 2.2
    else if(strcmp("asic_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_version = strdup(value.attr_string);
    }
    else if(strcmp("configuration_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->configuration_version = strdup(value.attr_string);
    }
    else if(strcmp("device_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->device_type = strdup(value.attr_string);
    }
    else if(strcmp("distribution_status",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->distribution_status = strdup(value.attr_string);
    }
    else if(strcmp("distribution_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->distribution_version = strdup(value.attr_string);
    }
    else if(strcmp("flow_cell_product_code",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->flow_cell_product_code = strdup(value.attr_string);
    }
    else if(strcmp("guppy_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->guppy_version = strdup(value.attr_string);
    }
    else if(strcmp("protocol_group_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->protocol_group_id = strdup(value.attr_string);
    }
    else if(strcmp("sample_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->sample_id = strdup(value.attr_string);
    }else{
        fprintf(stderr,"[%s] no attribute %s\n",__func__ , name);
    }

    if(flag_value_string){
        free(value.attr_string);
    }

    return return_val;
}

int read_dataset(hid_t loc_id, const char *name, slow5_record_t* slow5_record) {
    int16_t* rawptr;   // raw signal

    hid_t dset = H5Dopen(loc_id, name, H5P_DEFAULT);
    if (dset < 0) {
        WARNING("Failed to open dataset '%s' to read raw signal.", name);
        return -1;
        // goto cleanup2;
    }

    hid_t space;
    hsize_t h5_nsample;
    space = H5Dget_space(dset);
    if (space < 0) {
        WARNING("Failed to create copy of dataspace for raw signal %s.",
                name);
        H5Dclose(dset);
        return -1;
        // goto cleanup3;
    }

    int32_t ret1 = H5Sget_simple_extent_dims(space, &h5_nsample, NULL);
    if (ret1 < 0) {
        WARNING("Failed to get the dataspace dimension for raw signal %s.", name);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }

    if(slow5_record->duration != h5_nsample){
        fprintf(stderr,"attribute duration in /read/Raw does not match with the length of the signal\n");
        exit(EXIT_FAILURE);
    }

    rawptr = (int16_t*)calloc(h5_nsample, sizeof(float));
    hid_t status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT,rawptr);

    if (status < 0) {
        free(rawptr);
        WARNING("Failed to read raw data from dataset %s.", name);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }
    H5Sclose(space);
    H5Dclose(dset);
    slow5_record->raw_signal = rawptr;
    return 0;
}

/************************************************************

  Operator function. If the object is a group, it
  is first checked against other groups in its path using
  the group_check function, then if it is not a duplicate,
  H5Literate is called for that group.  This guarantees that
  the program will not enter infinite recursion due to a
  circular path in the file.

 ************************************************************/
herr_t op_func_group (hid_t loc_id, const char *name, const H5L_info_t *info, void *op_data){

    herr_t return_val = 0;
    H5O_info_t infobuf;
    struct operator_obj *operator_data = (struct operator_obj *) op_data;
    /* Type conversion */
    unsigned spaces = 2*(operator_data->group_level + 1);

    // if group is 'Analyses'; then skip
    if(strcmp(name,"Analyses")==0)return return_val;
    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library.
     */
    H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    switch (infobuf.type) {
        case H5O_TYPE_GROUP:

            /*
             * Check group address against linked list of operator
             * data structures.  We will always run the check, as the
             * reference count cannot be relied upon if there are
             * symbolic links, and H5Oget_info_by_name always follows
             * symbolic links.  Alternatively we could use H5Lget_info
             * and never recurse on groups discovered by symbolic
             * links, however it could still fail if an object's
             * reference count was manually manipulated with
             * H5Odecr_refcount.
             */
            if (group_check(operator_data, infobuf.addr) ) {
                printf ("%*s  Warning: Loop detected!\n", spaces, "");
            }
            else {
                //@ group number of attributes
                hid_t group = H5Gopen(loc_id, name, H5P_DEFAULT);
                /*
                 * Initialize new operator data structure and
                 * begin recursive iteration on the discovered
                 * group.  The new operator_obj structure is given a
                 * pointer to the current one.
                 */
                struct operator_obj next_op = *operator_data;
                next_op.group_level = operator_data->group_level + 1;
                next_op.prev = operator_data;
                next_op.addr = infobuf.addr;

                //traverse the attributes belonging to the group
                //tracking_id and context_tags groups should be traversed only for the first read
                if(strcmp(name,"tracking_id")==0 && *(operator_data->flag_tracking_id)==0){
                    H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                    *(operator_data->flag_tracking_id) = 1;
                }else if (strcmp(name,"context_tags")==0 && *(operator_data->flag_context_tags)==0){
                    H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                    *(operator_data->flag_context_tags) = 1;
                } else if(strcmp(name,"tracking_id")!=0 && strcmp(name,"context_tags")!=0){
                    H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                }
                H5Gclose(group);
                //the recursive call
                return_val = H5Literate_by_name(loc_id, name, H5_INDEX_NAME,H5_ITER_INC, 0, op_func_group, (void *) &next_op, H5P_DEFAULT);
                //check if we are at a root-level group
                if(operator_data->group_level == ROOT){
                    if(*(operator_data->nreads) ==0){
                        if(*(operator_data->flag_context_tags) != 1) {
                            fprintf(stderr, "The first read does not have context_tags information\n");
                            exit(EXIT_FAILURE);
                        }
                        if(*(operator_data->flag_tracking_id) != 1){
                            fprintf(stderr,"The first read does not have tracking_id information\n");
                            exit(EXIT_FAILURE);
                        }
//                        print_header();

                    }
                    *(operator_data->nreads) = *(operator_data->nreads)+1;

                    //todo: we can pass slow5_record directly to write_data()
                    fast5_t f5;
                    f5.rawptr = operator_data->slow5_record->raw_signal;
                    f5.nsample = operator_data->slow5_record->duration;
                    f5.digitisation = operator_data->slow5_record->digitisation;
                    f5.offset = operator_data->slow5_record->offset;
                    f5.range = operator_data->slow5_record->range;
                    f5.sample_rate = operator_data->slow5_record->sampling_rate;

                    write_data(operator_data->f_out, operator_data->format_out, operator_data->strmp, operator_data->f_idx, operator_data->slow5_record->read_id, f5, operator_data->fast5_path);
                    free_attributes(READ, operator_data);
                    free_attributes(RAW, operator_data);
                    free_attributes(CHANNEL_ID, operator_data);

//                    print_record();
//                    fprintf(stdout,"%s\n",name);
                }
            }
            break;
        case H5O_TYPE_DATASET:
//            printf ("Dataset: %s\n", name);
            read_dataset(loc_id, name, operator_data->slow5_record);
            break;
        case H5O_TYPE_NAMED_DATATYPE:
            printf ("Datatype: %s\n", name);
            break;
        default:
            printf ( "Unknown: %s\n", name);
    }
    return return_val;
}

/************************************************************

  This function recursively searches the linked list of
  hdf5group structures for one whose address matches
  target_addr.  Returns 1 if a match is found, and 0
  otherwise.

 ************************************************************/
int group_check(struct operator_obj *od, haddr_t target_addr){
    if (od->addr == target_addr)
        return 1;       /* Addresses match */
    else if (!od->group_level)
        return 0;       /* Root group reached with no matches */
    else
        return group_check(od->prev, target_addr);
    /* Recursively examine the next node */
}

void free_attributes(group_flags group_flag, operator_obj* operator_data) {
    switch (group_flag) {
        case ROOT:
            if(operator_data->slow5_header->file_type)free(operator_data->slow5_header->file_type);operator_data->slow5_header->file_type = NULL;
            if(operator_data->slow5_header->file_version)free(operator_data->slow5_header->file_version);operator_data->slow5_header->file_version = NULL;
            break;
        case READ:
            if(operator_data->slow5_header->run_id)free(operator_data->slow5_header->run_id);operator_data->slow5_header->run_id = NULL;
            if(operator_data->slow5_header->pore_type)free(operator_data->slow5_header->pore_type);operator_data->slow5_header->pore_type = NULL;
            break;
        case RAW:
            if(operator_data->slow5_record->raw_signal)free(operator_data->slow5_record->raw_signal);operator_data->slow5_record->raw_signal = NULL;
            operator_data->slow5_record->start_time = ULONG_MAX;
            operator_data->slow5_record->duration = UINT_MAX;
            operator_data->slow5_record->read_number = -1;
            operator_data->slow5_record->start_mux = -1;
            if(operator_data->slow5_record->read_id)free(operator_data->slow5_record->read_id);operator_data->slow5_record->read_id = NULL;
            operator_data->slow5_record->median_before = -1;
            operator_data->slow5_record->end_reason = '6' - '0';
            break;
        case CHANNEL_ID:
            operator_data->slow5_record->digitisation = -1;
            operator_data->slow5_record->offset = -1;
            operator_data->slow5_record->range = -1;
            operator_data->slow5_record->sampling_rate = -1;
            if(operator_data->slow5_record->channel_number)free(operator_data->slow5_record->channel_number);operator_data->slow5_record->channel_number = NULL;
            break;
        case CONTEXT_TAGS:
            if(operator_data->slow5_header->sample_frequency)free(operator_data->slow5_header->sample_frequency);operator_data->slow5_header->sample_frequency = NULL;
            //additional attributes in 2.2
            if(operator_data->slow5_header->barcoding_enabled)free(operator_data->slow5_header->barcoding_enabled);operator_data->slow5_header->barcoding_enabled = NULL;
            if(operator_data->slow5_header->experiment_duration_set)free(operator_data->slow5_header->experiment_duration_set);operator_data->slow5_header->experiment_duration_set = NULL;
            if(operator_data->slow5_header->experiment_type)free(operator_data->slow5_header->experiment_type);operator_data->slow5_header->experiment_type = NULL;
            if(operator_data->slow5_header->local_basecalling)free(operator_data->slow5_header->local_basecalling);operator_data->slow5_header->local_basecalling = NULL;
            if(operator_data->slow5_header->package)free(operator_data->slow5_header->package);operator_data->slow5_header->package = NULL;
            if(operator_data->slow5_header->package_version)free(operator_data->slow5_header->package_version);operator_data->slow5_header->package_version = NULL;
            if(operator_data->slow5_header->sequencing_kit)free(operator_data->slow5_header->sequencing_kit);operator_data->slow5_header->sequencing_kit = NULL;
            //additional attributes in 2.0
            if(operator_data->slow5_header->filename)free(operator_data->slow5_header->filename);operator_data->slow5_header->filename = NULL;
            if(operator_data->slow5_header->experiment_kit)free(operator_data->slow5_header->experiment_kit);operator_data->slow5_header->experiment_kit = NULL;
            if(operator_data->slow5_header->user_filename_input)free(operator_data->slow5_header->user_filename_input);operator_data->slow5_header->user_filename_input = NULL;
            break;
        case TRACKING_ID:
            if(operator_data->slow5_header->asic_id)free(operator_data->slow5_header->asic_id);operator_data->slow5_header->asic_id = NULL;
            if(operator_data->slow5_header->asic_id_eeprom)free(operator_data->slow5_header->asic_id_eeprom);operator_data->slow5_header->asic_id_eeprom = NULL;
            if(operator_data->slow5_header->asic_temp)free(operator_data->slow5_header->asic_temp);operator_data->slow5_header->asic_temp = NULL;
            if(operator_data->slow5_header->auto_update)free(operator_data->slow5_header->auto_update);operator_data->slow5_header->auto_update = NULL;
            if(operator_data->slow5_header->auto_update_source)free(operator_data->slow5_header->auto_update_source);operator_data->slow5_header->auto_update_source = NULL;
            if(operator_data->slow5_header->bream_is_standard)free(operator_data->slow5_header->bream_is_standard);operator_data->slow5_header->bream_is_standard = NULL;
            if(operator_data->slow5_header->device_id)free(operator_data->slow5_header->device_id);operator_data->slow5_header->device_id = NULL;
            if(operator_data->slow5_header->exp_script_name)free(operator_data->slow5_header->exp_script_name);operator_data->slow5_header->exp_script_name = NULL;
            if(operator_data->slow5_header->exp_script_purpose)free(operator_data->slow5_header->exp_script_purpose);operator_data->slow5_header->exp_script_purpose = NULL;
            if(operator_data->slow5_header->exp_start_time)free(operator_data->slow5_header->exp_start_time);operator_data->slow5_header->exp_start_time = NULL;
            if(operator_data->slow5_header->flow_cell_id)free(operator_data->slow5_header->flow_cell_id);operator_data->slow5_header->flow_cell_id = NULL;
            if(operator_data->slow5_header->heatsink_temp)free(operator_data->slow5_header->heatsink_temp);operator_data->slow5_header->heatsink_temp = NULL;
            if(operator_data->slow5_header->hostname)free(operator_data->slow5_header->hostname);operator_data->slow5_header->hostname = NULL;
            if(operator_data->slow5_header->installation_type)free(operator_data->slow5_header->installation_type);operator_data->slow5_header->installation_type = NULL;
            if(operator_data->slow5_header->local_firmware_file)free(operator_data->slow5_header->local_firmware_file);operator_data->slow5_header->local_firmware_file = NULL;
            if(operator_data->slow5_header->operating_system)free(operator_data->slow5_header->operating_system);operator_data->slow5_header->operating_system = NULL;
            if(operator_data->slow5_header->protocol_run_id)free(operator_data->slow5_header->protocol_run_id);operator_data->slow5_header->protocol_run_id = NULL;
            if(operator_data->slow5_header->protocols_version)free(operator_data->slow5_header->protocols_version);operator_data->slow5_header->protocols_version = NULL;
            if(operator_data->slow5_header->tracking_id_run_id)free(operator_data->slow5_header->tracking_id_run_id);operator_data->slow5_header->tracking_id_run_id = NULL;
            if(operator_data->slow5_header->usb_config)free(operator_data->slow5_header->usb_config);operator_data->slow5_header->usb_config = NULL;
            if(operator_data->slow5_header->version)free(operator_data->slow5_header->version);operator_data->slow5_header->version = NULL;
            //additional attributes in 2.0
            if(operator_data->slow5_header->bream_core_version)free(operator_data->slow5_header->bream_core_version);operator_data->slow5_header->bream_core_version = NULL;
            if(operator_data->slow5_header->bream_ont_version)free(operator_data->slow5_header->bream_ont_version);operator_data->slow5_header->bream_ont_version = NULL;
            if(operator_data->slow5_header->bream_prod_version)free(operator_data->slow5_header->bream_prod_version);operator_data->slow5_header->bream_prod_version = NULL;
            if(operator_data->slow5_header->bream_rnd_version)free(operator_data->slow5_header->bream_rnd_version);operator_data->slow5_header->bream_rnd_version = NULL;
            //additional attributes in 2.2
            if(operator_data->slow5_header->asic_version)free(operator_data->slow5_header->asic_version);operator_data->slow5_header->asic_version = NULL;
            if(operator_data->slow5_header->configuration_version)free(operator_data->slow5_header->configuration_version);operator_data->slow5_header->configuration_version = NULL;
            if(operator_data->slow5_header->device_type)free(operator_data->slow5_header->device_type);operator_data->slow5_header->device_type = NULL;
            if(operator_data->slow5_header->distribution_status)free(operator_data->slow5_header->distribution_status);operator_data->slow5_header->distribution_status = NULL;
            if(operator_data->slow5_header->distribution_version)free(operator_data->slow5_header->distribution_version);operator_data->slow5_header->distribution_version = NULL;
            if(operator_data->slow5_header->flow_cell_product_code)free(operator_data->slow5_header->flow_cell_product_code);operator_data->slow5_header->flow_cell_product_code = NULL;
            if(operator_data->slow5_header->guppy_version)free(operator_data->slow5_header->guppy_version);operator_data->slow5_header->guppy_version = NULL;
            if(operator_data->slow5_header->protocol_group_id)free(operator_data->slow5_header->protocol_group_id);operator_data->slow5_header->protocol_group_id = NULL;
            if(operator_data->slow5_header->sample_id)free(operator_data->slow5_header->sample_id);operator_data->slow5_header->sample_id = NULL;
            break;
        default:
            fprintf(stderr,"unexpected behaviour\n");
            exit(EXIT_FAILURE);
    }
}
void reset_attributes(group_flags group_flag, operator_obj* operator_data) {
    switch (group_flag) {
        case ROOT:
            operator_data->slow5_header->file_type = NULL;
            operator_data->slow5_header->file_version = NULL;
            break;
        case READ:
            operator_data->slow5_header->run_id = NULL;
            operator_data->slow5_header->pore_type = NULL;
            break;
        case RAW:
            operator_data->slow5_record->start_time = ULONG_MAX;
            operator_data->slow5_record->duration = UINT_MAX;
            operator_data->slow5_record->read_number = -1;
            operator_data->slow5_record->start_mux = -1;
            operator_data->slow5_record->read_id = NULL;
            operator_data->slow5_record->median_before = -1;
            operator_data->slow5_record->end_reason = '6' - '0';
            break;
        case CHANNEL_ID:
            operator_data->slow5_record->digitisation = -1;
            operator_data->slow5_record->offset = -1;
            operator_data->slow5_record->range = -1;
            operator_data->slow5_record->sampling_rate = -1;
            operator_data->slow5_record->channel_number = NULL;
            break;
        case CONTEXT_TAGS:
            operator_data->slow5_header->sample_frequency = NULL;
            //additional attributes in 2.2
            operator_data->slow5_header->barcoding_enabled = NULL;
            operator_data->slow5_header->experiment_duration_set = NULL;
            operator_data->slow5_header->sequencing_kit = NULL;
            operator_data->slow5_header->experiment_type = NULL;
            operator_data->slow5_header->local_basecalling = NULL;
            operator_data->slow5_header->package = NULL;
            operator_data->slow5_header->package_version = NULL;
            //additional attributes in 2.0
            operator_data->slow5_header->filename = NULL;
            operator_data->slow5_header->experiment_kit = NULL;
            operator_data->slow5_header->user_filename_input = NULL;
            break;
        case TRACKING_ID:
            operator_data->slow5_header->asic_id = NULL;
            operator_data->slow5_header->asic_id_eeprom = NULL;
            operator_data->slow5_header->asic_temp = NULL;
            operator_data->slow5_header->auto_update = NULL;
            operator_data->slow5_header->auto_update_source = NULL;
            operator_data->slow5_header->bream_is_standard = NULL;
            operator_data->slow5_header->device_id = NULL;
            operator_data->slow5_header->exp_script_name = NULL;
            operator_data->slow5_header->exp_script_purpose = NULL;
            operator_data->slow5_header->exp_start_time = NULL;
            operator_data->slow5_header->flow_cell_id = NULL;
            operator_data->slow5_header->heatsink_temp = NULL;
            operator_data->slow5_header->hostname = NULL;
            operator_data->slow5_header->installation_type = NULL;
            operator_data->slow5_header->local_firmware_file = NULL;
            operator_data->slow5_header->operating_system = NULL;
            operator_data->slow5_header->protocol_run_id = NULL;
            operator_data->slow5_header->protocols_version = NULL;
            operator_data->slow5_header->tracking_id_run_id = NULL;
            operator_data->slow5_header->usb_config = NULL;
            operator_data->slow5_header->version = NULL;
            //additional attributes in 2.0
            operator_data->slow5_header->bream_core_version = NULL;
            operator_data->slow5_header->bream_ont_version = NULL;
            operator_data->slow5_header->bream_prod_version = NULL;
            operator_data->slow5_header->bream_rnd_version = NULL;
            //additional attributes in 2.2
            operator_data->slow5_header->asic_version = NULL;
            operator_data->slow5_header->configuration_version = NULL;
            operator_data->slow5_header->device_type = NULL;
            operator_data->slow5_header->distribution_status = NULL;
            operator_data->slow5_header->distribution_version = NULL;
            operator_data->slow5_header->flow_cell_product_code = NULL;
            operator_data->slow5_header->guppy_version = NULL;
            operator_data->slow5_header->protocol_group_id = NULL;
            operator_data->slow5_header->sample_id = NULL;

            break;
        default:
            fprintf(stderr,"unexpected behaviour\n");
            exit(EXIT_FAILURE);
    }
}