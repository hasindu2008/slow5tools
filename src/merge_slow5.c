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

#define DEFAULT_NUM_THREADS (4)
#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "merge slow5 files\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help                 display this message and exit\n" \
    "    -@, --threads=[INT]    number of threads -- 4\n" \
    "    -b, --binary               convert to blow5\n" \
    "    -c, --compress             convert to compressed blow5\n"   \
    "    -o, --output=[FILE]        output converted contents to FILE -- stdout\n" \

static double init_realtime = 0;

/* core data structure that has information that are global to all the threads */
typedef struct {
    int32_t num_thread;
    enum slow5_fmt format_out;
    enum press_method press_method;
    std::string output_dir;
} global_thread;

/* data structure for a batch of reads*/
typedef struct {
    int64_t n_batch;    // number of records in this batch
    int64_t n_err;      // number of errors in this batch
    std::vector<std::string> slow5_files;
} data_thread;

/* argument wrapper for the multithreaded framework used for data processing */
typedef struct {
    global_thread * core;
    data_thread * db;
    int32_t starti;
    int32_t endi;
    int32_t thread_index;
} pthread_arg;

void merge_slow5(pthread_arg* pthreadArg) {
    fprintf(stderr, "thread index = %d\n", pthreadArg->thread_index);
    std::string out = pthreadArg->core->output_dir;
    out += "/" + std::to_string(pthreadArg->thread_index) + ".slow5";
    const char* output_path = out.c_str();

    FILE* slow5_file_pointer = stdout;
    if(output_path){
        slow5_file_pointer = NULL;
        slow5_file_pointer = fopen(output_path, "w");
        if (!slow5_file_pointer) {
            ERROR("Output file %s could not be opened - %s.", output_path, strerror(errno));
            return;
        }
    }
    slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, output_path, pthreadArg->core->format_out);
    slow5_hdr_initialize(slow5File->header, 1);
    slow5File->header->num_read_groups = 0;
    std::vector<std::vector<size_t>> list;

    for(size_t i=pthreadArg->starti; i<pthreadArg->endi; i++) { //iterate over slow5files
        slow5_file_t* slow5File_i = slow5_open(pthreadArg->db->slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("cannot open %s. skipping...\n",pthreadArg->db->slow5_files[i].c_str());
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
                    list[i-pthreadArg->starti][j] = k; //assumption0: run_ids are similar. Hence, the rest of the header attribute values of jth read_group are same as kth read_group's.
                    break;
                }
            }
            if(flag_run_id_found == 0){ // time to add a new read_group
                khash_t(s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg); //assumption0
                if(new_read_group != read_group_count){ //sanity check
                    WARNING("New read group number is not equal to number of groups; something's wrong\n%s", "");
                }
                list[i-pthreadArg->starti][j] = new_read_group;
            }
        }
        slow5_close(slow5File_i);
    }

    if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, pthreadArg->core->format_out, pthreadArg->core->press_method) == -1){ //now write the header to the slow5File
        ERROR("Could not write the header to %s\n","merge.slow5");
        exit(EXIT_FAILURE);
    }

    for(size_t i=pthreadArg->starti; i<pthreadArg->endi; i++) { //iterate over slow5files
        slow5_file_t *slow5File_i = slow5_open(pthreadArg->db->slow5_files[i].c_str(), "r");
        if (!slow5File_i) {
            ERROR("cannot open %s. skipping...\n", pthreadArg->db->slow5_files[i].c_str());
            continue;
        }
        struct slow5_rec *read = NULL;
        int ret;
        struct press *press_ptr = press_init(pthreadArg->core->press_method);
        while ((ret = slow5_get_next(&read, slow5File_i)) == 0) {
            read->read_group = list[i-pthreadArg->starti][read->read_group]; //write records of the ith slow5file with the updated read_group value
            if (slow5_rec_fwrite(slow5File->fp, read, slow5File->header->aux_meta, pthreadArg->core->format_out, press_ptr) == -1) {
                slow5_rec_free(read);
                return;
            }
        }
        press_free(press_ptr);
        slow5_rec_free(read);
        slow5_close(slow5File_i);
    }

    if(pthreadArg->core->format_out == FORMAT_BINARY){
        slow5_eof_fwrite(slow5File->fp);
    }
    slow5_close(slow5File);

    return;
}

void* pthread_single_merge(void* voidargs) {
    int32_t i;
    pthread_arg* args = (pthread_arg*)voidargs;
    data_thread * db = args->db;
    global_thread * core = args->core;
    merge_slow5(args);

    //fprintf(stderr,"Thread %d done\n",(myargs->position)/THREADS);
    pthread_exit(0);
}

void pthread_data(global_thread * core, data_thread * db){
    //create threads
    pthread_t tids[core->num_thread];
    pthread_arg pt_args[core->num_thread];
    int32_t t, ret;
    int32_t i = 0;
    int32_t num_thread = core->num_thread;
    int32_t step = (db->n_batch + num_thread - 1) / num_thread;
    //todo : check for higher num of threads than the data
    //current works but many threads are created despite

    //set the data structures
    for (t = 0; t < num_thread; t++) {
        pt_args[t].core = core;
        pt_args[t].db = db;
        pt_args[t].starti = i;
        i += step;
        if (i > db->n_batch) {
            pt_args[t].endi = db->n_batch;
        } else {
            pt_args[t].endi = i;
        }
        pt_args[t].thread_index=t;
    }

    //create threads
    for(t = 0; t < core->num_thread; t++){
        ret = pthread_create(&tids[t], NULL, pthread_single_merge,
                             (void*)(&pt_args[t]));
        NEG_CHK(ret);
    }

    //pthread joining
    for (t = 0; t < core->num_thread; t++) {
        int ret = pthread_join(tids[t], NULL);
        NEG_CHK(ret);
    }
}

/* process all reads in the given batch db */
void work_data(global_thread* core, data_thread* db){

    if (core->num_thread == 1) {
        int32_t i=0;
        pthread_arg pt_args;
        pt_args.core = core;
        pt_args.thread_index = 0;
        pt_args.db = db;
        pt_args.starti = 0;
        pt_args.endi = db->slow5_files.size();
        merge_slow5(&pt_args);
    }

    else {
        fprintf(stderr, "multi threaded\n");
        pthread_data(core,db);
    }
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
            {"threads", required_argument, NULL, '@' }, //1
            {"binary", no_argument, NULL, 'b'},    //2
            {"compress", no_argument, NULL, 'c'},  //3
            {"output", required_argument, NULL, 'o'}, //4
            {NULL, 0, NULL, 0 }
    };

    // Default options
    FILE *f_out = stdout;
    // Default options
    enum slow5_fmt format_out = FORMAT_ASCII;
    enum press_method pressMethod = COMPRESS_NONE;

    // Input arguments
    char *arg_fname_out = NULL;
    char *arg_num_threads = NULL;

    int32_t num_threads = 1;


    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "bch@:o:", long_opts, &longindex)) != -1) {
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
            case '@':
                arg_num_threads = optarg;
                break;
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

    // Parse num threads argument
    if (arg_num_threads != NULL) {
        char *endptr;
        long ret = strtol(arg_num_threads, &endptr, 10);

        if (*endptr == '\0') {
            num_threads = ret;
        } else {
            MESSAGE(stderr, "invalid number of threads -- '%s'", arg_num_threads);
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

    std::string output_file = std::string(arg_fname_out);

    std::string output_dir = output_file.substr(0,output_file.find_last_of('/')) + "/temp";
    fprintf(stderr, "output_file=%s output_dir=%s\n",output_file.c_str(),output_dir.c_str());

    int check_dir = mkdir(output_dir.c_str(),0777);
    // check if directory is created or not
    if (!check_dir) {
        printf("Temporary directory created\n");
    }
    else {
        printf("Unable to create a temporary directory\n");
        exit(1);
    }

    double realtime0 = slow5_realtime();
    reads_count readsCount;
    std::vector<std::string> slow5_files;
    for (int i = optind; i < argc; ++i) {
        find_all_5(argv[i], slow5_files, ASCII_EXTENSION);
    }
    fprintf(stderr, "[%s] %ld slow5 files found - took %.3fs\n", __func__, slow5_files.size(), slow5_realtime() - realtime0);
    while(num_threads) {
        int check_dir = mkdir((output_dir+"/"+std::to_string(num_threads)).c_str(),0777);
        // check if directory is created or not
        if (!check_dir) {
            printf("Temporary directory %s created\n", (output_dir+"/"+std::to_string(num_threads)).c_str());
        }
        else {
            printf("Unable to create a temporary directory\n");
            exit(1);
        }
        // Setup multithreading structures
        global_thread core;
        core.num_thread = num_threads;
        core.format_out = format_out;
        core.press_method = pressMethod;
        core.output_dir = output_dir+"/"+std::to_string(num_threads);

        data_thread db = {0};
        db.slow5_files = slow5_files;
        db.n_batch = slow5_files.size();

        // Fetch records for read ids in the batch
        work_data(&core, &db);
        slow5_files.clear();
        find_all_5(output_dir+"/"+std::to_string(num_threads), slow5_files, ASCII_EXTENSION);
        fprintf(stderr, "[%s] %ld slow5 files found - took %.3fs\n", __func__, slow5_files.size(), slow5_realtime() - realtime0);
        num_threads = num_threads/2;
    }

    // rename the merged file with user given output file name
    if (rename((output_dir+"/1/0.slow5").c_str(), arg_fname_out) == 0)
    {
        printf("File renamed successfully.\n");
    }
    else
    {
        printf("Unable to rename files. Please check files exist and you have permissions to modify files.\n");
    }

    if(remove(output_dir.c_str()) == 0){ //todo check
        printf("Temporary directory deleted\n");
    }
    else {
        printf("Unable to delete temporary directory\n");
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

