/* @slow5tools
**
** main
** @author: Hasindu Gamaarachchi (hasindu@garvan.org.au)
** @@
******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
//#include "slow5misc.h" // TODO uncomment
#include "slow5.h"
#include "error.h"

#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif

// TODO put all in header file
// TODO add verbose information

#define USAGE_MSG "Usage: %s [OPTION]... [COMMAND] [ARG]...\n"
#define VERSION_MSG "%s 0.0\n" // TODO change
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Tools for using slow5 files.\n" \
    "\n" \
    "COMMANDS:\n" \
    "    f2s - convert fast5 file(s) to slow5\n" \
    "    s2f - convert slow5 file(s) to fast5\n" \
    "\n" \
    "ARGS:\n" \
    "    Try '%s [COMMAND] --help' for more information.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -d, --debug\n" \
    "        Output debug information.\n" \
    "\n" \
    "    -h, --help\n" \
    "        Display this message and exit.\n" \
    "\n" \
    "    -v, --verbose\n" \
    "        Explain what is being done.\n" \
    "\n" \
    "    -V, --version\n" \
    "        Output version information and exit.\n"

// Backtrace buffer threshold of functions
#define BT_BUF_SIZE (100)
// Number of backtrace calls from the segmentation fault source to the handler
#define SEG_FAULT_BT_SIZE (2)
#define SEG_FAULT_MSG "I regret to inform that a segmentation fault occurred. " \
                      "But at least it is better than a wrong answer."

int (f2s_main)(int, char **, struct program_meta *);

// Segmentation fault handler
void segv_handler(int sig) {

    ERROR(SEG_FAULT_MSG "%s", "");

#ifdef HAVE_EXECINFO_H
    void *buffer[BT_BUF_SIZE];
    int size = backtrace(buffer, BT_BUF_SIZE);
    NEG_CHK(size);
    fprintf(stderr, DEBUG_PREFIX "Here is the backtrace:\n",
            __func__);
    backtrace_symbols_fd(buffer + SEG_FAULT_BT_SIZE, size - SEG_FAULT_BT_SIZE, 
                         STDERR_FILENO);
    fprintf(stderr, NO_COLOUR);
#endif

    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){

    //double realtime0 = realtime(); // TODO uncomment

    // Setup segmentation fault handler
    if (signal(SIGSEGV, segv_handler) == SIG_ERR) {
        WARNING("Segmentation fault signal handler failed to be setup.%s", "");
    }

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, USAGE_MSG HELP_SMALL_MSG, argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    static struct command cmds[] = {
        {"f2s", f2s_main}
    };
    static size_t num_cmds = sizeof (cmds) / sizeof (struct command);

    // Default options
    struct program_meta meta = {
        .debug = false,
        .verbose = false
    };

    static struct option long_opts[] = {
        {"debug", no_argument, NULL, 'd' },
        {"help", no_argument, NULL, 'h' },
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0 }
    };

    char opt;
    // Parse options up to first non-option argument (command)
    while ((opt = getopt_long(argc, argv, "+dhvV", long_opts, NULL)) != -1) {
        switch (opt) {

            case 'd':
                // Print arguments
                if (!meta.debug) {
                    fprintf(stderr, DEBUG_PREFIX "argv=[",
                            argv[0], __FILE__, __func__, __LINE__);
                    for (int i = 0; i < argc; ++ i) {
                        fprintf(stderr, "\"%s\"", argv[i]);
                        if (i == argc - 1) {
                            fprintf(stderr, "]");
                        } else {
                            fprintf(stderr, ", ");
                        }
                    }
                    fprintf(stderr, NO_COLOUR);

                    meta.debug = true;
                }

                break;
            case 'h':
                fprintf(stdout, HELP_LARGE_MSG, argv[0], argv[0]);
                return EXIT_SUCCESS;
            case 'v':
                meta.verbose = true;
                break;
            case 'V':
                fprintf(stdout, VERSION_MSG, argv[0]);
                return EXIT_SUCCESS;

            default: // case '?' 
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                return EXIT_FAILURE;
        }
    }
    int optind_copy = optind;
    optind = 0;


    // Parse command

    // There are remaining non-option arguments
    if (optind_copy < argc) {
        bool cmd_found = false;
        char *combined_name = NULL;
        int cmd_ret = -1;

        for (size_t i = 0; i < num_cmds; ++ i) {
            if (strcmp(argv[optind_copy], cmds[i].name) == 0) {
                cmd_found = true;
                
                // Combining argv[0] and the command name
                // TODO this can be made quicker by putting argv[0] before command name and using both in command program
                // but then it's less neat and modular
                
                size_t argv_0_len = strlen(argv[0]);
                size_t cmd_len = strlen(argv[optind_copy]);
                size_t combined_len = argv_0_len + 1 + cmd_len + 1; // +2 for ' ' and '\0'

                combined_name = malloc(combined_len * sizeof *combined_name);
                MALLOC_CHK(combined_name);
                memcpy(combined_name, argv[0], argv_0_len);
                combined_name[argv_0_len] = ' ';
                memcpy(combined_name + argv_0_len + 1, argv[optind_copy], cmd_len);
                combined_name[combined_len - 1] = '\0';
                argv[optind_copy] = combined_name;

                // Calling command program
                cmd_ret = cmds[i].main(argc - optind_copy, argv + optind_copy, &meta);
            }
        }

        // No command found
        if (!cmd_found) {
            MESSAGE("invalid command -- '%s'", argv[optind_copy]);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);
            return EXIT_FAILURE;

        } else {
            free(combined_name);
            combined_name = NULL;
            return cmd_ret;
        }

    // No remaining non-option arguments
    } else {
        MESSAGE("missing command%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        return EXIT_FAILURE;
    }



    /* TODO uncomment

    int ret=1;

    if(argc<2){
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
    }
    else if(strcmp(argv[1],"fastt")==0){
        ret=fastt_main(argc-1, argv+1);
	}
    else if(strcmp(argv[1],"--version")==0 || strcmp(argv[1],"-V")==0){
        fprintf(stdout,"slow5tools %s\n",SLOW5_VERSION);
        exit(EXIT_SUCCESS);
    }
    else if(strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0){
        print_usage(stdout);
    }
    else{
        fprintf(stderr,"[slow5tools] Unrecognised command %s\n",argv[1]);
        print_usage(stderr);
    }

    fprintf(stderr, "\n[%s] CMD:", __func__);
    for (int i = 0; i < argc; ++i) {
        fprintf(stderr, " %s", argv[i]);
    }

    fprintf(stderr, "\n[%s] Real time: %.3f sec; CPU time: %.3f sec; Peak RAM: %.3f GB\n\n",
            __func__, realtime() - realtime0, cputime(),peakrss() / 1024.0 / 1024.0 / 1024.0);

    return ret;
    */
}
