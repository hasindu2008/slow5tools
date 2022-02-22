/**
 * @file split.c
 * @brief split a SLOW5 in different ways
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au)
 * @date 27/02/2021
 */

#include <getopt.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include "error.h"
#include "cmd.h"
#include "misc.h"
#include "slow5_extra.h"
#include "read_fast5.h"

#define USAGE_MSG "Usage: %s [OPTIONS] [SLOW5_FILE/DIR] ...\n"
#define HELP_LARGE_MSG \
    "Split a single a SLOW5/BLOW5 file into multiple separate files.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    HELP_MSG_OUTPUT_FORMAT \
    HELP_MSG_OUTPUT_DIRECTORY \
    HELP_MSG_PRESS \
    "    -g, --groups                  split multi read group file into single read group files\n" \
    "    -r, --reads [INT]             split into n reads, i.e., each file will have n reads\n"    \
    "    -f, --files [INT]             split reads into n files evenly \n"              \
    HELP_MSG_PROCESSES \
    HELP_MSG_LOSSLESS \
    HELP_MSG_HELP \
    HELP_FORMATS_METHODS

extern int slow5tools_verbosity_level;
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

void split_child_worker(proc_arg_t args,
                        std::vector<std::string> &slow5_files,
                        char *output_dir,
                        program_meta *meta,
                        reads_count *readsCount,
                        meta_split_method metaSplitMethod,
                        enum slow5_fmt format_out,
                        slow5_press_method_t pressMethod,
                        size_t lossy) {

    readsCount->total_5 = args.endi-args.starti - 1;
    std::string extension = ".blow5";
    if(format_out == SLOW5_FORMAT_ASCII){
        extension = ".slow5";
    }
    for (int i = args.starti; i < args.endi; i++) {
        readsCount->total_5++;

        slow5_file_t* slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("Cannot open %s. Skipping.\n",slow5_files[i].c_str());
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
                ERROR("Cannot open %s. Skipping.\n",slow5_files[i].c_str());
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
                int ret0 = slow5_hdr_initialize(slow5File->header, lossy);
                if(ret0<0){
                    exit(EXIT_FAILURE);
                }
                slow5File->header->num_read_groups = 0;
                if(lossy==0){
                    slow5_aux_meta_t* aux_ptr = slow5File_i->header->aux_meta;
                    uint32_t num_aux_attrs = aux_ptr->num;
                    int aux_add_fail = 0;
                    for(uint32_t r=0; r<num_aux_attrs; r++){
                        if(aux_ptr->types[r]==SLOW5_ENUM || aux_ptr->types[r]==SLOW5_ENUM_ARRAY){
                            uint8_t n;
                            const char **enum_labels = (const char** )slow5_get_aux_enum_labels(slow5File_i->header, aux_ptr->attrs[r], &n);
                            if(!enum_labels){
                                aux_add_fail = 1;
                            }
                            if(slow5_aux_meta_add_enum(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r], enum_labels, n)){
                                aux_add_fail = 1;
                            }
                        }else{
                            if(slow5_aux_meta_add(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r])) {
                                aux_add_fail =1;
                            }
                        }
                        if(aux_add_fail){
                            ERROR("Could not initialize the record attribute '%s'", aux_ptr->attrs[r]);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                khash_t(slow5_s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract 0th read_group related data from ith slow5file
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
                struct slow5_press *press_ptr = slow5_press_init(pressMethod);
                if(!press_ptr){
                    ERROR("Could not initialize the slow5 compression method%s","");
                    exit(EXIT_FAILURE);
                }
                while ((ret = slow5_get_next(&read, slow5File_i)) >= 0) {
                    if (slow5_rec_fwrite(slow5File->fp, read, slow5File_i->header->aux_meta, format_out, press_ptr) == -1) {
                        slow5_rec_free(read);
                        slow5_press_free(press_ptr);
                        ERROR("Could not write the record to %s\n", slow5_path.c_str());
                        exit(EXIT_FAILURE);
                    }
                    record_count++;
                    if(record_count == metaSplitMethod.n){
                        break;
                    }
                }
                slow5_press_free(press_ptr);
                slow5_rec_free(read);

                if(format_out == SLOW5_FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
                if(ret == SLOW5_ERR_EOF){
                    if(record_count==0){
                        int del = remove(slow5_path.c_str());
                        if (del) {
                            WARNING("Deleting additional file %s failed\n", slow5_path.c_str());
                            perror("");
                        }
                    }
                    break;
                }
                file_count++;
            }
            slow5_close(slow5File_i);

        }else if(metaSplitMethod.splitMethod==FILE_SPLIT){ //FILE_SPLIT
            slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
            if(!slow5File_i){
                ERROR("Cannot open %s. Skipping...\n",slow5_files[i].c_str());
                return;
            }
            int number_of_records = 0;
            struct slow5_rec *read = NULL;
            int ret;
            while ((ret = slow5_get_next(&read, slow5File_i)) >= 0) {
                number_of_records++;
            }
            slow5_rec_free(read);
            slow5_close(slow5File_i);

            int limit = number_of_records/metaSplitMethod.n;
            int rem = number_of_records%metaSplitMethod.n;
            size_t file_count = 0;

            slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
            if(!slow5File_i){
                ERROR("Cannot open %s. Skipping.\n",slow5_files[i].c_str());
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
                int ret0 = slow5_hdr_initialize(slow5File->header, lossy);
                if(ret0<0){
                    exit(EXIT_FAILURE);
                }
                slow5File->header->num_read_groups = 0;
                if(lossy==0){
                    slow5_aux_meta_t* aux_ptr = slow5File_i->header->aux_meta;
                    uint32_t num_aux_attrs = aux_ptr->num;
                    int aux_add_fail = 0;
                    for(uint32_t r=0; r<num_aux_attrs; r++){
                        if(aux_ptr->types[r]==SLOW5_ENUM || aux_ptr->types[r]==SLOW5_ENUM_ARRAY){
                            uint8_t n;
                            const char **enum_labels = (const char** )slow5_get_aux_enum_labels(slow5File_i->header, aux_ptr->attrs[r], &n);
                            if(!enum_labels){
                                aux_add_fail = 1;
                            }
                            if(slow5_aux_meta_add_enum(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r], enum_labels, n)){
                                aux_add_fail = 1;
                            }
                        }else{
                            if(slow5_aux_meta_add(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r])) {
                                aux_add_fail =1;
                            }
                        }
                        if(aux_add_fail){
                            ERROR("Could not initialize the record attribute '%s'", aux_ptr->attrs[r]);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                khash_t(slow5_s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract 0th read_group related data from ith slow5file
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
                struct slow5_press *press_ptr = slow5_press_init(pressMethod);
                if(!press_ptr){
                    ERROR("Could not initialize the slow5 compression method%s","");
                    exit(EXIT_FAILURE);
                }
                while ((number_of_records_per_file > 0) && (ret = slow5_get_next(&read, slow5File_i)) >= 0) {
                    if (slow5_rec_fwrite(slow5File->fp, read, slow5File_i->header->aux_meta, format_out, press_ptr) == -1) {
                        slow5_rec_free(read);
                        ERROR("Could not write the record to %s\n", slow5_path.c_str());
                        exit(EXIT_FAILURE);
                    }
                    number_of_records_per_file--;
                }
                slow5_press_free(press_ptr);
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
                    ERROR("Cannot open %s. Skipping.\n",slow5_files[i].c_str());
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
                int ret0 = slow5_hdr_initialize(slow5File->header, lossy);
                if(ret0<0){
                    exit(EXIT_FAILURE);
                }
                slow5File->header->num_read_groups = 0;
                if(lossy==0){
                    slow5_aux_meta_t* aux_ptr = slow5File_i->header->aux_meta;
                    uint32_t num_aux_attrs = aux_ptr->num;
                    int aux_add_fail = 0;
                    for(uint32_t r=0; r<num_aux_attrs; r++){
                        if(aux_ptr->types[r]==SLOW5_ENUM || aux_ptr->types[r]==SLOW5_ENUM_ARRAY){
                            uint8_t n;
                            const char **enum_labels = (const char** )slow5_get_aux_enum_labels(slow5File_i->header, aux_ptr->attrs[r], &n);
                            if(!enum_labels){
                                aux_add_fail = 1;
                            }
                            if(slow5_aux_meta_add_enum(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r], enum_labels, n)){
                                aux_add_fail = 1;
                            }
                        }else{
                            if(slow5_aux_meta_add(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r])) {
                                aux_add_fail =1;
                            }
                        }
                        if(aux_add_fail){
                            ERROR("Could not initialize the record attribute '%s'", aux_ptr->attrs[r]);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                khash_t(slow5_s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
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
                struct slow5_press *press_ptr = slow5_press_init(pressMethod);
                if(!press_ptr){
                    ERROR("Could not initialize the slow5 compression method%s","");
                    exit(EXIT_FAILURE);
                }
                while ((ret = slow5_get_next(&read, slow5File_i)) >= 0) {
                    if(read->read_group == j){
                        read->read_group = 0; //since single read_group files are now created
                        if (slow5_rec_fwrite(slow5File->fp, read, slow5File_i->header->aux_meta, format_out, press_ptr) == -1) {
                            slow5_rec_free(read);
                            ERROR("Could not write the record to %s\n", slow5_path.c_str());
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                slow5_press_free(press_ptr);
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
        meta_split_method metaSplitMethod, enum slow5_fmt format_out, slow5_press_method_t pressMethod, size_t lossy) {
    int64_t num_slow5_files = slow5_files.size();
    if (iop > num_slow5_files) {
        iop = num_slow5_files;
    }
    VERBOSE("%d proceses will be used",iop);

    //create processes
    pid_t* pids = (pid_t*) malloc(iop*sizeof(pid_t));
    proc_arg_t* proc_args = (proc_arg_t*)malloc(iop*sizeof(proc_arg_t));
    MALLOC_CHK(pids);
    MALLOC_CHK(proc_args);

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
        free(proc_args);
        free(pids);
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
    free(proc_args);
    free(pids);
}

int split_main(int argc, char **argv, struct program_meta *meta){
    init_realtime = slow5_realtime();

    // Debug: print arguments
    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    // Default options
    static struct option long_opts[] = {
            {"help",        no_argument, NULL, 'h' }, //0
            {"to",          required_argument, NULL, 'b'},    //1
            {"compress",    no_argument, NULL, 'c'},  //2
            {"sig-compress",required_argument,  NULL, 's'}, //3
            {"out-dir",     required_argument, NULL, 'd' },  //4
            { "iop",        required_argument, NULL, 'p'},   //5
            { "lossless",   required_argument, NULL, 'l'}, //6
            { "groups",     no_argument, NULL, 'g'}, //7
            { "files",      required_argument, NULL, 'f'}, //8
            { "reads",      required_argument, NULL, 'r'}, //9
            {NULL, 0, NULL, 0 }
    };


    meta_split_method metaSplitMethod;
    metaSplitMethod.n = 0;
    metaSplitMethod.splitMethod = READS_SPLIT;

    opt_t user_opts;
    init_opt(&user_opts);

    int opt;
    int longindex = 0;
    // Parse options
    while ((opt = getopt_long(argc, argv, "hb:c:s:gl:f:r:d:p:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'h':
                DEBUG("displaying large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'b':
                user_opts.arg_fmt_out = optarg;
                break;
            case 'c':
                user_opts.arg_record_press_out = optarg;
                break;
            case 's':
                user_opts.arg_signal_press_out = optarg;
                break;
            case 'd':
                user_opts.arg_dir_out = optarg;
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
                user_opts.arg_lossless = optarg;
                break;
            case 'p':
                user_opts.arg_num_processes = optarg;
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }
    if(parse_num_processes(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_arg_lossless(&user_opts, argc, argv, meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_format_args(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(auto_detect_formats(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_compression_opts(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
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
    if(!user_opts.arg_dir_out){
        ERROR("The output directory must be specified %s","");
        return EXIT_FAILURE;
    }

    if(user_opts.arg_dir_out){
        struct stat st = {0};
        if (stat(user_opts.arg_dir_out, &st) == -1) {
            mkdir(user_opts.arg_dir_out, 0700);
        }else{
            std::vector< std::string > dir_list = list_directory(user_opts.arg_dir_out);
            if(dir_list.size()>2){
                ERROR("Output directory %s is not empty. Please remove it or specify another directory.",user_opts.arg_dir_out);
                return EXIT_FAILURE;
            }
        }
    }

    reads_count readsCount;
    std::vector<std::string> slow5_files;

    if(metaSplitMethod.splitMethod==READS_SPLIT){
        VERBOSE("An input slow5 file will be split such that each output file has %lu reads", metaSplitMethod.n);
    }else if(metaSplitMethod.splitMethod==FILE_SPLIT){
        VERBOSE("An input slow5 file will be split into %lu output files", metaSplitMethod.n);
    } else{
        VERBOSE("An input multi read group slow5 files will be split into single read group slow5 files %s","");
    }

    //measure file listing time
    double realtime0 = slow5_realtime();
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], slow5_files, 0, SLOW5_FILE_EXTENSION_REGEX);
    }
    VERBOSE("%ld slow5 files found - took %.3fs",slow5_files.size(), slow5_realtime() - realtime0);
    if(slow5_files.size()==0){
        ERROR("No slow5/blow5 files found. Exiting...%s","");
        return EXIT_FAILURE;
    }
    //measure slow5 splitting time
    slow5_press_method_t press_out = {user_opts.record_press_out,user_opts.signal_press_out};
    split_iop(user_opts.num_processes, slow5_files, user_opts.arg_dir_out, meta, &readsCount, metaSplitMethod, user_opts.fmt_out, press_out, user_opts.flag_lossy);
    VERBOSE("Splitting %ld s/blow5 files took %.3fs",slow5_files.size(), slow5_realtime() - init_realtime);

    return EXIT_SUCCESS;
}
