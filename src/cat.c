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
    "Concatenate slow5s with same run_id, compression type, and file extension\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n"       \
    "    -o, --output [FILE]                output contents to FILE [default: stdout]\n" \
    "    -h, --help                         display this message and exit\n" \


int close_files_and_exit(slow5_file_t *slow5_file, slow5_file_t *slow5_file_i, char *arg_fname_out);

int cat_main(int argc, char **argv, struct program_meta *meta){
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
            {"help", no_argument, NULL, 'h' }, //0
            {"output", required_argument, NULL, 'o'}, //1
            {NULL, 0, NULL, 0 }
    };

    // Input arguments
    char *arg_fname_out = NULL;
    std::string format =  "low5";
    int lossy = 0;

    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum slow5_fmt extension_format = SLOW5_FORMAT_BINARY;
    enum slow5_press_method pressMethodRecord = SLOW5_COMPRESS_ZLIB;
    enum slow5_press_method pressMethodSignal = SLOW5_COMPRESS_NONE;
//    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
//    slow5_press_method_t pressMethod = {SLOW5_COMPRESS_ZLIB,SLOW5_COMPRESS_NONE};

    int longindex = 0;
    int opt;

    // Parse options
    while ((opt = getopt_long(argc, argv, "ho:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'h':
                if (meta->verbosity_level >= LOG_DEBUG) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
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
        list_all_items(argv[i], slow5_files, 0, format.c_str());
    }
    VERBOSE("%ld files found - took %.3fs\n", slow5_files.size(), slow5_realtime() - realtime0);

    if(slow5_files.size()==0){
        ERROR("No %s files found to cat. Exiting...",format.c_str());
        return EXIT_FAILURE;
    }

    FILE* slow5_file_pointer = stdout;
    if(arg_fname_out){
        slow5_file_pointer = fopen(arg_fname_out, "wb");
        if (!slow5_file_pointer) {
            ERROR("Output file %s could not be opened - %s.", arg_fname_out, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    slow5_file_t* slow5File = NULL;
    int first_iteration = 1;
    uint32_t num_read_groups = 1;
    std::vector<std::string> run_ids;
    size_t num_files = slow5_files.size();
    for(size_t i=0; i<num_files; i++) { //iterate over slow5files
        slow5_file_t *slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if (!slow5File_i) {
            ERROR("[Skip file]: cannot open %s. skipping...\n", slow5_files[i].c_str());
            return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
        }
        if(first_iteration){
            // set parameters
            if (slow5File_i->header->aux_meta == NULL) {
                lossy = 1;
            }
            format_out = slow5File_i->format;
            if(arg_fname_out && format_out!=extension_format){
                ERROR("Output file extension does not match with the output format%s",".");
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            pressMethodRecord = slow5File_i->compress->record_press->method;
            pressMethodSignal = slow5File_i->compress->signal_press->method;
            slow5_press_method_t press_out = {pressMethodRecord,pressMethodSignal};
            num_read_groups = slow5File_i->header->num_read_groups;
            slow5File = slow5_init_empty(slow5_file_pointer, arg_fname_out, format_out);

            int ret0 = slow5_hdr_initialize(slow5File->header, lossy);
            if(ret0<0){
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
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
                        return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
                    }
                }
            }
            //set run_ids
            for(uint32_t j=0; j<num_read_groups; j++){
                char* temp = slow5_hdr_get("run_id", 0, slow5File_i->header);
                if(!temp){
                    ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                    return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
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
                    return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
                }
            }
            //now write the header to the slow5File.
            if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, press_out) == -1){
                ERROR("Could not write the header to %s\n", arg_fname_out);
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            first_iteration = 0;
        }else {
            if (lossy == 0 && slow5File_i->header->aux_meta == NULL) {
                ERROR("%s has no auxiliary fields. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            if (lossy == 1 && slow5File_i->header->aux_meta != NULL) {
                ERROR("%s has auxiliary fields. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            if (slow5File_i->format != format_out) {
                ERROR("%s has a different file format. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            if (slow5File_i->compress->record_press->method != pressMethodRecord) {
                ERROR("%s has a different record compression type. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            if (slow5File_i->compress->signal_press->method != pressMethodSignal) {
                ERROR("%s has a different signal compression type. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            if (slow5File_i->header->num_read_groups != num_read_groups) {
                ERROR("%s has a different number of read groups than the first file. Use merge (instead of cat) to merge files.",
                      slow5_files[i].c_str());
                return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
            }
            for (uint32_t j = 0; j < num_read_groups; j++) {
                char *temp = slow5_hdr_get("run_id", 0, slow5File_i->header);
                if (!temp) {
                    ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                    return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
                }
                if (strcmp(temp, run_ids[j].c_str())) {
                    ERROR("%s has a different run_id. Use merge (instead of cat) to merge files.",
                          slow5_files[i].c_str());
                    return close_files_and_exit(slow5File, slow5File_i, arg_fname_out);
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
