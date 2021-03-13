//
// Created by Hiruna on 2021-01-20.
//

#include "slow5_old.h"
#include "error.h"
#include "cmd.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "split slow5 files\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help                 display this message and exit\n" \
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

void create_slow5_files(std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, size_t i, operator_obj* tracker, FILE *slow5, meta_split_method metaSplitMethod, size_t number_of_records, size_t file_number){

    size_t limit;
    size_t uneven_bins;

    if(metaSplitMethod.splitMethod==READS_SPLIT){
        limit = metaSplitMethod.n;
    }else if(metaSplitMethod.splitMethod==FILE_SPLIT){
        uneven_bins = metaSplitMethod.n - (number_of_records%metaSplitMethod.n);
        if(number_of_records<metaSplitMethod.n){
            limit = 1;
        }else{
            limit = number_of_records/metaSplitMethod.n;
        }
    }


    size_t flag_end_of_file = 0;
    while(1) {

        std::string slow5file = slow5_files[i].substr(slow5_files[i].find_last_of('/'),slow5_files[i].length() - slow5_files[i].find_last_of('/') - 6) + "_" + std::to_string(file_number) + ".slow5";
        std::string slow5_path = std::string(output_dir);
        slow5_path += slow5file;
//        fprintf(stderr, "file_number=%lu %s\n", file_number, slow5_path.c_str());

        FILE *f_out = NULL;
        f_out = fopen(slow5_path.c_str(), "w");

        // An error occured
        if (!f_out) {
            ERROR("File '%s' could not be opened - %s. Aborting..", slow5_path.c_str(), strerror(errno));
            return;
        }
        tracker->f_out = f_out;
        print_slow5_header(tracker);//remove this later; added for the sake of slow5 format completeness
        if(metaSplitMethod.splitMethod == GROUP_SPLIT){

            size_t prev_read_group_id;
            size_t first_record = 1;
            while(1){
                char* buffer = NULL;
                if(read_line(slow5, &buffer) == -1){
                    flag_end_of_file = 1;
                    break;
                }
                char read_id[50];
                size_t read_group_id;
                if(sscanf(buffer, "%[^\t]\t%lu", read_id, &read_group_id)!=2){
                    ERROR("Slow5 format error in line: %s",buffer);
                }
                if(first_record==0){
                    if(read_group_id!=prev_read_group_id){
                        fseek(slow5,-sizeof(char)*strlen(buffer),SEEK_CUR);
                        free(buffer);
                        flag_end_of_file = 1;
                        break;
                    }
                }
                fprintf(f_out,"%s", buffer);
                prev_read_group_id = read_group_id;
                first_record=0;
                free(buffer);
            }
        }
        else {
            if(metaSplitMethod.splitMethod == FILE_SPLIT && file_number == uneven_bins){
                    limit +=1;
            }
            for (size_t t = 0; t < limit; t++) {
                char *buffer = NULL;
                if (read_line(slow5, &buffer) == -1) {
                    flag_end_of_file = 1;
                    break;
                }
                fprintf(f_out, "%s", buffer);
                free(buffer);
            }
        }
        if (fclose(f_out) == EOF) {
            WARNING("File '%s' failed on closing.", slow5_path.c_str());
        }

        if(flag_end_of_file == 0){  //check if the for loop finished just before an EOF
            char *buffer = NULL;
            if (read_line(slow5, &buffer) == -1) {
                flag_end_of_file = 1;
            } else{ // continue
                fseek(slow5,-sizeof(char)*strlen(buffer),SEEK_CUR);
                free(buffer);
            }
        }
        if(flag_end_of_file == 1){
            break;
        }
        file_number++;
    }

}

void s2f_child_worker(proc_arg_t args, std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, reads_count *readsCount, meta_split_method metaSplitMethod) {

    readsCount->total_5 = args.endi-args.starti - 1;

    for (int i = args.starti; i < args.endi; i++) {
        std::vector<slow5_header_t> slow5_headers;

        readsCount->total_5++;
        std::vector<slow5_header_t> slow5headers;
        FILE *slow5;
        slow5 = fopen(slow5_files[i].c_str(), "r"); // read mode
        if (slow5 == NULL) {
            WARNING("slow5 file [%s] is unreadable and will be skipped", slow5_files[i].c_str());
            readsCount->bad_5_file++;
            continue;
        }

        hsize_t num_read_group;
        if(find_num_read_group(slow5, &num_read_group)==-1){
            WARNING("slow5 file [%s] is unreadable and will be skipped", slow5_files[i].c_str());
            continue;
        }
        if (num_read_group > 1) {
            readsCount->multi_group_slow5++;
        }
        if(num_read_group==1 && metaSplitMethod.splitMethod==GROUP_SPLIT){
            ERROR("The file %s already has a single read group", slow5_files[i].c_str());
            continue;
        }
        if(num_read_group>1 && metaSplitMethod.splitMethod!=GROUP_SPLIT){
            ERROR("The file %s a multi read group file. Cannot use read split or file split", slow5_files[i].c_str());
            continue;
        }
        for(size_t j=0; j<num_read_group; j++){
            slow5_header_t slow5_header;
            slow5_headers.push_back(slow5_header);
        }
        if (read_slow5_header(slow5, slow5_headers, num_read_group) == -1) {
            WARNING("slow5 file [%s] is unreadable and will be skipped", slow5_files[i].c_str());
            readsCount->bad_5_file++;
            continue;
        }

        //READ_SPLIT
        if(metaSplitMethod.splitMethod==READS_SPLIT) {
            struct operator_obj tracker;
            tracker.meta = meta;
            tracker.slow5_header = &slow5_headers[0];

            create_slow5_files(slow5_files, output_dir, meta, i, &tracker, slow5, metaSplitMethod, 0, 0);

//            free_attributes(READ, &tracker);
//            free_attributes(ROOT, &tracker);
//            free_attributes(CONTEXT_TAGS, &tracker);
//            free_attributes(TRACKING_ID, &tracker);
            if ((char *) slow5_headers[0].file_format){
                free((char *) slow5_headers[0].file_format);
            }

        }else if(metaSplitMethod.splitMethod==FILE_SPLIT){ //FILE_SPLIT

            struct operator_obj tracker;
            tracker.meta = meta;
            tracker.slow5_header = &slow5_headers[0];

            size_t number_of_records = 0;
            size_t checkpoint = ftell(slow5);
            char *buffer = NULL;
            while(read_line(slow5, &buffer) != -1){
                number_of_records++;
            }
            fseek(slow5, checkpoint, SEEK_SET);

//            fprintf(stderr, "#records=%lu #files=%lu limit=%lu\n",number_of_records, metaSplitMethod.n, limit);
            create_slow5_files(slow5_files, output_dir, meta, i, &tracker, slow5, metaSplitMethod, number_of_records, 0);

//            free_attributes(READ, &tracker);
//            free_attributes(ROOT, &tracker);
//            free_attributes(CONTEXT_TAGS, &tracker);
//            free_attributes(TRACKING_ID, &tracker);
            if ((char *) slow5_headers[0].file_format){
                free((char *) slow5_headers[0].file_format);
            }
        }else if(metaSplitMethod.splitMethod == GROUP_SPLIT){ // GROUP_SPLIT

            for(size_t j=0; j<num_read_group; j++){
                struct operator_obj tracker;
                tracker.meta = meta;
                tracker.slow5_header = &slow5_headers[j];
                tracker.slow5_header->num_read_groups = 1;

                create_slow5_files(slow5_files, output_dir, meta, i, &tracker, slow5, metaSplitMethod, 0, j);

//                free_attributes(READ, &tracker);
//                free_attributes(ROOT, &tracker);
//                free_attributes(CONTEXT_TAGS, &tracker);
//                free_attributes(TRACKING_ID, &tracker);
                if ((char *) slow5_headers[j].file_format){
                    free((char *) slow5_headers[j].file_format);
                }
            }
        }


        //  Close the slow5 file.
        if(fclose(slow5) == EOF) {
            WARNING("File '%s' failed on closing.", slow5_files[i].c_str());
        }
    }
}


void s2f_iop(int iop, std::vector<std::string> &slow5_files, char *output_dir, program_meta *meta, reads_count *readsCount,
        meta_split_method metaSplitMethod) {
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
        s2f_child_worker(proc_args[0], slow5_files, output_dir, meta, readsCount, metaSplitMethod);
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
            s2f_child_worker(proc_args[t],slow5_files,output_dir, meta, readsCount, metaSplitMethod);
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
                       meta_split_method metaSplitMethod) {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(f_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // If it has the fast5 extension
            if (std::string(f_path).find(SLOW5_EXTENSION)!= std::string::npos) {
                // Open FAST5 and convert to SLOW5 into f_out
                std::vector<std::string> slow5_files;
                slow5_files.push_back(f_path);
                s2f_iop(1, slow5_files, output_dir, meta, readsCount, metaSplitMethod);
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
                recurse_slow5_dir(sub_f_path, readsCount, output_dir, meta, metaSplitMethod);

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
            {"output", required_argument, NULL, 'o' },  //1
            { "iop", required_argument, NULL, 0},   //2
            {NULL, 0, NULL, 0 }
    };

    // Input arguments
    char *arg_dir_out = NULL;
    int longindex = 0;
    char opt;
    int iop = 1;
    meta_split_method metaSplitMethod;

    // Parse options
    while ((opt = getopt_long(argc, argv, "hgf:r:o:", long_opts, &longindex)) != -1) {
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
                if (longindex == 2) {
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
            recurse_slow5_dir(argv[i], &readsCount, arg_dir_out, meta, metaSplitMethod);
        }else{
            find_all_5(argv[i], slow5_files, SLOW5_EXTENSION);
        }
    }

    if(iop==1){
        MESSAGE(stderr, "total slow5: %lu, bad slow5: %lu multi-group slow5: %lu", readsCount.total_5, readsCount.bad_5_file, readsCount.multi_group_slow5);
    }else{
        fprintf(stderr, "[%s] %ld slow5 files found - took %.3fs\n", __func__, slow5_files.size(), slow5_realtime() - realtime0);
        s2f_iop(iop, slow5_files, arg_dir_out, meta, &readsCount, metaSplitMethod);
    }

    return EXIT_SUCCESS;
}
