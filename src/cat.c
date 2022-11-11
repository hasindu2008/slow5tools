/**
 * @file stats.c
 * @brief summarize a SLOW5 file
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au)
 * @date 27/02/2021
 */

#include <getopt.h>
#include <sys/wait.h>
#include <string>
#include "error.h"
#include "cmd.h"
#include "slow5_extra.h"
#include "read_fast5.h"
#include "misc.h"
#include <slow5/slow5_press.h>

#define USAGE_MSG "Usage: %s [SLOW5_FILE/DIR]\n"
#define HELP_LARGE_MSG \
    "Quickly concatenate SLOW5/BLOW5 files of same type (same header, extension, compression)\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n"       \
    HELP_MSG_OUTPUT_FILE \

extern int slow5tools_verbosity_level;
int close_files_and_exit(slow5_file_t *slow5_file, slow5_file_t *slow5_file_i, char *arg_fname_out);

// return 0 if no warnings
// return 1 if warnings are found
int compare_headers(slow5_hdr_t *output_header, slow5_hdr_t *input_header, int64_t output_g, int64_t input_g, const char *i_file_path, const char *j_run_id) {
    int flag_warnings = 0;
    khash_t(slow5_s2s) *rg_o = slow5_hdr_get_data(output_g, output_header);
    khash_t(slow5_s2s) *rg_i = slow5_hdr_get_data(input_g, input_header);

    size_t size_rg_o = kh_size(rg_o);
    size_t size_rg_i = kh_size(rg_i);

//    fprintf(stderr,"size_rg_i,size_rg_i\t%zu,%zu\n",size_rg_i,size_rg_i);
    if(size_rg_i != size_rg_o){
        flag_warnings = 1;
        WARNING("Input file %s (run_id-%s) has a different number of attributes (%zu) than seen in the previous files processed so far (%zu)", i_file_path, j_run_id, size_rg_i, size_rg_o);
    }

    for (khint_t itr = kh_begin(rg_o); itr != kh_end(rg_o); ++itr) {  // traverse hash_table_o
        if (kh_exist(rg_o, itr)) {
            int flag_set_attr_value_to_empty = 0;
            const char* key_o = kh_key(rg_o, itr);
            khint_t pos_i = kh_get(slow5_s2s, rg_i, key_o);
            if(pos_i == kh_end(rg_i)){
                WARNING("Attribute '%s' is not available in input file %s (run_id-%s)", key_o, i_file_path, j_run_id);
                flag_warnings = 1;
                flag_set_attr_value_to_empty = 1;
            } else{
                const char *value_o = kh_value(rg_o, itr);
                const char *value_i = kh_value(rg_i, pos_i);
                if(strcmp(value_o,value_i)){
                    WARNING("Attribute '%s' in input file %s (run_id-%s) has a different value (%s) than what has been seen so far (%s)", key_o, i_file_path, j_run_id, value_i, value_o);
                    flag_warnings = 1;
                    flag_set_attr_value_to_empty = 1;
                }
            }
            if(flag_set_attr_value_to_empty){
                INFO("Setting output header's attribute '%s' (run_id-%s) to empty", key_o, j_run_id);
                int ret_hdr_attr_set = slow5_hdr_set(key_o, "", output_g, output_header);
                if(ret_hdr_attr_set<0){
                    ERROR("Could not set attribute '%s' value to empty", key_o);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    for (khint_t itr = kh_begin(rg_i); itr != kh_end(rg_i); ++itr) {  // traverse hash_table_o
        if (kh_exist(rg_i, itr)) {
            const char* key_i = kh_key(rg_i, itr);
            khint_t pos_o = kh_get(slow5_s2s, rg_o, key_i);
            if(pos_o == kh_end(rg_o)){
                const char *value_i = kh_value(rg_i, itr);
                flag_warnings = 1;
                INFO("Attribute '%s' in input file %s (run_id-%s) is not seen in previous files. It will be added to the output header but its value (%s) will not be set in the output header.", key_i, i_file_path, j_run_id, value_i);
                int ret_attr_header = slow5_hdr_add_attr(key_i, output_header);
                if(ret_attr_header==-1){
                    ERROR("Could not add the new attribute %s to the output header", key_i);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return flag_warnings;
}

int compare_aux_attrs(slow5_hdr_t *output_header, slow5_hdr_t *input_header, const char *i_file_path) {
    uint32_t num_axu_o = output_header->aux_meta->num;
    uint32_t num_axu_i = input_header->aux_meta->num;
    if(num_axu_o != num_axu_i){
        ERROR("Input file %s has a different number of auxiliary attributes (%" PRIu32 ") than seen in the previous files processed so far (%" PRIu32 ")", i_file_path, num_axu_o, num_axu_i);
        return -1;
    }
    for(uint32_t i=0; i<num_axu_o; i++){
        DEBUG("%s\t%s\n", output_header->aux_meta->attrs[i], input_header->aux_meta->attrs[i]);
        if(strcmp(output_header->aux_meta->attrs[i],input_header->aux_meta->attrs[i])){
            ERROR("Input file %s has a different order of auxiliary attributes from the order seen in the previous files processed so far", i_file_path);
            return -1;
        }
    }
    return 0;
}

int cat_main(int argc, char **argv, struct program_meta *meta){

    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h' }, //0
            {"output", required_argument, NULL, 'o'}, //1
            {NULL, 0, NULL, 0 }
    };

    int lossy = 0;
    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum slow5_press_method pressMethodRecord = SLOW5_COMPRESS_ZLIB;
    enum slow5_press_method pressMethodSignal = SLOW5_COMPRESS_NONE;

    opt_t user_opts;
    init_opt(&user_opts);

    int longindex = 0;
    int opt;

    // Parse options
    while ((opt = getopt_long(argc, argv, "ho:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'o':
                user_opts.arg_fname_out = optarg;
                break;
            case 'h':
                DEBUG("displaying large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }
    if(auto_detect_formats(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
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
    std::vector<std::string> slow5_files;
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], slow5_files, 0, ".slow5");
    }
    VERBOSE("%ld files found - took %.3fs\n", slow5_files.size(), slow5_realtime() - realtime0);

    if(slow5_files.size()==0){
        ERROR("No %s files found to cat. Exiting...", ".slow5");
        return EXIT_FAILURE;
    }

    // Parse output argument
    if (user_opts.arg_fname_out != NULL) {
        DEBUG("opening output file%s","");
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(user_opts.arg_fname_out, "w");

        // An error occurred
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  user_opts.arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            user_opts.f_out = new_file;
        }
    }

    WARNING("%s","slow5tools cat is much faster than merge, but performs minimal input validation. Use with caution.");

    slow5_file_t* slow5File = NULL;
    int first_iteration = 1;
    uint32_t num_read_groups = 1;
    std::vector<std::string> run_ids;
    size_t num_files = slow5_files.size();
    for(size_t i=0; i<num_files; i++) { //iterate over slow5files
        slow5_file_t *slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if (!slow5File_i) {
            ERROR("[Skip file]: cannot open %s. skipping...\n", slow5_files[i].c_str());
            return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
        }
        if(first_iteration){
            // set parameters
            if (slow5File_i->header->aux_meta == NULL) {
                lossy = 1;
            }
            format_out = slow5File_i->format;
            if(user_opts.arg_fname_out && format_out!=user_opts.fmt_out){
                ERROR("Output file extension does not match with the output format%s",".");
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            pressMethodRecord = slow5File_i->compress->record_press->method;
            pressMethodSignal = slow5File_i->compress->signal_press->method;
            slow5_press_method_t press_out = {pressMethodRecord,pressMethodSignal};
            num_read_groups = slow5File_i->header->num_read_groups;
            slow5File = slow5_init_empty(user_opts.f_out, user_opts.arg_fname_out, format_out);

            int ret0 = slow5_hdr_initialize(slow5File->header, lossy);
            if(ret0<0){
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
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
                        return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                    }
                }
            }
            //set run_ids
            for(uint32_t j=0; j<num_read_groups; j++){
                char* temp = slow5_hdr_get("run_id", j, slow5File_i->header);
                if(!temp){
                    ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                    return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                }
                run_ids.push_back(std::string(temp));
                //write header
                khash_t(slow5_s2s) *rg = slow5_hdr_get_data(j, slow5File_i->header); // extract jth read_group related data from ith slow5file
                if(!rg){
                    ERROR("Could not fetch the header data map.%s","");
                }
                int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg); //assumption0: if run_ids are similar the rest of the header attribute values of jth read_groups to follow will be similar.
                if(new_read_group<0){
                    ERROR("Could not assign a new read group. %s","");
                    return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                }
            }
            //now write the header to the slow5File.
            if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, press_out) == -1){
                ERROR("Could not write the header to %s\n", user_opts.arg_fname_out);
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            first_iteration = 0;
        }else {
            if (lossy == 0 && slow5File_i->header->aux_meta == NULL) {
                ERROR("%s has no auxiliary fields. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            if (lossy == 1 && slow5File_i->header->aux_meta != NULL) {
                ERROR("%s has auxiliary fields. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            if (slow5File_i->format != format_out) {
                ERROR("%s has a different file format. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            if (slow5File_i->compress->record_press->method != pressMethodRecord) {
                ERROR("%s has a different record compression type. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            if (slow5File_i->compress->signal_press->method != pressMethodSignal) {
                ERROR("%s has a different signal compression type. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            if (slow5File_i->header->num_read_groups != num_read_groups) {
                ERROR("%s has a different number of read groups than the first file. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
            }
            for (uint32_t j = 0; j < num_read_groups; j++) {
                char *temp = slow5_hdr_get("run_id", j, slow5File_i->header);
                if (!temp) {
                    ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                    return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                }
                if (strcmp(temp, run_ids[j].c_str())) {
                    ERROR("%s has a different run_id. Use merge (instead of cat) to merge files.",
                          slow5_files[i].c_str());
                    return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                }
                /// compare headers
                int ret_compare_headers = compare_headers(slow5File->header, slow5File_i->header, j, j, slow5_files[i].c_str(), run_ids[j].c_str());
                if(ret_compare_headers){
                    return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                }

            }
            if(lossy == 0){
                int ret_compare_aux_attrs = compare_aux_attrs(slow5File->header, slow5File_i->header, slow5_files[i].c_str());
                if(ret_compare_aux_attrs){
                    return close_files_and_exit(slow5File, slow5File_i, user_opts.arg_fname_out);
                }
            }
        }

        //writing to reads to the output
        // BUFSIZE default is 8192 bytes
        // BUFSIZE of 1 means one chareter at time
        // good values should fit to blocksize, like 1024 or 4096
        // higher values reduce number of system calls
        char buf[BUFSIZ];
        size_t size;
        while ((size = fread(buf, 1, BUFSIZ, slow5File_i->fp))) {
            fwrite(buf, 1, size, slow5File->fp);
        }
        slow5_close(slow5File_i);

        if(format_out==SLOW5_FORMAT_BINARY){
            fseek(slow5File->fp, -5, SEEK_END); //slow5 EOF file "5WOLB"
        }
    }

    if (format_out == SLOW5_FORMAT_BINARY) {
        slow5_eof_fwrite(slow5File->fp);
    }
    slow5_close(slow5File);

    return EXIT_SUCCESS;
}

int close_files_and_exit(slow5_file_t *slow5_file, slow5_file_t *slow5_file_i, char *arg_fname_out) {
    if(slow5_file_i){
        slow5_close(slow5_file_i);
    }
    if(slow5_file){
        slow5_close(slow5_file);
    }
    if(arg_fname_out){
        int del = remove(arg_fname_out);
        if (del){
            ERROR("Failed to delete the malformed output file %s", arg_fname_out);
        }
    }
    return EXIT_FAILURE;
}
