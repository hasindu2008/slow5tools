//
// Created by Hiruna Samarkoon on 2021-06-01.
//

#include <getopt.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include "error.h"
#include "cmd.h"
#include "slow5_extra.h"
#include "read_fast5.h"
#include <slow5/slow5_press.h>


#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "\n" \
    "If no argument is given details about slow5tools is printed\n" \
    "OPTIONS:\n" \
    "    -h, -  -help         display this message and exit\n" \



int stats_main(int argc, char **argv, struct program_meta *meta){

    // Debug: print arguments
    if (meta != NULL && meta->verbosity_level >= LOG_DEBUG) {
        if (meta->verbosity_level >= LOG_VERBOSE) {
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

        fprintf(stdout, "slow5 library version\t%s\n", SLOW5_LIB_VERSION);

        std::string hdf5_environment = "";
        #ifdef HAVE_HDF5_SERIAL_HDF5_H
                hdf5_environment = "HAVE_HDF5_SERIAL_HDF5_H ";
        #endif
        #ifdef HAVE_HDF5_H
                hdf5_environment += "HAVE_HDF5_H ";
        #endif
        #ifdef HAVE_HDF5_HDF5_H
                hdf5_environment += "HAVE_HDF5_HDF5_H ";
        #endif
        #ifdef HAVE___HDF5_INCLUDE_HDF5_H
                hdf5_environment += "HAVE___HDF5_INCLUDE_HDF5_H ";
        #endif

        unsigned major, minor, release;
        H5get_libversion(&major, &minor, &release);
        fprintf(stdout, "hdf5 library version\t%u.%u.%u\n", major, minor, release);
        fprintf(stdout, "hdf5_macro_activated\t%s\n", hdf5_environment.c_str());
        //    free(&major);free(&minor);free(&release);H5dont_atexit();H5garbage_collect();H5close();
        return EXIT_SUCCESS;
    }

    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h' }, //0
            {NULL, 0, NULL, 0 }
    };

    // Input arguments
    int longindex = 0;
    int opt;

    // Parse options
    while ((opt = getopt_long(argc, argv, "h", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'h':
                if (meta->verbosity_level >= LOG_VERBOSE) {
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

    slow5_file_t* slow5File = slow5_open(argv[optind], "r");
    if(!slow5File){
        ERROR("cannot open %s. skipping...\n", argv[optind]);
        exit(EXIT_FAILURE);
    }
    uint32_t read_group_count_i = slow5File->header->num_read_groups;

    std::string file_format = "file format error";
    if(slow5File->format==SLOW5_FORMAT_UNKNOWN){
        file_format = "FORMAT_UNKNOWN";
    }else if(slow5File->format==SLOW5_FORMAT_ASCII){
        file_format = "FORMAT_ASCII";
    }else if(slow5File->format==SLOW5_FORMAT_BINARY){
        file_format = "FORMAT_BINARY";
    }

    std::string compression_method = "compression error";
    if(slow5File->compress->method==SLOW5_COMPRESS_NONE){
        compression_method = "SLOW5_COMPRESS_NONE";
    }else if(slow5File->compress->method==SLOW5_COMPRESS_GZIP){
        compression_method = "SLOW5_COMPRESS_GZIP";
    }

    unsigned int record_count = 0;
    struct slow5_rec *read = NULL;
    int ret;
    while ((ret = slow5_get_next(&read, slow5File)) >= 0) {
        record_count++;
    }
    slow5_rec_free(read);
    slow5_close(slow5File);

    fprintf(stdout, "file format\t%s\n", file_format.c_str());
    fprintf(stdout, "compression method\t%s\n", compression_method.c_str());
    fprintf(stdout,"number of read groups\t%u\n", read_group_count_i);
    fprintf(stdout,"number of records\t%u\n", record_count);

    return EXIT_SUCCESS;
}
