//
// Created by Hiruna on 2021-01-20.
//

#include <getopt.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include "error.h"
#include "cmd.h"
#include "slow5_extra.h"
#include "read_fast5.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "split slow5 files\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help                 display this message and exit\n" \
    "    -b, --binary               convert to blow5\n" \
    "    -c, --compress             convert to compressed blow5\n"   \
    "    -o, --output=[dir]         output directory\n"              \
    "    -f INT                     split into n files\n"              \
    "    -r INT                     split into n reads\n"              \
    "    -g                         split multi read group file into single read group files\n"              \
    "    --iop INT                  number of I/O processes to read slow5 files\n" \

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

void s2f_child_worker(proc_arg_t args, std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, reads_count *readsCount, meta_split_method metaSplitMethod, enum slow5_fmt format_out, enum press_method pressMethod) {

    readsCount->total_5 = args.endi-args.starti - 1;

    for (int i = args.starti; i < args.endi; i++) {
        readsCount->total_5++;

        slow5_file_t* slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
            readsCount->bad_5_file++;
            continue;
        }

        int64_t read_group_count_i = slow5File_i->header->num_read_groups;

        if (read_group_count_i > 1) {
            readsCount->multi_group_slow5++;
        }
        if(read_group_count_i == 1 && metaSplitMethod.splitMethod==GROUP_SPLIT){
            ERROR("The file %s already has a single read group", slow5_files[i].c_str());
            continue;
        }
        if(read_group_count_i > 1 && metaSplitMethod.splitMethod!=GROUP_SPLIT){
            ERROR("The file %s a multi read group file. Cannot use read split or file split", slow5_files[i].c_str());
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
                std::string slow5file = slow5_files[i].substr(slow5_files[i].find_last_of('/'),slow5_files[i].length() - slow5_files[i].find_last_of('/') - 6) + "_" + std::to_string(file_count) + ".slow5";
                std::string slow5_path = std::string(output_dir);
                slow5_path += slow5file;

                FILE* slow5_file_pointer =  NULL;
                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                if (!slow5_file_pointer) {
                    ERROR("Output file %s could not be opened - %s.", slow5_path.c_str(), strerror(errno));
                    return;
                }
                slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), format_out);
                slow5_hdr_initialize(slow5File->header, 0);
                slow5File->header->num_read_groups = 0;

                khash_t(s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract 0th read_group related data from ith slow5file
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg);

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

                if(format_out == FORMAT_BINARY){
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
//                fprintf(stderr, "file_count = %d, number_of_records_per_file = %d, number_of_records = %d\n", file_count, number_of_records_per_file, number_of_records);
                number_of_records -= number_of_records_per_file;
                rem--;
                std::string slow5file = slow5_files[i].substr(slow5_files[i].find_last_of('/'),slow5_files[i].length() - slow5_files[i].find_last_of('/') - 6) + "_" + std::to_string(file_count) + ".slow5";
                std::string slow5_path = std::string(output_dir);
                slow5_path += slow5file;

                FILE* slow5_file_pointer =  NULL;
                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                if (!slow5_file_pointer) {
                    ERROR("Output file %s could not be opened - %s.", slow5_path.c_str(), strerror(errno));
                    return;
                }
                slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), format_out);
                slow5_hdr_initialize(slow5File->header, 0);
                slow5File->header->num_read_groups = 0;

                khash_t(s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract 0th read_group related data from ith slow5file
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg);

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

                if(format_out == FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
                file_count++;
            }
            slow5_close(slow5File_i);

        }else if(metaSplitMethod.splitMethod == GROUP_SPLIT){ // GROUP_SPLIT

            for(size_t j=0; j<read_group_count_i; j++){
                slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
                if(!slow5File_i){
                    ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
                    return;
                }

                std::string slow5file = slow5_files[i].substr(slow5_files[i].find_last_of('/'),slow5_files[i].length() - slow5_files[i].find_last_of('/') - 6) + "_" + std::to_string(j) + ".slow5";
                std::string slow5_path = std::string(output_dir);
                slow5_path += slow5file;

                FILE* slow5_file_pointer =  NULL;
                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                if (!slow5_file_pointer) {
                    ERROR("Output file %s could not be opened - %s.", slow5_path.c_str(), strerror(errno));
                    return;
                }
                slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), format_out);
                slow5_hdr_initialize(slow5File->header, 0);
                slow5File->header->num_read_groups = 0;

                khash_t(s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg);

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

                if(format_out == FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
            }
        }
    }
}

void s2f_iop(int iop, std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, reads_count *readsCount,
        meta_split_method metaSplitMethod, enum slow5_fmt format_out, enum press_method pressMethod) {
    double realtime0 = slow5_realtime();
    int64_t num_slow5_files = slow5_files.size();

    //create processes
    std::vector<pid_t> pids_v(iop);
    std::vector<proc_arg_t> proc_args_v(iop);
    pid_t *pids = pids_v.data();
    proc_arg_t *proc_args = proc_args_v.data();

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
        s2f_child_worker(proc_args[0], slow5_files, output_dir, meta, readsCount, metaSplitMethod, format_out, pressMethod);
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
            s2f_child_worker(proc_args[t],slow5_files,output_dir, meta, readsCount, metaSplitMethod, format_out, pressMethod);
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
//    skip_forking:

    fprintf(stderr, "[%s] Parallel splitting slow5 is done - took %.3fs\n", __func__,  slow5_realtime() - realtime0);
}

void recurse_slow5_dir(const char *f_path, reads_count *readsCount, char *output_dir, program_meta *meta,
                       meta_split_method metaSplitMethod, enum slow5_fmt format_out, enum press_method pressMethod) {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(f_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // If it has the fast5 extension
            if (std::string(f_path).find(ASCII_EXTENSION)!= std::string::npos) {
                // Open FAST5 and convert to SLOW5 into f_out
                std::vector<std::string> slow5_files;
                slow5_files.push_back(f_path);
                s2f_iop(1, slow5_files, output_dir, meta, readsCount, metaSplitMethod, format_out, pressMethod);
            }

        } else {
            WARNING("File '%s' failed to open - %s.",
                    f_path, strerror(errno));
        }

    } else {
        fprintf(stderr, "[%s::%.3f*%.2f] Extracting slow5 from %s\n", __func__,
                slow5_realtime() - init_realtime, slow5_cputime() / (slow5_realtime() - init_realtime), f_path);

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
                recurse_slow5_dir(sub_f_path, readsCount, output_dir, meta, metaSplitMethod, format_out, pressMethod);

                free(sub_f_path);
                sub_f_path = NULL;
            }
        }

        closedir(dir);
    }
}

int split_main(int argc, char **argv, struct program_meta *meta){
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
            {"help", no_argument, NULL, 'h' }, //0
            {"binary", no_argument, NULL, 'b'},    //1
            {"compress", no_argument, NULL, 'c'},  //2
            {"output", required_argument, NULL, 'o' },  //3
            { "iop", required_argument, NULL, 0},   //4
            {NULL, 0, NULL, 0 }
    };

    // Input arguments
    char *arg_dir_out = NULL;
    int longindex = 0;
    char opt;
    int iop = 1;
    meta_split_method metaSplitMethod;
    // Default options
    enum slow5_fmt format_out = FORMAT_ASCII;
    enum press_method pressMethod = COMPRESS_NONE;

    // Parse options
    while ((opt = getopt_long(argc, argv, "hbcgf:r:o:", long_opts, &longindex)) != -1) {
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
            case  0 :
                if (longindex == 4) {
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

    double realtime0 = slow5_realtime();
    reads_count readsCount;
    std::vector<std::string> slow5_files;

    if(metaSplitMethod.splitMethod==READS_SPLIT){
        MESSAGE(stderr, "an input slow5 file will be split such that each output file has %lu reads", metaSplitMethod.n);
    }else if(metaSplitMethod.splitMethod==FILE_SPLIT){
        MESSAGE(stderr, "an input slow5 file will be split into %lu output files", metaSplitMethod.n);
    } else{
        MESSAGE(stderr, "an input multi read group slow5 files will be split into single read group slow5 files %s","");
    }


    for (int i = optind; i < argc; ++ i) {
//        fprintf(stderr,"%s",argv[i]);
        if(iop==1) {
            // Recursive way
            recurse_slow5_dir(argv[i], &readsCount, arg_dir_out, meta, metaSplitMethod, format_out, pressMethod);
        }else{
            find_all_5(argv[i], slow5_files, ASCII_EXTENSION);
        }
    }

    if(iop==1){
        MESSAGE(stderr, "total slow5: %lu, bad slow5: %lu multi-group slow5: %lu", readsCount.total_5, readsCount.bad_5_file, readsCount.multi_group_slow5);
    }else{
        fprintf(stderr, "[%s] %ld slow5 files found - took %.3fs\n", __func__, slow5_files.size(), slow5_realtime() - realtime0);
        s2f_iop(iop, slow5_files, arg_dir_out, meta, &readsCount, metaSplitMethod, format_out, pressMethod);
    }

    return EXIT_SUCCESS;
}
