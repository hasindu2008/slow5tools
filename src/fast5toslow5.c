#include "error.h"
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif

#define USAGE_MSG "Usage: %s [OPTION]... [FAST5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Convert fast5 file(s) to slow5.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -d|--max-depth [NUM]\n" \
    "        Sets the maximum depth to search directories for fast5 files.\n" \
    "        Default: No maximum depth.\n" \
    "        E.g. NUM=1: Reads the files within a specified directory but\n" \
    "        not those within subdirectories.\n" \
    "\n" \
    "    -h|--help\n" \
    "        Prints this message.\n" \
    "\n" \
    "    -o|--output [SLOW5_FILE]\n" \
    "        Outputs slow5 contents to SLOW5_FILE.\n" \
    "        Default: Stdout.\n" \
    "\n" \
    "    -v|--verbose\n" \
    "        Output more information while converting.\n"

// Backtrace buffer threshold of functions
#define BT_BUF_SIZE (100)
// Number of backtrace calls from the segmentation fault source to the handler
#define SEG_FAULT_BT_SIZE (2)
#define SEG_FAULT_MSG "I regret to inform that a segmentation fault occurred. " \
                      "But at least it is better than a wrong answer."


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

int main(int argc, char **argv) {
    // Setup segmentation fault handler
    if (signal(SIGSEGV, segv_handler) == SIG_ERR) {
        WARNING("Segmentation fault signal handler failed to be setup.%s", "");
    }

    static struct option long_options[] = {
        {"max-depth", required_argument, NULL, 'd' },
        {"help", no_argument, NULL, 'h' },
        {"output", required_argument, NULL, 'o' },
        {"verbose", no_argument, NULL, 'v' },
        {NULL, 0, NULL, 0 }
    };

    int max_depth = -1;
    FILE *f_out = stdout;
    bool verbose = false;

    char *arg_max_depth = NULL;
    char *arg_fname_out = NULL;

    char opt;
    while ((opt = getopt_long(argc, argv, "d:ho:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                arg_max_depth = optarg;
                break;
            case 'h':
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                return EXIT_SUCCESS;
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            default: // case '?' 
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE("expected fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        return EXIT_FAILURE;
    }
    
    // Parse maximum depth argument
    if (arg_max_depth != NULL) {
          
    }

    return EXIT_SUCCESS;
}
