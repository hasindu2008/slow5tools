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
#include "thread.h"

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
    HELP_MSG_THREADS \
    HELP_MSG_BATCH \
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

int split_func(std::vector<std::string> slow5_files_input, opt_t user_opts, meta_split_method  meta_split_method_object);

//READ SPLIT
int read_split_func(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                    slow5_press_method_t press_out, meta_split_method meta_split_method_object,
                    int flag_single_threaded_execution);

int single_threaded_read_split_execution(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                                     slow5_press_method_t press_out, int read_limit,
                                     size_t *record_count_ptr, int* flag_EOF_ptr, slow5_file_t * input_slow5_file_i, slow5_file_t * slow5_file_out);

int multi_threaded_read_split_execution(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                                    slow5_press_method_t press_out, int read_limit,
                                    size_t *record_count_ptr, int* flag_EOF_ptr, slow5_file_t * input_slow5_file_i, slow5_file_t * slow5_file_out);

void read_split_thread_func(core_t *core, db_t *db, int32_t i) {
    //
    struct slow5_rec *read = NULL;
    if (slow5_rec_depress_parse(&db->mem_records[i], &db->mem_bytes[i], NULL, &read, core->fp) != 0) {
        exit(EXIT_FAILURE);
    } else {
        free(db->mem_records[i]);
    }
    struct slow5_press *press_ptr = slow5_press_init(core->press_method);
    if(!press_ptr){
        ERROR("Could not initialize the slow5 compression method%s","");
        exit(EXIT_FAILURE);
    }
    size_t len;
    if ((db->read_record[i].buffer = slow5_rec_to_mem(read, core->fp->header->aux_meta, core->format_out, press_ptr, &len)) == NULL) {
        slow5_press_free(press_ptr);
        slow5_rec_free(read);
        exit(EXIT_FAILURE);
    }
    slow5_press_free(press_ptr);
    db->read_record[i].len = len;
    slow5_rec_free(read);
}

//FILE SPLIT
int file_split_func(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                    slow5_press_method_t press_out, meta_split_method meta_split_method_object,
                    int flag_single_threaded_execution);


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
            {"threads",     required_argument, NULL, 't'},   //5
            {"lossless",    required_argument, NULL, 'l'}, //6
            {"groups",      no_argument, NULL, 'g'}, //7
            {"files",       required_argument, NULL, 'f'}, //8
            {"reads",       required_argument, NULL, 'r'}, //9
            {"batchsize",   required_argument, NULL, 'K'}, //8
            {NULL, 0, NULL, 0 }
    };

    meta_split_method meta_split_method_object;
    meta_split_method_object.n = 0;
    meta_split_method_object.splitMethod = READS_SPLIT;

    opt_t user_opts;
    init_opt(&user_opts);

    int opt;
    int longindex = 0;
    // Parse options
    while ((opt = getopt_long(argc, argv, "hb:c:s:gl:f:r:d:t:K:", long_opts, &longindex)) != -1) {
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
                meta_split_method_object.splitMethod = FILE_SPLIT;
                meta_split_method_object.n = atoi(optarg);
                break;
            case 'r':
                meta_split_method_object.splitMethod = READS_SPLIT;
                meta_split_method_object.n = atoi(optarg);
                break;
            case 'g':
                meta_split_method_object.splitMethod = GROUP_SPLIT;
                break;
            case 'l':
                user_opts.arg_lossless = optarg;
                break;
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
    if(meta_split_method_object.splitMethod == READS_SPLIT && meta_split_method_object.n == 0){
        ERROR("Default splitting method - reads split is used. Specify the number of reads to include in a slow5 file%s","");
        return EXIT_FAILURE;
    }
    if(meta_split_method_object.splitMethod == FILE_SPLIT && meta_split_method_object.n == 0){
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
    std::vector<std::string> slow5_files_input;

    if(meta_split_method_object.splitMethod == READS_SPLIT){
        VERBOSE("An input slow5 file will be split such that each output file has %lu reads", meta_split_method_object.n);
    }else if(meta_split_method_object.splitMethod == FILE_SPLIT){
        VERBOSE("An input slow5 file will be split into %lu output files", meta_split_method_object.n);
    } else{
        VERBOSE("An input multi read group slow5 files will be split into single read group slow5 files %s","");
    }

    //measure file listing time
    double realtime0 = slow5_realtime();
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], slow5_files_input, 0, NULL);
    }
    VERBOSE("%ld slow5 files found - took %.3fs", slow5_files_input.size(), slow5_realtime() - realtime0);
    if(slow5_files_input.size() == 0){
        ERROR("No slow5/blow5 files found. Exiting...%s","");
        return EXIT_FAILURE;
    }
    //measure slow5 splitting time

    //split threading start
    int ret_split_func = split_func(slow5_files_input, user_opts, meta_split_method_object);

    //split threading end

    VERBOSE("Splitting %ld s/blow5 files took %.3fs", slow5_files_input.size(), slow5_realtime() - init_realtime);

    return EXIT_SUCCESS;
}

int split_func(std::vector<std::string> slow5_files_input, opt_t user_opts, meta_split_method meta_split_method_object) {
    std::string extension = ".blow5";
    if(user_opts.fmt_out == SLOW5_FORMAT_ASCII){
        extension = ".slow5";
    }
    slow5_press_method_t press_out = {user_opts.record_press_out,user_opts.signal_press_out};

    for(size_t i=0; i < slow5_files_input.size(); i++) {
        slow5_file_t *input_slow5_file_i = slow5_open(slow5_files_input[i].c_str(), "r");
        if (!input_slow5_file_i) {
            ERROR("Cannot open %s. Skipping.\n", slow5_files_input[i].c_str());
            continue;
        }
        uint32_t read_group_count_i = input_slow5_file_i->header->num_read_groups;

        if (read_group_count_i == 1 && meta_split_method_object.splitMethod == GROUP_SPLIT) {
            ERROR("The file %s already has a single read group", slow5_files_input[i].c_str());
            continue;
        }
        if (read_group_count_i > 1 && meta_split_method_object.splitMethod != GROUP_SPLIT) {
            ERROR("The file %s contains multiple read groups. You must first separate the read groups using -g. See https://slow5.page.link/faq for more info.",
                  slow5_files_input[i].c_str());
            continue;
        }
        int flag_single_threaded_execution = 0;
        if(user_opts.fmt_out==SLOW5_FORMAT_BINARY && input_slow5_file_i->format==user_opts.fmt_out && input_slow5_file_i->compress->record_press->method==user_opts.record_press_out && input_slow5_file_i->compress->signal_press->method==user_opts.signal_press_out){
            flag_single_threaded_execution = 1;
        }
        if(user_opts.fmt_out==SLOW5_FORMAT_ASCII && input_slow5_file_i->format==user_opts.fmt_out){
            flag_single_threaded_execution = 1;
        }
        slow5_close(input_slow5_file_i); //todo-implement a method to fseek() to the first record of the slow5File_i
        if (meta_split_method_object.splitMethod == READS_SPLIT) {
            int ret_read_split_func = read_split_func(slow5_files_input[i], user_opts, extension, press_out, meta_split_method_object, flag_single_threaded_execution);
        }
        if (meta_split_method_object.splitMethod == FILE_SPLIT) {
            int ret_read_split_func = file_split_func(slow5_files_input[i], user_opts, extension, press_out, meta_split_method_object, flag_single_threaded_execution);
        }
    }

    return 0;
}

int read_split_func(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                    slow5_press_method_t press_out, meta_split_method meta_split_method_object,
                    int flag_single_threaded_execution) {
    int flag_EOF = 0;
    slow5_file_t * input_slow5_file_i = slow5_open(input_slow5_path.c_str(), "r");
    if (!input_slow5_file_i) {
        ERROR("Cannot open %s. Skipping.\n", input_slow5_path.c_str());
        return -1;
    }
    size_t file_count = 0;
    while (1) {
        int last_slash = input_slow5_path.find_last_of('/');
        if (last_slash == -1) {
            last_slash = 0;
        }
        std::string substr_path_slow5_out =
                input_slow5_path.substr(last_slash, input_slow5_path.length() - last_slash - extension.length()) +
                "_" + std::to_string(file_count) + extension;
        std::string slow5_path_out = std::string(user_opts.arg_dir_out) + "/";
        slow5_path_out += substr_path_slow5_out;

        FILE *slow5_file_pointer_out = NULL;
        slow5_file_pointer_out = fopen(slow5_path_out.c_str(), "w");
        if (!slow5_file_pointer_out) {
            ERROR("Output file %s could not be opened - %s.", slow5_path_out.c_str(), strerror(errno));
            continue;
        }
        slow5_file_t *slow5_file_out = slow5_init_empty(slow5_file_pointer_out, slow5_path_out.c_str(), user_opts.fmt_out);
        int ret0 = slow5_hdr_initialize(slow5_file_out->header, user_opts.flag_lossy);
        if (ret0 < 0) {
            exit(EXIT_FAILURE);
        }
        slow5_file_out->header->num_read_groups = 0;
        if (user_opts.flag_lossy == 0) {
            slow5_aux_meta_t *aux_ptr = input_slow5_file_i->header->aux_meta;
            uint32_t num_aux_attrs = aux_ptr->num;
            int aux_add_fail = 0;
            for (uint32_t r = 0; r < num_aux_attrs; r++) {
                if (aux_ptr->types[r] == SLOW5_ENUM || aux_ptr->types[r] == SLOW5_ENUM_ARRAY) {
                    uint8_t n;
                    const char **enum_labels = (const char **) slow5_get_aux_enum_labels(input_slow5_file_i->header,
                                                                                         aux_ptr->attrs[r], &n);
                    if (!enum_labels) {
                        aux_add_fail = 1;
                    }
                    if (slow5_aux_meta_add_enum(slow5_file_out->header->aux_meta, aux_ptr->attrs[r],
                                                aux_ptr->types[r], enum_labels, n)) {
                        aux_add_fail = 1;
                    }
                } else {
                    if (slow5_aux_meta_add(slow5_file_out->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r])) {
                        aux_add_fail = 1;
                    }
                }
                if (aux_add_fail) {
                    ERROR("Could not initialize the record attribute '%s'", aux_ptr->attrs[r]);
                    exit(EXIT_FAILURE);
                }
            }
        }
        khash_t(slow5_s2s) *rg = slow5_hdr_get_data(0, input_slow5_file_i->header); // extract 0th read_group related data from ith slow5file
        if (slow5_hdr_add_rg_data(slow5_file_out->header, rg) < 0) {
            ERROR("Could not add read group to %s\n", slow5_path_out.c_str());
            exit(EXIT_FAILURE);
        }

        if (slow5_hdr_fwrite(slow5_file_out->fp, slow5_file_out->header, user_opts.fmt_out, press_out) == -1) { //now write the header to the slow5File
            ERROR("Could not write the header to %s\n", slow5_path_out.c_str());
            exit(EXIT_FAILURE);
        }
        size_t record_count = 0;
        if(flag_single_threaded_execution){
            int ret_single_threaded_read_split_execution = single_threaded_read_split_execution(input_slow5_path,
                                                                                                user_opts, extension,
                                                                                                press_out,
                                                                                                meta_split_method_object.n,
                                                                                                &record_count, &flag_EOF, input_slow5_file_i, slow5_file_out);
        }else{
            int ret_multi_threaded_read_split_execution = multi_threaded_read_split_execution(input_slow5_path,
                                                                                              user_opts, extension,
                                                                                              press_out,
                                                                                              meta_split_method_object.n,
                                                                                              &record_count, &flag_EOF, input_slow5_file_i, slow5_file_out);
        }
        if (user_opts.fmt_out == SLOW5_FORMAT_BINARY) {
            slow5_eof_fwrite(slow5_file_out->fp);
        }
        slow5_close(slow5_file_out);

        if (flag_EOF) {
            if (record_count == 0) {
                int del = remove(slow5_path_out.c_str());
                if (del) {
                    WARNING("Deleting additional file %s failed\n", slow5_path_out.c_str());
                    perror("");
                }
            }
            break;
        }
        file_count++;
    }
    slow5_close(input_slow5_file_i);
    return 0;
}

int multi_threaded_read_split_execution(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                                    slow5_press_method_t press_out, int read_limit,
                                    size_t *record_count_ptr, int* flag_EOF_ptr, slow5_file_t * input_slow5_file_i, slow5_file_t * slow5_file_out) {

    size_t record_count = *record_count_ptr;
    int flag_EOF = *flag_EOF_ptr;
    while(record_count<read_limit){
        int64_t batch_size = (user_opts.read_id_batch_capacity<read_limit)?user_opts.read_id_batch_capacity:read_limit;
        db_t db = {0};
        db.mem_records = (char **) malloc(batch_size * sizeof(char *));
        db.mem_bytes = (size_t *) malloc(batch_size * sizeof(size_t));
        MALLOC_CHK(db.mem_records);
        MALLOC_CHK(db.mem_bytes);
        int64_t record_count_local = 0;
        size_t bytes;
        char *mem;
        while (record_count_local < batch_size) {
            if (!(mem = (char *) slow5_get_next_mem(&bytes, input_slow5_file_i))) {
                if (slow5_errno != SLOW5_ERR_EOF) {
                    return EXIT_FAILURE;
                } else { //EOF file reached
                    flag_EOF = 1;
                    break;
                }
            } else {
                db.mem_records[record_count_local] = mem;
                db.mem_bytes[record_count_local] = bytes;
                record_count_local++;
                record_count++;
            }
        }

        // Setup multithreading structures
        core_t core;
        core.num_thread = user_opts.num_threads;
        core.fp = input_slow5_file_i;
        core.aux_meta = slow5_file_out->header->aux_meta;
        core.format_out = user_opts.fmt_out;
        core.press_method = press_out;
        core.lossy = user_opts.flag_lossy;

        db.n_batch = record_count_local;
        db.read_record = (raw_record_t *) malloc(record_count_local * sizeof *db.read_record);
        MALLOC_CHK(db.read_record);
        work_db(&core, &db, read_split_thread_func);

        for (int64_t i = 0; i < record_count_local; i++) {
            fwrite(db.read_record[i].buffer, 1, db.read_record[i].len, slow5_file_out->fp);
            free(db.read_record[i].buffer);
        }
        // Free everything
        free(db.mem_bytes);
        free(db.mem_records);
        free(db.read_record);

        if(flag_EOF){
            break;
        }
    }
    *flag_EOF_ptr = flag_EOF;
    *record_count_ptr = record_count;
    return 0;
}

int single_threaded_read_split_execution(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                                     slow5_press_method_t press_out, int read_limit,
                                     size_t *record_count_ptr, int* flag_EOF_ptr, slow5_file_t * input_slow5_file_i, slow5_file_t * slow5_file_out) {

    size_t record_count = *record_count_ptr;
    int flag_EOF = *flag_EOF_ptr;
    size_t bytes;
    char *buffer;
    while (record_count < read_limit) {
        if (!(buffer = (char *) slow5_get_next_mem(&bytes, input_slow5_file_i))) {
            if (slow5_errno != SLOW5_ERR_EOF) {
                ERROR("Could not read file %s", input_slow5_path.c_str());
                return EXIT_FAILURE;
            } else { //EOF file reached
                flag_EOF = 1;
                break;
            }
        }
        fwrite(buffer,1,bytes,slow5_file_out->fp);
        if(user_opts.fmt_out == SLOW5_FORMAT_ASCII){
            fwrite("\n",1,1,slow5_file_out->fp);
        }
        record_count++;
    }
    *flag_EOF_ptr = flag_EOF;
    *record_count_ptr = record_count;
    return 0;
}

int file_split_func(std::basic_string<char> &input_slow5_path, opt_t user_opts, std::string extension,
                    slow5_press_method_t press_out, meta_split_method meta_split_method_object,
                    int flag_single_threaded_execution){
    int flag_EOF = 0;
    slow5_file_t * input_slow5_file_i = slow5_open(input_slow5_path.c_str(), "r");
    if (!input_slow5_file_i) {
        ERROR("Cannot open %s. Skipping.\n", input_slow5_path.c_str());
        return -1;
    }

    int64_t number_of_records = 0;
    size_t bytes;
    char *mem;
    double time_get_to_mem = slow5_realtime();
    while ((mem = (char *) slow5_get_next_mem(&bytes, input_slow5_file_i))) {
        free(mem);
        number_of_records++;
    }

    slow5_close(input_slow5_file_i); //todo-implement a method to fseek() to the first record of the slow5File_i

    input_slow5_file_i = slow5_open(input_slow5_path.c_str(), "r");
    if (!input_slow5_file_i) {
        ERROR("Cannot open %s. Skipping.\n", input_slow5_path.c_str());
        return -1;
    }

    int limit = number_of_records/meta_split_method_object.n;
    int rem = number_of_records%meta_split_method_object.n;
    size_t file_count = 0;

    while(number_of_records > 0) {
        int number_of_records_per_file = (rem > 0) ? 1 : 0;
        number_of_records_per_file += limit;
        // fprintf(stderr, "file_count = %d, number_of_records_per_file = %d, number_of_records = %d\n", file_count, number_of_records_per_file, number_of_records);
        number_of_records -= number_of_records_per_file;
        rem--;
        int last_slash = input_slow5_path.find_last_of('/');
        if (last_slash == -1) {
            last_slash = 0;
        }
        std::string substr_path_slow5_out =
                input_slow5_path.substr(last_slash, input_slow5_path.length() - last_slash - extension.length()) +
                "_" + std::to_string(file_count) + extension;
        std::string slow5_path_out = std::string(user_opts.arg_dir_out) + "/";
        slow5_path_out += substr_path_slow5_out;

        FILE *slow5_file_pointer_out = NULL;
        slow5_file_pointer_out = fopen(slow5_path_out.c_str(), "w");
        if (!slow5_file_pointer_out) {
            ERROR("Output file %s could not be opened - %s.", slow5_path_out.c_str(), strerror(errno));
            continue;
        }
        slow5_file_t *slow5_file_out = slow5_init_empty(slow5_file_pointer_out, slow5_path_out.c_str(), user_opts.fmt_out);
        int ret0 = slow5_hdr_initialize(slow5_file_out->header, user_opts.flag_lossy);
        if (ret0 < 0) {
            exit(EXIT_FAILURE);
        }
        slow5_file_out->header->num_read_groups = 0;
        if (user_opts.flag_lossy == 0) {
            slow5_aux_meta_t *aux_ptr = input_slow5_file_i->header->aux_meta;
            uint32_t num_aux_attrs = aux_ptr->num;
            int aux_add_fail = 0;
            for (uint32_t r = 0; r < num_aux_attrs; r++) {
                if (aux_ptr->types[r] == SLOW5_ENUM || aux_ptr->types[r] == SLOW5_ENUM_ARRAY) {
                    uint8_t n;
                    const char **enum_labels = (const char **) slow5_get_aux_enum_labels(input_slow5_file_i->header,
                                                                                         aux_ptr->attrs[r], &n);
                    if (!enum_labels) {
                        aux_add_fail = 1;
                    }
                    if (slow5_aux_meta_add_enum(slow5_file_out->header->aux_meta, aux_ptr->attrs[r],
                                                aux_ptr->types[r], enum_labels, n)) {
                        aux_add_fail = 1;
                    }
                } else {
                    if (slow5_aux_meta_add(slow5_file_out->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r])) {
                        aux_add_fail = 1;
                    }
                }
                if (aux_add_fail) {
                    ERROR("Could not initialize the record attribute '%s'", aux_ptr->attrs[r]);
                    exit(EXIT_FAILURE);
                }
            }
        }
        khash_t(slow5_s2s) *rg = slow5_hdr_get_data(0, input_slow5_file_i->header); // extract 0th read_group related data from ith slow5file
        if (slow5_hdr_add_rg_data(slow5_file_out->header, rg) < 0) {
            ERROR("Could not add read group to %s\n", slow5_path_out.c_str());
            exit(EXIT_FAILURE);
        }

        if (slow5_hdr_fwrite(slow5_file_out->fp, slow5_file_out->header, user_opts.fmt_out, press_out) == -1) { //now write the header to the slow5File
            ERROR("Could not write the header to %s\n", slow5_path_out.c_str());
            exit(EXIT_FAILURE);
        }
        size_t record_count = 0;
        if(flag_single_threaded_execution){
            int ret_single_threaded_read_split_execution = single_threaded_read_split_execution(input_slow5_path,
                                                                                                user_opts, extension,
                                                                                                press_out,
                                                                                                number_of_records_per_file,
                                                                                                &record_count, &flag_EOF, input_slow5_file_i, slow5_file_out);
        }else{
            int ret_multi_threaded_read_split_execution = multi_threaded_read_split_execution(input_slow5_path,
                                                                                              user_opts, extension,
                                                                                              press_out,
                                                                                              number_of_records_per_file,
                                                                                              &record_count, &flag_EOF, input_slow5_file_i, slow5_file_out);
        }
        if (user_opts.fmt_out == SLOW5_FORMAT_BINARY) {
            slow5_eof_fwrite(slow5_file_out->fp);
        }
        slow5_close(slow5_file_out);

        if (flag_EOF) {
            if (record_count == 0) {
                int del = remove(slow5_path_out.c_str());
                if (del) {
                    WARNING("Deleting additional file %s failed\n", slow5_path_out.c_str());
                    perror("");
                }
            }
            break;
        }
        file_count++;
    }
    slow5_close(input_slow5_file_i);
    return 0;
}