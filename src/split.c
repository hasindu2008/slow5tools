//
// Created by Hiruna Samarkoon on 2021-01-20.
//

#include <getopt.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include "error.h"
#include "cmd.h"
#include "misc.h"
#include "slow5_extra.h"
#include "read_fast5.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Split a single a SLOW5/BLOW5 file into multiple separate files.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    --to [STR]                         output in the format specified in STR. slow5 for SLOW5 ASCII. blow5 for SLOW5 binary (BLOW5) [default: BLOW5] \n" \
    "    -c, --compress [compression_type]  convert to compressed blow5 [default: gzip]\n" \
    "    -d, --out-dir [STR]                output directory where files are written to\n" \
    "    -f, --files [INT]                  split reads into n files evenly\n"              \
    "    -r, --reads [INT]                  split into n reads, i.e., each file will have n reads\n"              \
    "    -g, --groups                       split multi read group file into single read group files\n" \
    "    -l, --lossless [STR]               retain information in auxilliary fields during the conversion.[default: true].\n" \
    "    -p, --iop [INT]                    number of I/O processes used to split files [default: 8]\n" \
    "    -h, --help                         display this message and exit\n" \

static double init_realtime = 0;

enum SplitMethod {
    READS_SPLIT,
    FILE_SPLIT,
    GROUP_SPLIT,
};

typedef struct {
    SplitMethod splitMethod = READS_SPLIT;
    size_t n;
}meta_split_method;

void split_child_worker(proc_arg_t args, std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, reads_count *readsCount, meta_split_method metaSplitMethod, enum slow5_fmt format_out, enum press_method pressMethod, size_t lossy) {

    readsCount->total_5 = args.endi-args.starti - 1;
    std::string extension = ".blow5";
    if(format_out == SLOW5_FORMAT_ASCII){
        extension = ".slow5";
    }
    for (int i = args.starti; i < args.endi; i++) {
        readsCount->total_5++;

        slow5_file_t* slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
            readsCount->bad_5_file++;
            continue;
        }

        uint32_t read_group_count_i = slow5File_i->header->num_read_groups;

        if (read_group_count_i > 1) {
            readsCount->multi_group_slow5++;
        }
        if(read_group_count_i == 1 && metaSplitMethod.splitMethod==GROUP_SPLIT){
            ERROR("The file %s already has a single read group", slow5_files[i].c_str());
            continue;
        }
        if(read_group_count_i > 1 && metaSplitMethod.splitMethod!=GROUP_SPLIT){
            ERROR("The file %s contains multiple read groups. You must first separate the read groups using -g. See https://slow5.page.link/faq for more info.", slow5_files[i].c_str());
            continue;
        }

        slow5_close(slow5File_i); //todo-implement a method to fseek() to the first record of the slow5File_i

        //READ_SPLIT
        if(metaSplitMethod.splitMethod==READS_SPLIT) {
            slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
            if(!slow5File_i){
                ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
                return;
            }
            size_t file_count = 0;
            while(1){
                int last_slash = slow5_files[i].find_last_of('/');
                if(last_slash == -1){
                    last_slash = 0;
                }
                std::string slow5file = slow5_files[i].substr(last_slash,slow5_files[i].length() - last_slash - extension.length()) + "_" + std::to_string(file_count) + extension;
                std::string slow5_path = std::string(output_dir) + "/";
                slow5_path += slow5file;

                FILE* slow5_file_pointer =  NULL;
                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                if (!slow5_file_pointer) {
                    ERROR("Output file %s could not be opened - %s.", slow5_path.c_str(), strerror(errno));
                    return;
                }
                slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), format_out);
                slow5_hdr_initialize(slow5File->header, lossy);
                slow5File->header->num_read_groups = 0;

                khash_t(s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract 0th read_group related data from ith slow5file
                if(slow5_hdr_add_rg_data(slow5File->header, rg) < 0){
                    ERROR("Could not add read group to %s\n", slow5_path.c_str());
                    exit(EXIT_FAILURE);
                }

                if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, pressMethod) == -1){ //now write the header to the slow5File
                    ERROR("Could not write the header to %s\n", slow5_path.c_str());
                    exit(EXIT_FAILURE);
                }

                size_t record_count = 0;
                struct slow5_rec *read = NULL;
                int ret;
                struct press *press_ptr = press_init(pressMethod);
                while ((ret = slow5_get_next(&read, slow5File_i)) == 0) {
                    if (slow5_rec_fwrite(slow5File->fp, read, slow5File_i->header->aux_meta, format_out, press_ptr) == -1) {
                        slow5_rec_free(read);
                        ERROR("Could not write the record to %s\n", slow5_path.c_str());
                        exit(EXIT_FAILURE);
                    }
                    record_count++;
                    if(record_count == metaSplitMethod.n){
                        break;
                    }
                }
                press_free(press_ptr);
                slow5_rec_free(read);

                if(format_out == SLOW5_FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
                if(ret != 0){
                    break;
                }
                file_count++;
            }
            slow5_close(slow5File_i);

        }else if(metaSplitMethod.splitMethod==FILE_SPLIT){ //FILE_SPLIT
            slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
            if(!slow5File_i){
                ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
                return;
            }
            int number_of_records = 0;
            struct slow5_rec *read = NULL;
            int ret;
            while ((ret = slow5_get_next(&read, slow5File_i)) == 0) {
                number_of_records++;
            }
            slow5_rec_free(read);
            slow5_close(slow5File_i);

            int limit = number_of_records/metaSplitMethod.n;
            int rem = number_of_records%metaSplitMethod.n;
            size_t file_count = 0;

            slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
            if(!slow5File_i){
                ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
                return;
            }

            while(number_of_records > 0) {
                int number_of_records_per_file = (rem > 0) ? 1 : 0;
                number_of_records_per_file += limit;
                // fprintf(stderr, "file_count = %d, number_of_records_per_file = %d, number_of_records = %d\n", file_count, number_of_records_per_file, number_of_records);
                number_of_records -= number_of_records_per_file;
                rem--;
                int last_slash = slow5_files[i].find_last_of('/');
                if(last_slash == -1){
                    last_slash = 0;
                }
                std::string slow5file = slow5_files[i].substr(last_slash,slow5_files[i].length() - last_slash - extension.length()) + "_" + std::to_string(file_count) + extension;
                std::string slow5_path = std::string(output_dir) + "/";
                slow5_path += slow5file;

                FILE* slow5_file_pointer =  NULL;
                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                if (!slow5_file_pointer) {
                    ERROR("Output file %s could not be opened - %s.", slow5_path.c_str(), strerror(errno));
                    return;
                }
                slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), format_out);
                slow5_hdr_initialize(slow5File->header, lossy);
                slow5File->header->num_read_groups = 0;

                khash_t(s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract 0th read_group related data from ith slow5file
                if(slow5_hdr_add_rg_data(slow5File->header, rg) < 0){
                    ERROR("Could not add read group to %s\n", slow5_path.c_str());
                    exit(EXIT_FAILURE);
                }
                if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, pressMethod) == -1){ //now write the header to the slow5File
                    ERROR("Could not write the header to %s\n", slow5_path.c_str());
                    exit(EXIT_FAILURE);
                }

                struct slow5_rec *read = NULL;
                int ret;
                struct press *press_ptr = press_init(pressMethod);
                while ((number_of_records_per_file > 0) && (ret = slow5_get_next(&read, slow5File_i)) == 0) {
                    if (slow5_rec_fwrite(slow5File->fp, read, slow5File_i->header->aux_meta, format_out, press_ptr) == -1) {
                        slow5_rec_free(read);
                        ERROR("Could not write the record to %s\n", slow5_path.c_str());
                        exit(EXIT_FAILURE);
                    }
                    number_of_records_per_file--;
                }
                press_free(press_ptr);
                slow5_rec_free(read);

                if(format_out == SLOW5_FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
                file_count++;
            }
            slow5_close(slow5File_i);

        }else if(metaSplitMethod.splitMethod == GROUP_SPLIT){ // GROUP_SPLIT

            for(uint32_t j=0; j<read_group_count_i; j++){
                slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
                if(!slow5File_i){
                    ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
                    return;
                }
                int last_slash = slow5_files[i].find_last_of('/');
                if(last_slash == -1){
                    last_slash = 0;
                }
                std::string slow5file = slow5_files[i].substr(last_slash,slow5_files[i].length() - last_slash - extension.length()) + "_" + std::to_string(j) + extension;
                std::string slow5_path = std::string(output_dir) + "/";
                slow5_path += slow5file;

                FILE* slow5_file_pointer =  NULL;
                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                if (!slow5_file_pointer) {
                    ERROR("Output file %s could not be opened - %s.", slow5_path.c_str(), strerror(errno));
                    return;
                }
                slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), format_out);
                slow5_hdr_initialize(slow5File->header, lossy);
                slow5File->header->num_read_groups = 0;

                khash_t(s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
                if(slow5_hdr_add_rg_data(slow5File->header, rg) < 0){
                    ERROR("Could not add read group to %s\n", slow5_path.c_str());
                    exit(EXIT_FAILURE);
                }

                if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, pressMethod) == -1){ //now write the header to the slow5File
                    ERROR("Could not write the header to %s\n", slow5_path.c_str());
                    exit(EXIT_FAILURE);
                }

                struct slow5_rec *read = NULL;
                int ret;
                struct press *press_ptr = press_init(pressMethod);
                while ((ret = slow5_get_next(&read, slow5File_i)) == 0) {
                    if(read->read_group == j){
                        read->read_group = 0; //since single read_group files are now created
                        if (slow5_rec_fwrite(slow5File->fp, read, slow5File_i->header->aux_meta, format_out, press_ptr) == -1) {
                            slow5_rec_free(read);
                            ERROR("Could not write the record to %s\n", slow5_path.c_str());
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                press_free(press_ptr);
                slow5_rec_free(read);
                slow5_close(slow5File_i);

                if(format_out == SLOW5_FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
            }
        }
    }
}

void split_iop(int iop, std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, reads_count *readsCount,
        meta_split_method metaSplitMethod, enum slow5_fmt format_out, enum press_method pressMethod, size_t lossy) {
    int64_t num_slow5_files = slow5_files.size();
    if (iop > num_slow5_files) {
        iop = num_slow5_files;
        INFO("Only %d proceses will be used",iop);
    }

    //create processes
    pid_t pids[iop];
    proc_arg_t proc_args[iop];

    int32_t t;
    int32_t i = 0;
    int32_t step = (num_slow5_files + iop - 1) / iop;
    //todo : check for higher num of procs than the data
    //current works but many procs are created despite

    //set the data structures
    for (t = 0; t < iop; t++) {
        proc_args[t].starti = i;
        i += step;
        if (i > num_slow5_files) {
            proc_args[t].endi = num_slow5_files;
        } else {
            proc_args[t].endi = i;
        }
        proc_args[t].proc_index = t;
    }

    if(iop==1){
        split_child_worker(proc_args[0], slow5_files, output_dir, meta, readsCount, metaSplitMethod, format_out, pressMethod, lossy);
//        goto skip_forking;
        return;
    }

    //create processes
    STDERR("Spawning %d I/O processes...", iop);
    for(t = 0; t < iop; t++){
        pids[t] = fork();

        if(pids[t]==-1){
            ERROR("%s","Fork failed");
            perror("");
            exit(EXIT_FAILURE);
        }
        if(pids[t]==0){ //child
            split_child_worker(proc_args[t],slow5_files,output_dir, meta, readsCount, metaSplitMethod, format_out, pressMethod, lossy);
            exit(EXIT_SUCCESS);
        }
        if(pids[t]>0){ //parent
            continue;
        }
    }

    //wait for processes
    int status,w;
    for (t = 0; t < iop; t++) {
//        if(opt::verbose>1){
//            STDERR("parent : Waiting for child with pid %d",pids[t]);
//        }
        w = waitpid(pids[t], &status, 0);
        if (w == -1) {
            ERROR("%s","waitpid failed");
            perror("");
            exit(EXIT_FAILURE);
        }
        else if (WIFEXITED(status)){
//            if(opt::verbose>1){
//                STDERR("child process %d exited, status=%d", pids[t], WEXITSTATUS(status));
//            }
            if(WEXITSTATUS(status)!=0){
                ERROR("child process %d exited with status=%d",pids[t], WEXITSTATUS(status));
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (WIFSIGNALED(status)) {
                ERROR("child process %d killed by signal %d", pids[t], WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                ERROR("child process %d stopped by signal %d", pids[t], WSTOPSIG(status));
            } else {
                ERROR("child process %d did not exit propoerly: status %d", pids[t], status);
            }
            exit(EXIT_FAILURE);
        }
    }
}

int split_main(int argc, char **argv, struct program_meta *meta){
    init_realtime = slow5_realtime();

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

    // code from f2s
    int iop = 8;
    int lossy = 0;

    // Default options
    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h' }, //0
            {"to", required_argument, NULL, 'b'},    //1
            {"compress", no_argument, NULL, 'c'},  //2
            {"out-dir", required_argument, NULL, 'd' },  //3
            { "iop", required_argument, NULL, 'p'},   //4
            { "lossless", required_argument, NULL, 'l'}, //5
            { "groups", no_argument, NULL, 'g'}, //6
            { "files", required_argument, NULL, 'f'}, //7
            { "reads", required_argument, NULL, 'r'}, //8
            {NULL, 0, NULL, 0 }
    };


    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum press_method pressMethod = COMPRESS_GZIP;
    meta_split_method metaSplitMethod;
    metaSplitMethod.n = 0;
    metaSplitMethod.splitMethod = READS_SPLIT;

    // Input arguments
    char *arg_dir_out = NULL;

    int opt;
    int longindex = 0;
    // Parse options
    while ((opt = getopt_long(argc, argv, "hb:cgl:f:r:d:p:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'h':
                if (meta->verbosity_level >= LOG_VERBOSE) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'b':
                if(strcmp(optarg,"slow5")==0){
                    format_out = SLOW5_FORMAT_ASCII;
                    pressMethod = COMPRESS_NONE;
                }else if(strcmp(optarg,"blow5")==0){
                    format_out = SLOW5_FORMAT_BINARY;
                }else{
                    ERROR("Incorrect output format%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'c':
                if(strcmp(optarg,"none")==0){
                    pressMethod = COMPRESS_NONE;
                }else if(strcmp(optarg,"gzip")==0){
                    pressMethod = COMPRESS_GZIP;
                }else{
                    ERROR("Incorrect compression type%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                arg_dir_out = optarg;
                break;
            case 'f':
                metaSplitMethod.splitMethod = FILE_SPLIT;
                metaSplitMethod.n = atoi(optarg);
                break;
            case 'r':
                metaSplitMethod.splitMethod = READS_SPLIT;
                metaSplitMethod.n = atoi(optarg);
                break;
            case 'g':
                metaSplitMethod.splitMethod = GROUP_SPLIT;
                break;
            case 'l':
                if(strcmp(optarg,"true")==0){
                    lossy = 0;
                }else if(strcmp(optarg,"false")==0){
                    lossy = 1;
                }else{
                    ERROR("Incorrect argument%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                iop = atoi(optarg);
                if (iop < 1) {
                    ERROR("Number of I/O processes should be larger than 0. You entered %d", iop);
                    exit(EXIT_FAILURE);
                }
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    // compression option is only effective with -b blow5
    if(format_out==SLOW5_FORMAT_ASCII && pressMethod!=COMPRESS_NONE){
        ERROR("Compression option is only effective with SLOW5 binary format%s","");
        return EXIT_FAILURE;
    }

    if(metaSplitMethod.splitMethod==READS_SPLIT && metaSplitMethod.n==0){
        ERROR("Default splitting method - reads split is used. Specify the number of reads to include in a slow5 file%s","");
        return EXIT_FAILURE;
    }
    if(metaSplitMethod.splitMethod==FILE_SPLIT && metaSplitMethod.n==0){
        ERROR("Splitting method - files split is used. Specify the number of files to create from a slow5 file%s","");
        return EXIT_FAILURE;
    }
    if(!arg_dir_out){
        ERROR("The output directory must be specified %s","");
        return EXIT_FAILURE;
    }

    if(arg_dir_out){
        struct stat st = {0};
        if (stat(arg_dir_out, &st) == -1) {
            mkdir(arg_dir_out, 0700);
        }else{
            std::vector< std::string > dir_list = list_directory(arg_dir_out);
            if(dir_list.size()>2){
                ERROR("Output director %s is not empty",arg_dir_out);
                return EXIT_FAILURE;
            }
        }
    }

    reads_count readsCount;
    std::vector<std::string> slow5_files;

    if(metaSplitMethod.splitMethod==READS_SPLIT){
        MESSAGE(stderr, "an input slow5 file will be split such that each output file has %lu reads", metaSplitMethod.n);
    }else if(metaSplitMethod.splitMethod==FILE_SPLIT){
        MESSAGE(stderr, "an input slow5 file will be split into %lu output files", metaSplitMethod.n);
    } else{
        MESSAGE(stderr, "an input multi read group slow5 files will be split into single read group slow5 files %s","");
    }

    //measure file listing time
    double realtime0 = slow5_realtime();
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], slow5_files, 0, NULL);
    }
    fprintf(stderr, "[%s] %ld slow5 files found - took %.3fs\n", __func__, slow5_files.size(), slow5_realtime() - realtime0);

    //measure slow5 splitting time
    split_iop(iop, slow5_files, arg_dir_out, meta, &readsCount, metaSplitMethod, format_out, pressMethod, lossy);
    fprintf(stderr, "[%s] Splitting %ld s/blow5 files using %d process - took %.3fs\n", __func__, slow5_files.size(), iop, slow5_realtime() - init_realtime);

    return EXIT_SUCCESS;
}
