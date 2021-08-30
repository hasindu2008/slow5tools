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
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Concatenate slow5s with same run_id, compression type, and file extension\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n"       \
    "    -o, --output [FILE]                output contents to FILE [default: stdout]\n" \
    "    -h, --help                         display this message and exit\n" \

int concat_main(int argc, char **argv, struct program_meta *meta){
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
    int output_file_set = 0;

    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    slow5_press_method_t pressMethod = {SLOW5_COMPRESS_ZLIB,SLOW5_COMPRESS_NONE};

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

    std::string output_file;
    std::string extension = format;
    if(arg_fname_out){
        output_file = std::string(arg_fname_out);
        extension = output_file.substr(output_file.length()-6, output_file.length());
        output_file_set = 1;
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
        ERROR("No %s files found to concat. Exiting...",format.c_str());
        return EXIT_FAILURE;
    }

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

    slow5_file_t* slow5File = slow5_init_empty(slow5_file_pointer, arg_fname_out, format_out);


    int first_iteration = 1;
    uint32_t num_read_groups = 1;
    std::vector<std::string> run_ids;

    size_t num_files = slow5_files.size();
    for(size_t i=0; i<num_files; i++) { //iterate over slow5files
        slow5_file_t *slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if (!slow5File_i) {
            ERROR("[Skip file]: cannot open %s. skipping...\n", slow5_files[i].c_str());
            return EXIT_FAILURE;
        }
        if(first_iteration){
            // set parameters
            if (slow5File_i->header->aux_meta == NULL) {
                lossy = 1;
            }
            format_out = slow5File_i->format;
            pressMethod = {slow5File_i->compress->record_press->method,slow5File_i->compress->signal_press->method};
            num_read_groups = slow5File_i->header->num_read_groups;

            if(output_file_set && format_out==SLOW5_FORMAT_ASCII && extension!=".slow5"){
                ERROR("Output file extension '%s' does not match with the input file format:FORMAT_ASCII", extension.c_str());
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }else if(output_file_set && format_out==SLOW5_FORMAT_BINARY && extension!=".blow5"){
                ERROR("Output file extension '%s' does not match with the input file format:FORMAT_BINARY", extension.c_str());
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }

            if(lossy==0){
                struct slow5_aux_meta *aux_meta = slow5_aux_meta_init_empty();
                slow5File->header->aux_meta = aux_meta;
                slow5_aux_meta_t* aux_ptr = slow5File_i->header->aux_meta;
                uint32_t num_aux_attrs = aux_ptr->num;
                for(uint32_t r=0; r<num_aux_attrs; r++){
                    if(slow5_aux_meta_add(slow5File->header->aux_meta, aux_ptr->attrs[r], aux_ptr->types[r])){
                        ERROR("Could not initialize the record attribute '%s'", aux_ptr->attrs[r]);
                        return EXIT_FAILURE;
                    }
                }
            }
            //set run_ids
            for(uint32_t j=0; j<num_read_groups; j++){
                char* temp = slow5_hdr_get("run_id", 0, slow5File_i->header);
                if(!temp){
                    ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                    return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
                }
            }

            slow5File->header->num_read_groups = num_read_groups;
            //now write the header to the slow5File.
            if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, pressMethod) == -1){
                ERROR("Could not write the header to %s\n", arg_fname_out);
                slow5_close(slow5File_i);
                return EXIT_FAILURE;
            }

            first_iteration = 0;
        }else {
            if (lossy == 0 && slow5File_i->header->aux_meta == NULL) {
                ERROR("%s has no auxiliary fields. Use merge (instead of concat) to merge files.",
                      slow5_files[i].c_str());
                slow5_close(slow5File_i);
                return EXIT_FAILURE;
            }
            if (lossy == 1 && slow5File_i->header->aux_meta != NULL) {
                ERROR("%s has auxiliary fields. Use merge (instead of concat) to merge files.",
                      slow5_files[i].c_str());
                slow5_close(slow5File_i);
                continue;
            }
            if (slow5File_i->format != format_out) {
                ERROR("%s has a different file format. Use merge (instead of concat) to merge files.",
                      slow5_files[i].c_str());
                slow5_close(slow5File_i);
                return EXIT_FAILURE;
            }
            if (slow5File_i->compress->record_press->method != pressMethod.record_method) { //todo check for signal compression as well
                ERROR("%s has a different compression type. Use merge (instead of concat) to merge files.",
                      slow5_files[i].c_str());
                slow5_close(slow5File_i);
                return EXIT_FAILURE;
            }
            if (slow5File_i->header->num_read_groups != num_read_groups) {
                ERROR("%s has a different number of read groups than the first file. Use merge (instead of concat) to merge files.",
                      slow5_files[i].c_str());
                slow5_close(slow5File_i);
                return EXIT_FAILURE;
            }

            for (uint32_t j = 0; j < num_read_groups; j++) {
                char *temp = slow5_hdr_get("run_id", 0, slow5File_i->header);
                if (!temp) {
                    ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                    return EXIT_FAILURE;
                }
                if (strcmp(temp, run_ids[j].c_str())) {
                    ERROR("%s has a different run_id. Use merge (instead of concat) to merge files.",
                          slow5_files[i].c_str());
                    slow5_close(slow5File_i);
                    return EXIT_FAILURE;
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
