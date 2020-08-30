// Sasha Jenner

#include "fast5lite.h"
#include "slow5.h"

#define USAGE_MSG "Usage: %s [OPTION]... [FAST5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Convert fast5 file(s) to slow5 or blow5.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -b, --binary\n" \
    "        Convert to blow5, rather than the default slow5.\n" \
    "\n" \
    "    -d, --max-depth=[NUM]\n" \
    "        Set the maximum depth to search directories for fast5 files.\n" \
    "        NUM must be a non-negative integer.\n" \
    "        Default: No maximum depth.\n" \
    "\n" \
    "        E.g. NUM=1: Read the files within a specified directory but\n" \
    "        not those within subdirectories.\n" \
    "\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \
    "\n" \
    "    -o, --output=[FILE]\n" \
    "        Output slow5 or blow5 contents to FILE.\n" \
    "        Default: Stdout.\n" \

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

int fast5_to_slow5(const char *fast5_path, FILE *f_out, bool binary_out) {

    total_reads++;

    fast5_file_t fast5_file = fast5_open(fast5_path);

    if (fast5_file.hdf5_file >= 0) {

        //TODO: can optimise for performance
        if(fast5_file.is_multi_fast5) {
            std::vector<std::string> read_groups = fast5_get_multi_read_groups(fast5_file);
            std::string prefix = "read_";
            for(size_t group_idx = 0; group_idx < read_groups.size(); ++group_idx) {
                std::string group_name = read_groups[group_idx];
                if(group_name.find(prefix) == 0) {
                    std::string read_id = group_name.substr(prefix.size());
                        fast5_t f5;
                        int32_t ret=fast5_read_multi_fast5(fast5_file, &f5, read_id);
                        if(ret<0){
                            WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_path);
                            bad_fast5_file++;
                            fast5_close(fast5_file);
                            return 0;
                        }

                        fprintf(f_out, "%s\t%ld\t%.1f\t%.1f\t%.1f\t%.1f\t", read_id.c_str(),
                                f5.nsample,f5.digitisation, f5.offset, f5.range, f5.sample_rate);

                        if (binary_out) {
                            fwrite(f5.rawptr, sizeof *f5.rawptr, f5.nsample, f_out);
                        } else {
                            for (uint16_t j = 0; j < f5.nsample; ++ j) {
                                if (j == f5.nsample - 1) {
                                    fprintf(f_out, "%hu", f5.rawptr[j]);
                                } else {
                                    fprintf(f_out, "%hu,", f5.rawptr[j]);
                                }
                            }
                        }

                        fprintf(f_out, "\t%d\t%s\t%s\n",0,".",fast5_path);
                        free(f5.rawptr);
                }
            }
        }
        else{
            fast5_t f5;
            int32_t ret=fast5_read_single_fast5(fast5_file, &f5);
            if(ret<0){
                WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_path);
                bad_fast5_file++;
                fast5_close(fast5_file);
                return 0;
            }
            std::string read_id = fast5_get_read_id_single_fast5(fast5_file);
            if (read_id==""){
                WARNING("Fast5 file [%s] does not have a read ID and will be skipped", fast5_path);
                bad_fast5_file++;
                fast5_close(fast5_file);
                return 0;
            }

            fast5_close(fast5_file);

            //fprintf(f_out, 
            //"@read_id\tn_samples\tdigitisation\toffset\trange\tsample_rate\traw_signal\tnum_bases\tsequence\nfast5_path");

            fprintf(f_out, "%s\t%ld\t%.1f\t%.1f\t%.1f\t%.1f\t", read_id.c_str(),
                    f5.nsample,f5.digitisation, f5.offset, f5.range, f5.sample_rate);

            if (binary_out) {
                fwrite(f5.rawptr, sizeof *f5.rawptr, f5.nsample, f_out);
            } else {
                for (uint16_t j = 0; j < f5.nsample; ++ j) {
                    if (j == f5.nsample - 1) {
                        fprintf(f_out, "%hu", f5.rawptr[j]);
                    } else {
                        fprintf(f_out, "%hu,", f5.rawptr[j]);
                    }
                }
            }

            fprintf(f_out, "\t%d\t%s\t%s\n",0,".",fast5_path);

            free(f5.rawptr);
            f5.rawptr = NULL;
        }
    }
    else{
        WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_path);
        bad_fast5_file++;
        return 0;
    }

    return 1;

}

void recurse_dir(const char *f_path, FILE *f_out, bool binary_out) {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(f_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // If it has the fast5 extension
            if (has_fast5_ext(f_path)) {
                // Open FAST5 and convert to SLOW5 into f_out
                fast5_to_slow5(f_path, f_out, binary_out);
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
                recurse_dir(sub_f_path, f_out, binary_out);

                free(sub_f_path);
                sub_f_path = NULL;
            }
        }

        closedir(dir);
    }
}

int f2s_main(int argc, char **argv, struct program_meta *meta) {

    init_realtime = realtime();

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
        fprintf(stderr, USAGE_MSG HELP_SMALL_MSG, argv[0], argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
        {"binary", no_argument, NULL, 'b' },
        {"max-depth", required_argument, NULL, 'd' },
        {"help", no_argument, NULL, 'h' },
        {"output", required_argument, NULL, 'o' },
        {NULL, 0, NULL, 0 }
    };

    // Default options
    long max_depth = -1;
    FILE *f_out = stdout;
    bool binary_out = false;

    // Input arguments
    char *arg_max_depth = NULL;
    char *arg_fname_out = NULL;

    char opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "bd:ho:", long_opts, NULL)) != -1) {

        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 'b':
                binary_out = true;
                break;
            case 'd':
                arg_max_depth = optarg;
                break;
            case 'h':
                if (meta->verbose) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                return EXIT_SUCCESS;
            case 'o':
                arg_fname_out = optarg; 
                break; 
            default: // case '?' 
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    // Parse maximum depth argument
    if (arg_max_depth != NULL) {

        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing maximum depth%s","");
        }

        // Check it is a number
        
        // Cannot be empty 
        size_t arg_len = strlen(arg_max_depth);
        if (arg_len == 0) {
            MESSAGE(stderr, "invalid max depth -- '%s'", arg_max_depth);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }

        for (size_t i = 0; i < arg_len; ++ i) {
            // Not a digit and first char is not a '+'
            if (!isdigit((unsigned char) arg_max_depth[i]) && 
                    !(i == 0 && arg_max_depth[i] == '+')) {
                MESSAGE(stderr, "invalid max depth -- '%s'", arg_max_depth);
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);

                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
        }

        // Parse argument
        max_depth = strtol(arg_max_depth, NULL, 10);
        // Check for overflow
        if (errno == ERANGE) {
            WARNING("Overflow of max depth '%s'. Setting to %ld instead.", 
                    arg_max_depth, max_depth);
        }
    }

    // Parse output argument
    if (arg_fname_out != NULL) { 

        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing output filename%s","");
        }

        // Create new file or
        // Truncate existing file
        FILE *new_file = fopen(arg_fname_out, "w");

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

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }


    // Do the converting
    fprintf(f_out, SLOW5_HEADER);

    for (int i = optind; i < argc; ++ i) {
        // Recursive way
        recurse_dir(argv[i], f_out, binary_out);

        // TODO iterative way
    }

    MESSAGE(stderr, "total reads: %lu, bad fast5: %lu",
            total_reads, bad_fast5_file);


    if (f_out != stdout) {
        // Close output file
        if (fclose(f_out) == EOF) {
            ERROR("File '%s' failed on closing - %s.",
                  arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } 
    }

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
