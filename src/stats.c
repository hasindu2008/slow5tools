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


#define USAGE_MSG "Usage: %s [SLOW5_FILE]\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "\n" \
    "If no argument is given details about slow5tools is printed\n" \
    "OPTIONS:\n" \
    "    -h, --help         display this message and exit\n" \


extern int slow5tools_verbosity_level;

int stats_main(int argc, char **argv, struct program_meta *meta){

    // Debug: print arguments
    print_args(argc,argv);

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
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'h':
                VERBOSE("displaying large help message%s","");
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
        file_format = "SLOW5 ASCII";
    }else if(slow5File->format==SLOW5_FORMAT_BINARY){
        file_format = "BLOW5";
    }

    fprintf(stdout, "file version\t%d.%d.%d\n",slow5File->header->version.major, slow5File->header->version.minor, slow5File->header->version.patch);


    /* TODO print separate information for record and signal compression */
    std::string compression_method = "compression error";
    if(slow5File->compress->record_press->method==SLOW5_COMPRESS_NONE){
        compression_method = "none";
    }else if(slow5File->compress->record_press->method==SLOW5_COMPRESS_ZLIB){
        compression_method = "zlib";
    }else if(slow5File->compress->record_press->method==SLOW5_COMPRESS_ZSTD){
        compression_method = "zstd";
    }

    fprintf(stdout, "file format\t%s\n", file_format.c_str());
    fprintf(stdout, "record compression method\t%s\n", compression_method.c_str());


    //todo: version check for signal_compression
    std::string signal_compression_method = "compression error";
    if(slow5File->compress->signal_press->method==SLOW5_COMPRESS_NONE){
        signal_compression_method = "none";
    }else if(slow5File->compress->signal_press->method==SLOW5_COMPRESS_SVB_ZD){
        signal_compression_method = "svb-zd";
    }
    fprintf(stdout, "sigal compression method\t%s\n", signal_compression_method.c_str());

    fprintf(stdout,"number of read groups\t%u\n", read_group_count_i);

    if(slow5File->header->aux_meta){
        fprintf(stdout, "number of auxiliary fields\t%d\n",slow5File->header->aux_meta->num);
        fprintf(stdout, "auxiliary fields\t");
        uint32_t num = slow5File->header->aux_meta->num;
        for(uint32_t i=0; i<num; i++){
            fprintf(stdout, "%s",slow5File->header->aux_meta->attrs[i]);
            if(i<num-1){
                fprintf(stdout,",");
            }
        }
        fprintf(stdout, "\n");
    }else{
        fprintf(stdout, "number of auxiliary fields\t%d\n",0);
        fprintf(stdout, "auxiliary fields\n");
    }

    VERBOSE("counting number of slow5 records...%s","");

    int64_t record_count = 0;
    size_t bytes;
    char *mem;
    double time_get_to_mem = slow5_realtime();
    while ((mem = (char *) slow5_get_next_mem(&bytes, slow5File))) {
        free(mem);
        record_count++;
    }
    if (slow5_errno != SLOW5_ERR_EOF) {
        ERROR("Error reading the file.%s","");
        return EXIT_FAILURE;
    }
    DEBUG("time_get_to_mem\t%.3fs", slow5_realtime()-time_get_to_mem);

    slow5_close(slow5File);

    fprintf(stdout,"number of records\t%" PRId64 "\n", record_count);

    return EXIT_SUCCESS;
}
