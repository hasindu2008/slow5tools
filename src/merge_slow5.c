//
// Created by Hiruna on 2021-01-16.
//
#include "slow5.h"
#include "error.h"

static double init_realtime = 0;

int merge_slow5(FILE *f_out, std::vector<std::string> &slow5_files, reads_count* readsCount);

int compare_headers(slow5_header_t& slow5Header1, slow5_header_t& slow5Header2, int compare_level);


#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "merge slow5 files\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help                 display this message and exit\n" \
    "    -o, --output=[FILE]        output converted contents to FILE -- stdout\n" \

int merge_main(int argc, char **argv, struct program_meta *meta){
    init_realtime = realtime();

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
            {"output", required_argument, NULL, 'o'},   //1
            {NULL, 0, NULL, 0 }
    };

    // Default options
    FILE *f_out = stdout;
    enum FormatOut format_out = OUT_ASCII;

    // Input arguments
    char *arg_fname_out = NULL;

    char opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "ho:", long_opts, &longindex)) != -1) {
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
                arg_fname_out = optarg;
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    // Parse output argument
    if (arg_fname_out != NULL) {
        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing output filename%s","");
        }
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(arg_fname_out, "w");

        // An error occured
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            f_out = new_file;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing slow5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    double realtime0 = realtime();
    reads_count readsCount;
    std::vector<std::string> slow5_files;

    for (int i = optind; i < argc; ++ i) {
        find_all_5(argv[i], slow5_files, SLOW5_EXTENSION);
    }
    fprintf(stderr, "[%s] %ld fast5 files found - took %.3fs\n", __func__, slow5_files.size(), realtime() - realtime0);

    if(merge_slow5(f_out, slow5_files, &readsCount)==-1){
        return EXIT_FAILURE;
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

int merge_slow5(FILE *f_out, std::vector<std::string> &slow5_files, reads_count* readsCount) {
    size_t read_group_count = 0;
    size_t slow5_files_count = slow5_files.size();
    readsCount->total_5 = slow5_files_count;

    std::vector<FILE*>slow_files_pointers(slow5_files_count);
    std::vector<slow5_header_t> slow5_headers(slow5_files_count);
    std::vector<std::vector<size_t>> list; //always use list to access slow5_headers. slow5_headers has invalid files' headers too.

    for(size_t i =0; i<slow5_files_count; i++) {
        slow_files_pointers[i] = fopen(slow5_files[i].c_str(), "r"); // read mode
        if (slow_files_pointers[i] == NULL) {
            WARNING("slow5 file [%s] is unreadable and will be skipped", slow5_files[i].c_str());
            return -1;
        }

        if (read_single_group_slow5_header(slow_files_pointers[i], slow5_headers[i]) == -1) {
            WARNING("slow5 file [%s] is unreadable and will be skipped", slow5_files[i].c_str());
            readsCount->bad_5_file++;
            continue;
        }
        if (slow5_headers[i].num_read_groups > 1) {
            WARNING("[%s] has multiple read groups, hence skipped. Use 'split' to create single read group files.",
                    slow5_files[i].c_str());
            readsCount->multi_group_slow5++;
            continue;
        }
//        fprintf(stderr,"slow5=%s run_id=%s\n",slow5_files[i].c_str(),slow5_headers[i].run_id);

        size_t flag_run_id_exist = 0;
        //check if run_id is already in the list
        for (size_t j = 0; j < list.size(); j++) {
            if (strcmp(slow5_headers[list[j][0]].run_id, slow5_headers[i].run_id) == 0) {
                list[j].push_back(i);
                flag_run_id_exist = 1;
                break;
            }
        }
        //if run_id is new, add it
        if (flag_run_id_exist == 0) {
            list.push_back(std::vector<size_t>{i});
            read_group_count++;
        }
    }

    // compare headers
    for(size_t j=0; j<list.size(); j++){
        for(size_t k=0; k<list[j].size(); k++){
            if(compare_headers(slow5_headers[list[0][0]],slow5_headers[list[j][k]],0)==-1){
                ERROR("%s and %s are different. Cannot merge. Exiting..\n",slow5_files[list[0][0]].c_str(),slow5_files[list[j][k]].c_str());
                return -1;
            }
            if(compare_headers(slow5_headers[list[j][0]],slow5_headers[list[j][k]],1)==-1){
                WARNING("%s and %s files have same run_id but different headers",slow5_files[list[j][0]].c_str(),slow5_files[list[j][k]].c_str());
            }
        }
    }

    print_multi_group_header(f_out, slow5_headers, list, read_group_count);
    print_multi_group_records(f_out, slow_files_pointers, list, read_group_count);

    //free slow5 headers
    for(size_t i =0; i<slow5_files_count; i++){
        if(slow_files_pointers[i]){
            fclose(slow_files_pointers[i]);
        }

        //context tags
        if(slow5_headers[i].sample_frequency)free(slow5_headers[i].sample_frequency);
        //additional attributes in 2.2
        if(slow5_headers[i].barcoding_enabled)free(slow5_headers[i].barcoding_enabled);
        if(slow5_headers[i].experiment_duration_set)free(slow5_headers[i].experiment_duration_set);
        if(slow5_headers[i].experiment_type)free(slow5_headers[i].experiment_type);
        if(slow5_headers[i].local_basecalling)free(slow5_headers[i].local_basecalling);
        if(slow5_headers[i].package)free(slow5_headers[i].package);
        if(slow5_headers[i].package_version)free(slow5_headers[i].package_version);
        if(slow5_headers[i].sequencing_kit)free(slow5_headers[i].sequencing_kit);
        //additional attributes in 2.0
        if(slow5_headers[i].filename)free(slow5_headers[i].filename);
        if(slow5_headers[i].experiment_kit)free(slow5_headers[i].experiment_kit);
        if(slow5_headers[i].user_filename_input)free(slow5_headers[i].user_filename_input);
        //tracking_id
        if(slow5_headers[i].asic_id)free(slow5_headers[i].asic_id);
        if(slow5_headers[i].asic_id_eeprom)free(slow5_headers[i].asic_id_eeprom);
        if(slow5_headers[i].asic_temp)free(slow5_headers[i].asic_temp);
        if(slow5_headers[i].auto_update)free(slow5_headers[i].auto_update);
        if(slow5_headers[i].auto_update_source)free(slow5_headers[i].auto_update_source);
        if(slow5_headers[i].bream_is_standard)free(slow5_headers[i].bream_is_standard);
        if(slow5_headers[i].device_id)free(slow5_headers[i].device_id);
        if(slow5_headers[i].exp_script_name)free(slow5_headers[i].exp_script_name);
        if(slow5_headers[i].exp_script_purpose)free(slow5_headers[i].exp_script_purpose);
        if(slow5_headers[i].exp_start_time)free(slow5_headers[i].exp_start_time);
        if(slow5_headers[i].flow_cell_id)free(slow5_headers[i].flow_cell_id);
        if(slow5_headers[i].heatsink_temp)free(slow5_headers[i].heatsink_temp);
        if(slow5_headers[i].hostname)free(slow5_headers[i].hostname);
        if(slow5_headers[i].installation_type)free(slow5_headers[i].installation_type);
        if(slow5_headers[i].local_firmware_file)free(slow5_headers[i].local_firmware_file);
        if(slow5_headers[i].operating_system)free(slow5_headers[i].operating_system);
        if(slow5_headers[i].protocol_run_id)free(slow5_headers[i].protocol_run_id);
        if(slow5_headers[i].protocols_version)free(slow5_headers[i].protocols_version);
        if(slow5_headers[i].tracking_id_run_id)free(slow5_headers[i].tracking_id_run_id);
        if(slow5_headers[i].usb_config)free(slow5_headers[i].usb_config);
        if(slow5_headers[i].version)free(slow5_headers[i].version);
        if(slow5_headers[i].asic_version)free(slow5_headers[i].asic_version);
        //2.2
        if(slow5_headers[i].configuration_version)free(slow5_headers[i].configuration_version);
        if(slow5_headers[i].device_type)free(slow5_headers[i].device_type);
        if(slow5_headers[i].distribution_status)free(slow5_headers[i].distribution_status);
        if(slow5_headers[i].distribution_version)free(slow5_headers[i].distribution_version);
        if(slow5_headers[i].flow_cell_product_code)free(slow5_headers[i].flow_cell_product_code);
        if(slow5_headers[i].guppy_version)free(slow5_headers[i].guppy_version);
        if(slow5_headers[i].protocol_group_id)free(slow5_headers[i].protocol_group_id);
        if(slow5_headers[i].sample_id)free(slow5_headers[i].sample_id);
        //2.0
        if(slow5_headers[i].bream_core_version)free(slow5_headers[i].bream_core_version);
        if(slow5_headers[i].bream_ont_version)free(slow5_headers[i].bream_ont_version);
        if(slow5_headers[i].bream_prod_version)free(slow5_headers[i].bream_prod_version);
        if(slow5_headers[i].bream_rnd_version)free(slow5_headers[i].bream_rnd_version);

        //other
        if((char*)slow5_headers[i].file_format)free((char*)slow5_headers[i].file_format);
        if(slow5_headers[i].file_version)free(slow5_headers[i].file_version);
        if(slow5_headers[i].file_type)free(slow5_headers[i].file_type);
        if(slow5_headers[i].pore_type)free(slow5_headers[i].pore_type);
        if(slow5_headers[i].run_id)free(slow5_headers[i].run_id);
    }

    return 1;
}

int compare_headers(slow5_header_t& slow5Header1, slow5_header_t& slow5Header2, int compare_level) {
    switch (compare_level) {
        case 0:
            if(strcmp(slow5Header1.file_format,slow5Header2.file_format)!=0){
                fprintf(stderr,"file_format mismatch:%s %s\n",slow5Header1.file_format,slow5Header2.file_format);
                return -1;
            }
            if(strcmp(slow5Header1.file_version,slow5Header2.file_version)!=0){
                fprintf(stderr,"file_version mismatch:%s %s\n",slow5Header1.file_version,slow5Header2.file_version);
                return -1;
            }
            return 1;
            break;
        case 1:
            if(strcmp(slow5Header1.run_id,slow5Header2.run_id)!=0){
                fprintf(stderr,"run_id mismatch:%s %s\n",slow5Header1.run_id,slow5Header2.run_id);
                return -1;
            }
            if(strcmp(slow5Header1.sample_frequency,slow5Header2.sample_frequency)!=0){
                fprintf(stderr,"sample_frequency mismatch:%s %s\n",slow5Header1.sample_frequency,slow5Header2.sample_frequency);
                return -1;
            }
            //todo: implement more comparisons...

            break;
        default:
            fprintf(stderr,"internal error\n");
            break;
    }

}
