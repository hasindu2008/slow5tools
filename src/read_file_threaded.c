/**
 * @file read_file_threaded.c
 * @brief benchmark slow5 records reading implementation
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au)
 * @date 27/02/2021
 */
#include <getopt.h>

#include <string>
#include <vector>
#include <queue>
#include <pthread.h>

#include "error.h"
#include "cmd.h"
#include <slow5/slow5.h>
#include "read_fast5.h"
#include "slow5_extra.h"
#include "misc.h"
#include "thread.h"

#define USAGE_MSG "Usage: %s [OPTIONS] [SLOW5_FILE/DIR] ...\n"
#define HELP_LARGE_MSG \
    "Read SLOW5/BLOW5 files using threads\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    HELP_MSG_OUTPUT_FORMAT \
    HELP_MSG_THREADS \
    HELP_MSG_BATCH \
    HELP_MSG_HELP \

extern int slow5tools_verbosity_level;

void create_read_data(core_t *core, db_t *db, int32_t i) {
    struct slow5_rec *read = NULL;
    if (slow5_rec_depress_parse(&db->mem_records[i], &db->mem_bytes[i], NULL, &read, db->slow5_file_pointers[i]) != 0) {
        exit(EXIT_FAILURE);
    } else {
        free(db->mem_records[i]);
    }
    slow5_rec_free(read);
}

int read_file_threaded_main(int argc, char **argv, struct program_meta *meta){

    // Debug: print arguments
    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h'},  //0
            {"threads", required_argument, NULL, 't' }, //1
            {"batchsize", required_argument, NULL, 'K'}, //7
            {NULL, 0, NULL, 0 }
    };

    opt_t user_opts;
    init_opt(&user_opts);

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "h:t:K:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'h':
                DEBUG("displaying large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 't':
                user_opts.arg_num_threads = optarg;
                break;
            case 'K':
                user_opts.arg_batch = optarg;
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }
    if(parse_num_threads(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_batch_size(&user_opts,argc,argv) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        ERROR("Not enough arguments. Enter one or more slow5/blow5 files or directories as arguments.%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    //measure file listing time
    double realtime0 = slow5_realtime();
    std::vector<std::string> slow5_files;
    for (int i = optind; i < argc; ++i) {
        list_all_items(argv[i], slow5_files, 0, NULL);
    }
    VERBOSE("%ld files found - took %.3fs\n", slow5_files.size(), slow5_realtime() - realtime0);

    if(slow5_files.size()==0){
        ERROR("No slow5/blow5 files found. Exiting.%s","");
        return EXIT_FAILURE;
    }

    double time_get_to_mem_loop = 0;
    double time_get_next_mem = 0;
    double time_thread_execution = 0;
    double time_malloc = 0;
    int flag_end_of_records = 0;

    int64_t batch_size = user_opts.read_id_batch_capacity;
    size_t slow5_file_index = 0;
    std::queue<struct slow5_file*> open_files_pointers;

    struct slow5_file *from = slow5_open(slow5_files[slow5_file_index].c_str(), "r");
    if (from == NULL) {
        ERROR("File '%s' could not be opened - %s.", slow5_files[slow5_file_index].c_str(), strerror(errno));
        return EXIT_FAILURE;
    }
    open_files_pointers.push(from);
    size_t open_file_from = slow5_file_index;

    while(1) {
        double realtime = slow5_realtime();
        db_t db = { 0 };
        db.mem_records = (char **) malloc(batch_size * sizeof(char*));
        db.mem_bytes = (size_t *) malloc(batch_size * sizeof(size_t));
        db.slow5_file_pointers = (slow5_file_t **) malloc(batch_size * sizeof(slow5_file_t*));
        time_malloc += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        int64_t record_count = 0;
        size_t bytes;
        char *mem;
        while (record_count < batch_size) {
            double realtime2 = slow5_realtime();
            mem = (char *) slow5_get_next_mem(&bytes, from);
            time_get_next_mem += slow5_realtime() - realtime2;

            if (!mem) {
                if (slow5_errno != SLOW5_ERR_EOF) {
                    return EXIT_FAILURE;
                } else { //EOF file reached
                    slow5_file_index++;
                    if(slow5_file_index == slow5_files.size()){
                        flag_end_of_records = 1;
                        break;
                    }else{
                        from = slow5_open(slow5_files[slow5_file_index].c_str(), "r");
                        if (from == NULL) {
                            ERROR("File '%s' could not be opened - %s.", slow5_files[slow5_file_index].c_str(), strerror(errno));
                            return EXIT_FAILURE;
                        }
                        open_files_pointers.push(from);
                    }
                    continue;
                }
            } else {
                db.mem_records[record_count] = mem;
                db.mem_bytes[record_count] = bytes;
                db.slow5_file_pointers[record_count] = from;
                record_count++;
            }
        }

        time_get_to_mem_loop += slow5_realtime() - realtime;
        realtime = slow5_realtime();
        // Setup multithreading structures
        core_t core;
        core.num_thread = user_opts.num_threads;
        db.n_batch = record_count;
        work_db(&core,&db,create_read_data);
        time_thread_execution += slow5_realtime() - realtime;

        // Free everything
        free(db.mem_bytes);
        free(db.mem_records);
        free(db.slow5_file_pointers);

        for(size_t j=open_file_from; j<slow5_file_index; j++){
            if (slow5_close(open_files_pointers.front()) == EOF) { //close file
                ERROR("File '%s' failed on closing - %s.", slow5_files[j].c_str(), strerror(errno));
                return EXIT_FAILURE;
            }
            open_files_pointers.pop();
        }
        open_file_from = slow5_file_index;
        if(flag_end_of_records){
            break;
        }
        
    }
    VERBOSE("time_malloc\t%.3fs", time_malloc);
    VERBOSE("time_get_next_mem\t%.3fs", time_get_next_mem);
    VERBOSE("time_get_to_mem_loop\t%.3fs", time_get_to_mem_loop);
    VERBOSE("time_thread_execution\t%.3fs", time_thread_execution);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
