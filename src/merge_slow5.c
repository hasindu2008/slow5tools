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
    "    -b, --binary               convert to blow5\n" \
    "    -c, --compress             convert to compressed blow5\n"   \
    "    -o, --output=[FILE]        output converted contents to FILE -- stdout\n" \

static double init_realtime = 0;

int merge_slow5(const char* output_path, enum slow5_fmt format_out, enum press_method pressMethod, std::vector<std::string> &slow5_files, reads_count* readsCount) {
    size_t read_group_count = 0;
    size_t slow5_files_count = slow5_files.size();
    readsCount->total_5 = slow5_files_count;

    FILE* slow5_file_pointer = stdout;
    if(output_path){
        slow5_file_pointer = NULL;
        slow5_file_pointer = fopen(output_path, "w");
        if (!slow5_file_pointer) {
            ERROR("Output file %s could not be opened - %s.", output_path, strerror(errno));
            return -1;
        }
    }
    slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, output_path, format_out);
    slow5_hdr_initialize(slow5File->header, 1);
    slow5File->header->num_read_groups = 0;
    std::vector<std::vector<size_t>> list;
    for(size_t i=0; i<slow5_files_count; i++) { //iterate over slow5files
        slow5_file_t* slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
            continue;
        }
        int64_t read_group_count_i = slow5File_i->header->num_read_groups; // number of read_groups in ith slow5file
        std::vector<size_t> read_group_tracker(read_group_count_i); //this array will store the new group_numbers (of the slow5File) to where the jth read_group info should be written
        list.push_back(read_group_tracker);
        for(int64_t j=0; j<read_group_count_i; j++){
            char* run_id_j = slow5_hdr_get("run_id", j, slow5File_i->header); // run_id of the jth read_group of the ith slow5file
            int64_t read_group_count = slow5File->header->num_read_groups; //since this might change during iterating; cannot know beforehand
            size_t flag_run_id_found = 0;
            for(int64_t k=0; k<read_group_count; k++){
                char* run_id_k = slow5_hdr_get("run_id", k, slow5File->header);
                if(strcmp(run_id_j,run_id_k) == 0){
                    flag_run_id_found = 1;
                    list[i][j] = k; //assumption0: run_ids are similar. Hence, the rest of the header attribute values of jth read_group are same as kth read_group's.
                    break;
                }
            }
            if(flag_run_id_found == 0){ // time to add a new read_group
                khash_t(s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg); //assumption0
                if(new_read_group != read_group_count){ //sanity check
                    WARNING("New read group number is not equal to number of groups; something's wrong\n%s", "");
                }
                list[i][j] = new_read_group;
            }
        }
        slow5_close(slow5File_i);
    }

    if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, pressMethod) == -1){ //now write the header to the slow5File
        ERROR("Could not write the header to %s\n","merge.slow5");
        exit(EXIT_FAILURE);
    }

    for(size_t i=0; i<slow5_files_count; i++) { //iterate over slow5files
        slow5_file_t *slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if (!slow5File_i) {
            ERROR("cannot open %s. skipping...\n", slow5_files[i].c_str());
            continue;
        }
        struct slow5_rec *read = NULL;
        int ret;
        struct press *press_ptr = press_init(pressMethod);
        while ((ret = slow5_get_next(&read, slow5File_i)) == 0) {
            read->read_group = list[i][read->read_group]; //write records of the ith slow5file with the updated read_group value
            if (slow5_rec_fwrite(slow5File->fp, read, slow5File->header->aux_meta, format_out, press_ptr) == -1) {
                slow5_rec_free(read);
                return -1;
            }
        }
        press_free(press_ptr);
        slow5_rec_free(read);
        slow5_close(slow5File_i);
    }

    if(format_out == FORMAT_BINARY){
        slow5_eof_fwrite(slow5File->fp);
    }
    slow5_close(slow5File);

    return 1;
}

int merge_main(int argc, char **argv, struct program_meta *meta){
    init_realtime = slow5_realtime();

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
            {"binary", no_argument, NULL, 'b'},    //1
            {"compress", no_argument, NULL, 'c'},  //2
            {"output", required_argument, NULL, 'o'},   //3
            {NULL, 0, NULL, 0 }
    };

    // Default options
    FILE *f_out = stdout;
    // Default options
    enum slow5_fmt format_out = FORMAT_ASCII;
    enum press_method pressMethod = COMPRESS_NONE;

    // Input arguments
    char *arg_fname_out = NULL;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "bcho:", long_opts, &longindex)) != -1) {
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
            case 'b':
                format_out = FORMAT_BINARY;
                break;
            case 'c':
                pressMethod = COMPRESS_GZIP;
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

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing slow5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    double realtime0 = slow5_realtime();
    reads_count readsCount;
    std::vector<std::string> slow5_files;

    for (int i = optind; i < argc; ++ i) {
        find_all_5(argv[i], slow5_files, ASCII_EXTENSION);
    }
    fprintf(stderr, "[%s] %ld fast5 files found - took %.3fs\n", __func__, slow5_files.size(), slow5_realtime() - realtime0);

    if(slow5_files.size()){
        if(merge_slow5(arg_fname_out, format_out, pressMethod, slow5_files, &readsCount)==-1){
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

