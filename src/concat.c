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
    "    -l, --lossless [STR]               concat files with auxilliary fields.[default: true].\n" \
    "    --format                           concat file format [default:.blow5]\n"\
    "    -c, --compress [compression_type]  concat compressed blow5 [default: zlib]\n" \
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
            { "lossless", required_argument, NULL, 'l'}, //2
            { "format", required_argument, NULL, 'f'}, //3
            {"compress", required_argument, NULL, 'c'},  //4
            {NULL, 0, NULL, 0 }
    };

    // Input arguments
    char *arg_fname_out = NULL;
    std::string format =  ".blow5";
    int lossy = 0;

    enum slow5_fmt format_out = SLOW5_FORMAT_BINARY;
    enum slow5_press_method pressMethod = SLOW5_COMPRESS_ZLIB;
    int compression_set = 0;

    int longindex = 0;
    int opt;

    // Parse options
    while ((opt = getopt_long(argc, argv, "ho:l:f:c:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
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
            case 'f':
                if(strcmp(optarg,"slow5")==0){
                    format_out = SLOW5_FORMAT_ASCII;
                    format = ".slow5";
                }else if(strcmp(optarg,"blow5")==0){
                    format_out = SLOW5_FORMAT_BINARY;
                    format = ".blow5";
                }else{
                    ERROR("Incorrect file format%s", "");
                    return EXIT_FAILURE;
                }
                break;
            case 'c':
                compression_set = 1;
                if(strcmp(optarg,"none")==0){
                    pressMethod = SLOW5_COMPRESS_NONE;
                }else if(strcmp(optarg,"zlib")==0){
                    pressMethod = SLOW5_COMPRESS_ZLIB;
                }else{
                    ERROR("Incorrect compression type%s", "");
                    return EXIT_FAILURE;
                }
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
    if(compression_set == 0 && format_out == SLOW5_FORMAT_ASCII){
        pressMethod = SLOW5_COMPRESS_NONE;
    }
    // compression option is only effective with -b blow5
    if(compression_set == 1 && format_out == SLOW5_FORMAT_ASCII){
        ERROR("%s","Compression option (-c) is only available for SLOW5 binary format.");
        return EXIT_FAILURE;
    }

    std::string output_file;
    std::string extension = format;
    if(arg_fname_out){
        output_file = std::string(arg_fname_out);
        extension = output_file.substr(output_file.length()-6, output_file.length());
    }

    if(arg_fname_out && format_out==SLOW5_FORMAT_ASCII && extension!=".slow5"){
        ERROR("Output file extension '%s' does not match with the input file format:FORMAT_ASCII", extension.c_str());
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }else if(arg_fname_out && format_out==SLOW5_FORMAT_BINARY && extension!=".blow5"){
        ERROR("Output file extension '%s' does not match with the input file format:FORMAT_BINARY", extension.c_str());
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
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
    slow5_hdr_initialize(slow5File->header, lossy);
    slow5File->header->num_read_groups = 0;

    int header_write = 1;
    int run_id_capture = 1;
    std::string run_id;
    size_t num_files = slow5_files.size();
    for(size_t i=0; i<num_files; i++) { //iterate over slow5files
        slow5_file_t *slow5File_i = slow5_open(slow5_files[i].c_str(), "r+");
        if (!slow5File_i) {
            ERROR("[Skip file]: cannot open %s. skipping...\n", slow5_files[i].c_str());
            continue;
        }
        if (lossy == 0 && slow5File_i->header->aux_meta == NULL) {
            WARNING("[Skip file]: %s has no auxiliary fields. Use merge (instead of concat) to merge files.",
                  slow5_files[i].c_str());
            slow5_close(slow5File_i);
            continue;
        }
        if (lossy == 1 && slow5File_i->header->aux_meta != NULL) {
            WARNING("[Skip file]: %s has auxiliary fields. Use merge (instead of concat) to merge files.",
                    slow5_files[i].c_str());
            slow5_close(slow5File_i);
            continue;
        }
        if (slow5File_i->format != format_out) {
            WARNING("[Skip file]: %s has a different file format. Use merge (instead of concat) to merge files.",
                    slow5_files[i].c_str());
            slow5_close(slow5File_i);
            continue;
        }
        if (slow5File_i->compress->method != pressMethod) {
            WARNING("[Skip file]: %s has a different compression type. Use merge (instead of concat) to merge files.",
                    slow5_files[i].c_str());
            slow5_close(slow5File_i);
            continue;
        }
        if(run_id_capture){
            run_id_capture = 0;
            char* temp = slow5_hdr_get("run_id", 0, slow5File_i->header);
            if(!temp){
                ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                return EXIT_FAILURE;
            }
            run_id = std::string(temp);

        }else{
            char* run_id_check = slow5_hdr_get("run_id", 0, slow5File_i->header);
            if(!run_id_check){
                ERROR("No run_id information found in %s.", slow5_files[i].c_str());
                return EXIT_FAILURE;
            }
            if(strcmp(run_id_check,run_id.c_str())){
                WARNING("[Skip file]: %s has a different run_id. Use merge (instead of concat) to merge files.",
                        slow5_files[i].c_str());
                slow5_close(slow5File_i);
                continue;
            }
        }
        if (slow5File_i->compress->method != pressMethod) {
            WARNING("[Skip file]: %s has a different compression type. Use merge (instead of concat) to merge files.",
                    slow5_files[i].c_str());
            slow5_close(slow5File_i);
            continue;
        }

        if(header_write){
            header_write = 0;
            khash_t(slow5_s2s) *rg = slow5_hdr_get_data(0, slow5File_i->header); // extract jth read_group related data from ith slow5file
            int64_t new_read_group = slow5_hdr_add_rg_data(slow5File->header, rg); //assumption0
            if(new_read_group != 0){ //sanity check
                ERROR("New read group number is not equal to 0. something's wrong\n%s", "");
                return EXIT_FAILURE;
            }
            //now write the header to the slow5File.
            if(slow5_hdr_fwrite(slow5File->fp, slow5File->header, format_out, pressMethod) == -1){
                ERROR("Could not write the header to %s\n", arg_fname_out);
                return EXIT_FAILURE;
            }
        }
        if(format_out==SLOW5_FORMAT_ASCII) {
            // BUFSIZE default is 8192 bytes
            //writing to reads to the output
            // BUFSIZE of 1 means one chareter at time
            // good values should fit to blocksize, like 1024 or 4096
            // higher values reduce number of system calls
            char buf[BUFSIZ];
            size_t size;
            while ((size = fread(buf, 1, BUFSIZ, slow5File_i->fp))) {
                fwrite(buf, 1, size, slow5File->fp);
            }
            slow5_close(slow5File_i);
        }else{
//            DEBUG("%d",101);
            int64_t pos = ftell(slow5File_i->fp);
            fseek(slow5File_i->fp, -5, SEEK_END); //slow5 EOF file "5WOLB"
            off_t  length = ftell(slow5File_i->fp);
            if(ftruncate(fileno(slow5File_i->fp),length) != 0){
                ERROR("Truncating file %s failed.", slow5_files[i].c_str());
                return EXIT_FAILURE;
            }
            fseek(slow5File_i->fp, pos, SEEK_SET);

            // BUFSIZE default is 8192 bytes
            //writing to reads to the output
            // BUFSIZE of 1 means one chareter at time
            // good values should fit to blocksize, like 1024 or 4096
            // higher values reduce number of system calls
            char buf[BUFSIZ];
            size_t size;
            while ((size = fread(buf, 1, BUFSIZ, slow5File_i->fp))) {
                fwrite(buf, 1, size, slow5File->fp);
            }
            slow5_close(slow5File_i);

//            struct slow5_rec *read = NULL;
//            struct slow5_press* compress = slow5_press_init(pressMethod);
//            int ret;
//            while ((ret = slow5_get_next(&read, slow5File_i)) >= 0) {
//                if (slow5_rec_fwrite(slow5File->fp, read, slow5File->header->aux_meta, format_out, compress) == -1) {
//                    slow5_rec_free(read);
//                    ERROR("Could not write records to the file %s\n", arg_fname_out);
//                    exit(EXIT_FAILURE);
//                }
//            }
//            slow5_rec_free(read);
//            slow5_press_free(compress);
//            slow5_close(slow5File_i);

        }
    }

    if (format_out == SLOW5_FORMAT_BINARY) {
        slow5_eof_fwrite(slow5File->fp);
    }
    slow5_close(slow5File);

    return EXIT_SUCCESS;
}
