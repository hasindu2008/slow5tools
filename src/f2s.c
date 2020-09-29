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

enum FormatOut {
    OUT_ASCII,
    OUT_BINARY,
    OUT_COMPRESS
};

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

    long curr_pos = -1;
    if (f_idx != NULL) {
        curr_pos = ftell(f_out);
        fprintf(f_idx, "%s\t%ld\t", read_id.c_str(), curr_pos);
    }

    // Interpret file parameter
    if (format_out == OUT_ASCII) {
        
        long curr_pos = ftell(f_out);
        if (f_idx != NULL) {
            fprintf(f_idx, "%ld\t", curr_pos);
        }

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

    } else if (format_out == OUT_BINARY) {

        long curr_pos = -1;
        if (f_idx != NULL) {
            curr_pos = ftell(f_out);
            fprintf(f_idx, "%ld\t", curr_pos);
        }

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

        if (f_idx != NULL) {
            fprintf(f_idx, "%ld\n", ftell(f_out) - curr_pos);
        }
            
    } else if (format_out == OUT_COMPRESS) {

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
        z_deflate_write(strmp, fast5_path, sizeof *fast5_path * fast5_path_len, f_out, Z_NO_FLUSH);
    }

    if (f_idx != NULL) {
        fprintf(f_idx, "%ld\n", ftell(f_out) - curr_pos);
    }


    free(f5.rawptr);
}

int fast5_to_slow5(const char *fast5_path, FILE *f_out, enum FormatOut format_out, 
        z_streamp strmp, FILE *f_idx) {

    total_reads++;

    fast5_file_t fast5_file = fast5_open(fast5_path);

    if (fast5_file.hdf5_file >= 0) {

        //TODO: can optimise for performance
        if (fast5_file.is_multi_fast5) {
            std::vector<std::string> read_groups = fast5_get_multi_read_groups(fast5_file);
            std::string prefix = "read_";
            for (size_t group_idx = 0; group_idx < read_groups.size(); ++group_idx) {
                std::string group_name = read_groups[group_idx];

                if (group_name.find(prefix) == 0) {
                    std::string read_id = group_name.substr(prefix.size());
                    fast5_t f5;
                    int32_t ret = fast5_read_multi_fast5(fast5_file, &f5, read_id);

                    if (ret < 0) {
                        WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_path);
                        bad_fast5_file++;
                        fast5_close(fast5_file);
                        return 0;
                    }

                    write_data(f_out, format_out, strmp, f_idx, read_id, f5, fast5_path);
                }
            }

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
                format_out = OUT_COMPRESS;
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

    if (format_out == OUT_COMPRESS) {
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
        case OUT_COMPRESS:
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
            ret = z_deflate_write(&strm, header, strlen(header), f_out, Z_NO_FLUSH);
            if (ret != Z_OK) {
                deflateEnd(&strm);
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

    // Output gzip footer to file
    if (format_out == OUT_COMPRESS) {
        char empty[] = "";
        ret = z_deflate_write(&strm, empty, strlen(empty), f_out, Z_FINISH);
        deflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            ERROR("footer failed\n%s", ""); // testing

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
