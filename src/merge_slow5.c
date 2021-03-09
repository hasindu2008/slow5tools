//
// Created by Hiruna on 2021-01-16.
//
//#include "slow5_old.h"


#include <getopt.h>
#include <sys/wait.h>

#include <string>
#include <vector>

#include "error.h"
#include "cmd.h"
#include "slow5.h"
#include "read_fast5.h"
#include "slow5_extra.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "merge slow5 files\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help                 display this message and exit\n" \
    "    -o, --output=[FILE]        output converted contents to FILE -- stdout\n" \

static double init_realtime = 0;

int merge_slow5(FILE *f_out, std::vector<std::string> &slow5_files, reads_count* readsCount);

int compare_headers(slow5_header_t& slow5Header1, slow5_header_t& slow5Header2, int compare_level);

int merge_main(int argc, char **argv, struct program_meta *meta){
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
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h'},  //0
            {"output", required_argument, NULL, 'o'},   //1
            {NULL, 0, NULL, 0 }
    };

    // Default options
    FILE *f_out = stdout;
    //enum FormatOut format_out = OUT_ASCII;

    // Input arguments
    char *arg_fname_out = NULL;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "ho:", long_opts, &longindex)) != -1) {
        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
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

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing slow5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    double realtime0 = realtime();
    reads_count readsCount;
    std::vector<std::string> slow5_files;

    for (int i = optind; i < argc; ++ i) {
        find_all_5(argv[i], slow5_files, ASCII_EXTENSION);
    }
    fprintf(stderr, "[%s] %ld fast5 files found - took %.3fs\n", __func__, slow5_files.size(), realtime() - realtime0);

    if(slow5_files.size()){
        if(merge_slow5(f_out, slow5_files, &readsCount)==-1){
            return EXIT_FAILURE;
        }
    }
    MESSAGE(stderr, "total slow5: %lu, bad slow5: %lu multi-group slow5: %lu", readsCount.total_5, readsCount.bad_5_file, readsCount.multi_group_slow5);

    // Close output file
    if (arg_fname_out != NULL && fclose(f_out) == EOF) {
        ERROR("File '%s' failed on closing - %s.",
              arg_fname_out, strerror(errno));

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}

int merge_slow5(FILE *f_out, std::vector<std::string> &slow5_files, reads_count* readsCount) {
    fprintf(stderr,"merging...\n");
    size_t read_group_count = 0;
    size_t slow5_files_count = slow5_files.size();
    readsCount->total_5 = slow5_files_count;

    std::string slow5_path = "merged.slow5";
    FILE *slow5_file_pointer = NULL;
    slow5_file_pointer = fopen(slow5_path.c_str(), "w");

    // An error occured
    if (!slow5_file_pointer) {
        ERROR("File '%s' could not be opened - %s.", slow5_path.c_str(), strerror(errno));
    }
    slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), FORMAT_ASCII);
    slow5_hdr_initialize(slow5File->header);
    slow5File->header->num_read_groups = 0;

    if(!slow5File){
        fprintf(stderr, "cannot open %s. skipping...\n",slow5_files[0].c_str());
        return EXIT_FAILURE;
    }
    for(size_t i=0; i<slow5_files_count; i++) {
        slow5_file_t* slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            fprintf(stderr, "cannot open %s. skipping...\n",slow5_files[i].c_str());
            continue;
        }
        int64_t read_group_count_i = slow5File_i->header->num_read_groups;
        fprintf(stderr,"reading slow5 file '%s' it has %u read groups\n", slow5_files[i].c_str(), read_group_count_i);
        size_t read_group_tracker [read_group_count_i];
        for(int64_t j=0; j<read_group_count_i; j++){
            char* run_id_j = slow5_hdr_get("run_id", j, slow5File_i->header);
            fprintf(stderr, "run_id = %s\n",run_id_j);
            int64_t read_group_count = slow5File->header->num_read_groups;
            size_t flag_run_id_found = 0;
            for(int64_t k=0; k<read_group_count; k++){
                char* run_id_k = slow5_hdr_get("run_id", k, slow5File->header);
                if(strcmp(run_id_j,run_id_k) == 0){
                    flag_run_id_found = 1;
                    read_group_tracker[j] = k;
                    break;
                }
            }
            if(flag_run_id_found == 0){
                int64_t new_read_group = slow5_hdr_add_rg(slow5File->header);
                fprintf(stderr, "new read_group = %ld\n", (long)new_read_group);
                if(new_read_group != read_group_count){ //sanity check
                    fprintf(stderr, "something's wrong\n");
                }
                read_group_tracker[j] = new_read_group;
                slow5_hdr_set("run_id", run_id_j, new_read_group, slow5File->header);
            }
        }

        struct slow5_rec *read = NULL;
        int ret;
//        struct press *press_ptr = press_init(to_compress);
        while ((ret = slow5_get_next(&read, slow5File_i)) == 0) {
            fprintf(stderr,"before read_group = %u\n",read->read_group);
            read->read_group = read_group_tracker[read->read_group];
            fprintf(stderr,"after read_group = %u\n",read->read_group);
            if (slow5_rec_fwrite(slow5File->fp, read, slow5File->header->aux_meta, FORMAT_ASCII, NULL) == -1) {
                slow5_rec_free(read);
                return -2;
            }
//            slow5_add_rec(read, slow5File);
        }
//        press_free(press_ptr);
        slow5_rec_free(read);
        slow5_close(slow5File_i);
    }
    fseek(slow5_file_pointer, 0, SEEK_SET);
    if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, FORMAT_ASCII, COMPRESS_NONE) == -1){
        fprintf(stderr, "Could not write the header\n");
        exit(EXIT_FAILURE);
    }
    slow5_close(slow5File);
    return 1;
}

int compare_headers(slow5_header_t& slow5Header1, slow5_header_t& slow5Header2, int compare_level) {
    switch (compare_level) {
        case 0:
            if(strcmp(slow5Header1.file_format,slow5Header2.file_format)!=0){
                fprintf(stderr,"file_format mismatch:%s %s\n",slow5Header1.file_format,slow5Header2.file_format);
                return -1;
            }
            if(strcmp(slow5Header1.file_version,slow5Header2.file_version)!=0){
                fprintf(stderr,"file_version mismatch:%s %s\n",slow5Header1.file_version,slow5Header2.file_version);
                return -1;
            }
            return 1;
            break;
        case 1:
            if(strcmp(slow5Header1.run_id,slow5Header2.run_id)!=0){
                fprintf(stderr,"run_id mismatch:%s %s\n",slow5Header1.run_id,slow5Header2.run_id);
                return -1;
            }
            if(strcmp(slow5Header1.sample_frequency,slow5Header2.sample_frequency)!=0){
                fprintf(stderr,"sample_frequency mismatch:%s %s\n",slow5Header1.sample_frequency,slow5Header2.sample_frequency);
                return -1;
            }
            //todo: implement more comparisons...

            break;
        default:
            fprintf(stderr,"internal error\n");
            break;
    }
    return 0;

}
