/**
 * @file merge.c
 * @brief merge two or more SLOW5 files
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au)
 * @date 27/02/2021
 */
#include <getopt.h>
#include <sys/wait.h>

#include <string>
#include <vector>
#include <pthread.h>

#include "error.h"
#include "cmd.h"
#include <slow5/slow5.h>
#include <map>
#include "read_fast5.h"
#include "slow5_extra.h"
#include "misc.h"
#include "thread.h"

#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_LARGE_MSG \
    "Merge multiple SLOW5/BLOW5 files to a single file\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    --to=[FORMAT]                      specify output file format\n" \
    "    -c, --compress=[REC_METHOD]        specify record compression method -- zlib (only available for format blow5)\n" \
    "    -s, --sig-compress=[SIG_METHOD]    specify signal compression method -- none (only available for format blow5)\n" \
    "    -o, --output [FILE]                output contents to FILE [default: stdout]\n" \
    "    -l, --lossless [STR]               retain information in auxiliary fields during the conversion.[default: true].\n" \
    "    -t, --threads [INT]                number of threads [default: 4]\n"        \
    "    -K --batchsize                     the number of records on the memory at once. [default: 4096]\n" \
    "    -h, --help                         display this message and exit\n"                                               \
    HELP_FORMATS_METHODS

int compare_headers(slow5_hdr_t *output_header, slow5_hdr_t *input_header, int64_t output_g, int64_t input_g);

void parallel_reads_model(core_t *core, db_t *db, int32_t i) {
    //
    struct slow5_rec *read = NULL;
    if (slow5_rec_depress_parse(&db->mem_records[i], &db->mem_bytes[i], NULL, &read, core->fp) != 0) {
        exit(EXIT_FAILURE);
    } else {
        free(db->mem_records[i]);
    }
    read->read_group = db->list[core->slow5_file_index][read->read_group]; //write records of the ith slow5file with the updated read_group value

    struct slow5_press *press_ptr = slow5_press_init(core->press_method);
    size_t len;
    struct slow5_aux_meta *aux_meta = core->fp->header->aux_meta;
    if(core->lossy){
        aux_meta = NULL;
    }
    if ((db->read_record[i].buffer = slow5_rec_to_mem(read, aux_meta, core->format_out, press_ptr, &len)) == NULL) {
        slow5_press_free(press_ptr);
        slow5_rec_free(read);
        exit(EXIT_FAILURE);
    }
    slow5_press_free(press_ptr);
    db->read_record[i].len = len;
    slow5_rec_free(read);
}

int merge_main(int argc, char **argv, struct program_meta *meta){

    // Debug: print arguments
    if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("printing the arguments given%s","");
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
            {"threads", required_argument, NULL, 't' }, //1
            {"to", required_argument, NULL, 'b'},    //2
            {"compress", required_argument, NULL, 'c'},  //3
            {"sig-compress",    required_argument,  NULL, 's'}, //4
            { "lossless", required_argument, NULL, 'l'}, //5
            {"output", required_argument, NULL, 'o'}, //6
            {"batchsize", required_argument, NULL, 'K'}, //7

            {NULL, 0, NULL, 0 }
    };

    // Default options
    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum slow5_fmt extension_format = SLOW5_FORMAT_BINARY;
    enum slow5_press_method pressMethodRecord = SLOW5_COMPRESS_ZLIB;
    enum slow5_press_method pressMethodSignal = SLOW5_COMPRESS_NONE;
    int compression_set = 0;
    int format_out_set = 0;

    // Input arguments
    char *arg_fmt_out = NULL;
    char *arg_fname_out = NULL;
    char *arg_num_threads = NULL;
    char *arg_record_press_out = NULL;
    char *arg_signal_press_out = NULL;
    int lossy = 0;

    size_t num_threads = DEFAULT_NUM_THREADS;
    int64_t read_id_batch_capacity = READ_ID_BATCH_CAPACITY;

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "b:c:s:hl:t:o:K:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'h':
                if (meta->verbosity_level >= LOG_DEBUG) {
                    DEBUG("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 't':
                arg_num_threads = optarg;
                break;
            case 'b':
                format_out_set = 1;
                arg_fmt_out = optarg;
                break;
            case 'K':
                read_id_batch_capacity = atoi(optarg);
                if(read_id_batch_capacity < 0){
                    fprintf(stderr, "batchsize cannot be negative\n");
                    fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                    EXIT_MSG(EXIT_FAILURE, argv, meta);
                    return EXIT_FAILURE;
                }
                break;
            case 'c':
                compression_set = 1;
                arg_record_press_out = optarg;
                break;
            case 's':
                arg_signal_press_out = optarg;
                break;
            case 'l':
                if(strcmp(optarg,"true")==0){
                    lossy = 0;
                }else if(strcmp(optarg,"false")==0){
                    lossy = 1;
                }else{
                    ERROR("Incorrect argument%s", "");
                    return EXIT_FAILURE;
                }
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
    if (arg_fmt_out) {
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("parsing output format%s","");
        }
        format_out = parse_name_to_fmt(arg_fmt_out);
        // An error occured
        if (format_out == SLOW5_FORMAT_UNKNOWN) {
            ERROR("invalid output format -- '%s'", arg_fmt_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if(arg_fname_out){
        if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("parsing output file format%s","");
        }
        extension_format = parse_path_to_fmt(arg_fname_out);
        if (extension_format == SLOW5_FORMAT_UNKNOWN) {
            ERROR("cannot detect file format -- '%s'", arg_fname_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
        if(format_out_set==0){
            format_out = extension_format;
        }
    }
    if(arg_fname_out && format_out!=extension_format){
        ERROR("Output file extension does not match with the output format%s",".");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(compression_set == 0 && format_out == SLOW5_FORMAT_ASCII){
        pressMethodRecord = SLOW5_COMPRESS_NONE;
        pressMethodSignal = SLOW5_COMPRESS_NONE;
    }
    // compression option is only effective with -b blow5
    if(compression_set == 1 && format_out == SLOW5_FORMAT_ASCII){
        ERROR("%s","Compression option (-c/-s) is only available for SLOW5 binary format.");
        return EXIT_FAILURE;
    }
    if (arg_record_press_out != NULL) {
        pressMethodRecord = name_to_slow5_press_method(arg_record_press_out);
        if (pressMethodRecord == (enum slow5_press_method) -1) {
            ERROR("invalid record compression method -- '%s'", arg_record_press_out);
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }
    if (arg_signal_press_out != NULL) {
        pressMethodSignal = name_to_slow5_press_method(arg_signal_press_out);
        if (pressMethodSignal == (enum slow5_press_method) -1) {
            ERROR("invalid signal compression method -- '%s'", arg_signal_press_out);
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
            ERROR("invalid number of threads -- '%s'", arg_num_threads);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        ERROR("missing slow5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    //measure file listing time
    double realtime0 = slow5_realtime();
    std::vector<std::string> files;
    for (int i = optind; i < argc; ++i) {
        list_all_items(argv[i], files, 0, NULL);
    }
    VERBOSE("%ld files found - took %.3fs\n", files.size(), slow5_realtime() - realtime0);

    if(files.size()==0){
        ERROR("No slow5/blow5 files found for conversion. Exiting...%s","");
        return EXIT_FAILURE;
    }

    //determine new read group numbers
    //measure read_group number allocation time
    realtime0 = slow5_realtime();

    FILE* slow5_file_pointer = stdout;
    if(arg_fname_out){
        slow5_file_pointer = fopen(arg_fname_out, "wb");
        if (!slow5_file_pointer) {
            ERROR("Output file %s could not be opened - %s.", arg_fname_out, strerror(errno));
            return EXIT_FAILURE;
        }
    }else{
        std::string stdout_s = "stdout";
        arg_fname_out = &stdout_s[0];
    }

    std::map<std::string, enum slow5_aux_type> set_aux_attr_pairs;
    slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, arg_fname_out, format_out);
    int ret = slow5_hdr_initialize(slow5File->header, lossy);
    if(ret<0){
        return EXIT_FAILURE;
    }
    slow5File->header->num_read_groups = 0;
    std::vector<std::vector<size_t>> list;
    size_t index = 0;
    std::vector<std::string> slow5_files;
    size_t num_files = files.size();
    for(size_t i=0; i<num_files; i++) { //iterate over slow5files
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("input file\t%s", files[i].c_str());
        }
        slow5_file_t* slow5File_i = slow5_open(files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("[Skip file]: cannot open %s. skipping...\n",files[i].c_str());
            continue;
        }
        if(lossy==0 && slow5File_i->header->aux_meta == NULL){
            ERROR("[Skip file]: %s has no auxiliary fields. Specify -l false to merge files with no auxiliary fields.", files[i].c_str());
            slow5_close(slow5File_i);
            return EXIT_FAILURE;
        }
        if(lossy==0){
            slow5_aux_meta_t* aux_ptr = slow5File_i->header->aux_meta;
            uint32_t num_aux_attrs = aux_ptr->num;
            for(uint32_t r=0; r<num_aux_attrs; r++){
                set_aux_attr_pairs.insert({std::string(aux_ptr->attrs[r]),aux_ptr->types[r]});
            }
        }

        int64_t read_group_count_i = slow5File_i->header->num_read_groups; // number of read_groups in ith slow5file
        std::vector<size_t> read_group_tracker(read_group_count_i); //this array will store the new group_numbers of the ith slow5File, i.e., the new value of jth read_group_number
        list.push_back(read_group_tracker);

        for(int64_t j=0; j<read_group_count_i; j++){
            char* run_id_j = slow5_hdr_get("run_id", j, slow5File_i->header); // run_id of the jth read_group of the ith slow5file
            if(!run_id_j){
                ERROR("No run_id information found in %s.", files[i].c_str());
                return EXIT_FAILURE;
            }
            int64_t read_group_count = slow5File->header->num_read_groups; //since this might change during iterating; cannot know beforehand
            size_t flag_run_id_found = 0;
            for(int64_t k=0; k<read_group_count; k++){
                char* run_id_k = slow5_hdr_get("run_id", k, slow5File->header);
                if(!run_id_k){
                    ERROR("No run_id information found in %s.", files[i].c_str());
                    return EXIT_FAILURE;
                }
                if(strcmp(run_id_j,run_id_k) == 0){
                    flag_run_id_found = 1;
                    list[index][j] = k; //assumption0: if run_ids are similar the rest of the header attribute values of jth and kth read_groups are similar.
                    int ret_compare = compare_headers(slow5File->header, slow5File_i->header, k, j);
                    if(ret_compare == -1){
                        ERROR("In file %s, read_group %" PRIu64 "has a different number of header attributes than the previous files had", files[i].c_str(), j);
                        return EXIT_FAILURE;
                    }else if(ret_compare == -2){
                        ERROR("In file %s, read_group %" PRIu64 " has a different set of header attributes than the previous files had", files[i].c_str(), j);
                        return EXIT_FAILURE;
                    }else if(ret_compare == -3){
                        ERROR("In file %s, read_group %" PRIu64 " has different values for the header attributes than the previous files had", files[i].c_str(), j);
                        return EXIT_FAILURE;
                    }
                    DEBUG("ret_compare\t%d", ret_compare);
                    break;
                }
            }
            if(flag_run_id_found == 0){ // time to add a new read_group
                khash_t(slow5_s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg); //assumption0
                if(new_read_group != read_group_count){ //sanity check
                    ERROR("New read group number is not equal to number of groups; something's wrong\n%s", "");
                    return EXIT_FAILURE;
                }
                list[index][j] = new_read_group;
            }
        }
        slow5_close(slow5File_i);
        index++;
        slow5_files.push_back(files[i]);

    }

//    if(lossy==0){
//        int aux_add_fail = 0;
//        for( const auto& pair :set_aux_attr_pairs){
//            if(pair.second==SLOW5_ENUM || pair.second==SLOW5_ENUM_ARRAY){
//                uint8_t n;
//                const char **enum_labels = (const char** )slow5_get_aux_enum_labels(slow5File->header, pair.first.c_str(), &n);
//                if(!enum_labels){
//                    aux_add_fail = 1;
//                }
//                if(slow5_aux_meta_add_enum(slow5File->header->aux_meta, pair.first.c_str(), pair.second, enum_labels, n)){
//                    aux_add_fail = 1;
//                }
//            }else{
//                if(slow5_aux_meta_add(slow5File->header->aux_meta, pair.first.c_str(), pair.second)){
//                    aux_add_fail = 1;
//                }
//            }
//            if(aux_add_fail){
//                ERROR("Could not initialize the record attribute '%s'", pair.first.c_str());
//                return EXIT_FAILURE;
//            }
//        }
//    }

   if(slow5_files.size()==0){
        ERROR("No slow5/blow5 files found for conversion. Exiting...%s","");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[%s] Allocating new read group numbers - took %.3fs\n", __func__, slow5_realtime() - realtime0);

    //now write the header to the slow5File. Use Binary non compress method for fast writing
    slow5_press_method_t method = {pressMethodRecord, pressMethodSignal};
    if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, method) == -1){
        ERROR("Could not write the header to %s\n", arg_fname_out);
        return EXIT_FAILURE;
    }

    double time_get_to_mem = 0;
    double time_thread_execution = 0;
    double time_write = 0;

    int64_t batch_size = read_id_batch_capacity;
    size_t slow5_file_index = 0;
    int flag_EOF = 0;

    struct slow5_file *from = slow5_open(slow5_files[slow5_file_index].c_str(), "r");
    if (from == NULL) {
        ERROR("File '%s' could not be opened - %s.", slow5_files[slow5_file_index].c_str(), strerror(errno));
        return EXIT_FAILURE;
    }

    while(1) {

        db_t db = { 0 };
        db.mem_records = (char **) malloc(batch_size * sizeof *db.read_id);
        db.mem_bytes = (size_t *) malloc(batch_size * sizeof *db.read_id);

        int64_t record_count = 0;
        size_t bytes;
        char *mem;
        double realtime = slow5_realtime();
        while (record_count < batch_size) {
            if (!(mem = (char *) slow5_get_next_mem(&bytes, from))) {
                if (slow5_errno != SLOW5_ERR_EOF) {
                    return EXIT_FAILURE;
                } else { //EOF file reached
                    flag_EOF = 1;
                    break;
                }
            } else {
                db.mem_records[record_count] = mem;
                db.mem_bytes[record_count] = bytes;
                record_count++;
            }
        }
        time_get_to_mem += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        // Setup multithreading structures
        core_t core;
        core.num_thread = num_threads;
        core.fp = from;
        core.format_out = format_out;
        slow5_press_method_t method = {pressMethodRecord,pressMethodSignal};
        core.press_method = method;
        core.lossy = lossy;
        core.slow5_file_index = slow5_file_index;

        db.n_batch = record_count;
        db.read_record = (raw_record_t*) malloc(record_count * sizeof *db.read_record);
        db.list = list;
        work_db(&core,&db,parallel_reads_model);
        time_thread_execution += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        for (int64_t i = 0; i < record_count; i++) {
            fwrite(db.read_record[i].buffer,1,db.read_record[i].len,slow5File->fp);
            free(db.read_record[i].buffer);
        }
        time_write += slow5_realtime() - realtime;

        // Free everything
        free(db.mem_bytes);
        free(db.mem_records);
        free(db.read_record);

        if(flag_EOF){
            flag_EOF = 0;
            if (slow5_close(from) == EOF) { //close file
                ERROR("File '%s' failed on closing - %s.", slow5_files[slow5_file_index].c_str(), strerror(errno));
                return EXIT_FAILURE;
            }
            slow5_file_index++;
            if(slow5_file_index == slow5_files.size()){
                break;
            }else{
                from = slow5_open(slow5_files[slow5_file_index].c_str(), "r");
                if (from == NULL) {
                    ERROR("File '%s' could not be opened - %s.", slow5_files[slow5_file_index].c_str(), strerror(errno));
                    return EXIT_FAILURE;
                }
            }
        }
    }
    if (meta->verbosity_level >= LOG_DEBUG) {
        DEBUG("time_get_to_mem\t%.3fs", time_get_to_mem);
        DEBUG("time_thread_execution\t%.3fs", time_thread_execution);
        DEBUG("time_write\t%.3fs", time_write);
    }

    if (format_out == SLOW5_FORMAT_BINARY) {
        slow5_eof_fwrite(slow5File->fp);
    }
    slow5_close(slow5File);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}

int compare_headers(slow5_hdr_t *output_header, slow5_hdr_t *input_header, int64_t output_g, int64_t input_g) {
    uint32_t output_num_attrs, input_num_attrs;
    output_num_attrs = output_header->data.num_attrs;
    input_num_attrs = input_header->data.num_attrs;
    if(output_num_attrs != input_num_attrs){
        return -1; //number of attributes are different
    }
    khash_t(slow5_s2s) *rg_o = slow5_hdr_get_data(output_g, output_header);
    khash_t(slow5_s2s) *rg_i = slow5_hdr_get_data(input_g, input_header);

    for (khint_t itr = kh_begin(rg_o); itr != kh_end(rg_o); ++itr) {  // traverse hash_table_o
        if (kh_exist(rg_o, itr)) {
            const char* key_o = kh_key(rg_o, itr);
            khint_t pos_i = kh_get(slow5_s2s, rg_i, key_o);
            if(pos_i == kh_end(rg_i)){
                return -2; //an attribute in hash_table_o is not available in hash_table_i
            }
            const char *value_o = kh_value(rg_o, itr);
            const char *value_i = kh_value(rg_i, pos_i);
            if(strcmp(value_o,value_i)){
                return -3; //the attribute values are different
            }
        }
    }
    return 0;
}
