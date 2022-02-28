/**
 * @file quickcheck.c
 * @brief performs a quick check of the header and if proper end of file exists in blow5
 * @author Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 26/08/2021
 */

#include <getopt.h>
#include "error.h"
#include "cmd.h"
#include "misc.h"
#include "slow5_extra.h"


#define USAGE_MSG "Usage: %s  [SLOW5_FILE]\n"
#define HELP_LARGE_MSG \
    "Performs a quick check if a SLOW5/BLOW5 file is intact. That is, quickcheck checks if the file begins with a valid header (SLOW5 or BLOW5) and then seeks to the end of the file and checks if proper EOF exists (BLOW5 only)." \
    "If the file is intact, the commands exists with 0. Otherwise exists with a non-zero error code.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help         display this message and exit\n" \

extern int slow5tools_verbosity_level;

int quickcheck_main(int argc, char **argv, struct program_meta *meta){

    // Debug: print arguments
    print_args(argc,argv);

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

    if (argc - optind < 1){
        ERROR("%s", "Not enough arguments");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        exit(EXIT_FAILURE);
    }

    if (argc - optind > 1){
        ERROR("%s", "Too many arguments");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        exit(EXIT_FAILURE);
    }


    slow5_file_t* slow5File = slow5_open(argv[optind], "r");
    if(!slow5File){
        ERROR("Opening %s failed\n", argv[optind]);
        exit(EXIT_FAILURE);
    }

    int ret_get_next = 0;
    slow5_rec_t *rec = NULL;
    if((ret_get_next=slow5_get_next(&rec,slow5File)) < 0){
        ERROR("%s does not have a proper slow5 record/format", argv[optind]);
        exit(EXIT_FAILURE);
    }
    slow5_rec_free(rec);

    if(slow5File->format==SLOW5_FORMAT_BINARY){
        if(fseek(slow5File->fp , 0, SEEK_END) !=0 ){
            ERROR("%s","Fseek to the end of the BLOW file failed.");
            exit(EXIT_FAILURE);
        }
        const char eof[] = SLOW5_BINARY_EOF;
        if(slow5_is_eof(slow5File->fp, eof, sizeof eof)!=1){
            ERROR("%s","No valid slow5 eof marker at the end of the BLOW5 file.");
            exit(EXIT_FAILURE);
        }
    }
    slow5_close(slow5File);

    return EXIT_SUCCESS;
}
