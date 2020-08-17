// Sasha Jenner

#include "error.h"
#include <signal.h>
#include <unistd.h> #include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

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
    "    -d, --max-depth=[NUM]\n" \
    "        Sets the maximum depth to search directories for fast5 files.\n" \
    "        NUM must be a non-negative integer.\n" \
    "        Default: No maximum depth.\n" \
    "\n" \
    "        E.g. NUM=1: Reads the files within a specified directory but\n" \
    "        not those within subdirectories.\n" \
    "\n" \
    "    -h, --help\n" \
    "        Prints this message.\n" \
    "\n" \
    "    -o, --output=[SLOW5_FILE]\n" \
    "        Outputs slow5 contents to SLOW5_FILE.\n" \
    "        Default: Stdout.\n" \
    "\n" \
    "    -v, --verbose\n" \
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

    // Default options
    long max_depth = -1;
    int fd_out = STDOUT_FILENO;
    bool verbose = false;

    // Input arguments
    char *arg_max_depth = NULL;
    char *arg_fname_out = NULL;

    char opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "d:ho:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                arg_max_depth = optarg;
                break;
            case 'h':
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                return EXIT_SUCCESS;
            case 'o':
                arg_fname_out = optarg; break; case 'v':
                verbose = true;
                break;
            default: // case '?' 
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Parse maximum depth argument
    if (arg_max_depth != NULL) {

        // Check it is a number
        
        // Cannot be empty 
        size_t arg_len = strlen(arg_max_depth);
        if (arg_len == 0) {
            MESSAGE("invalid max depth -- '%s'", arg_max_depth);
            fprintf(stderr, HELP_SMALL_MSG, argv[0]);
            return EXIT_FAILURE;
        }

        for (size_t i = 0; i < arg_len; ++ i) {
            // Not a digit and first char is not a '+'
            if (!isdigit((unsigned char) arg_max_depth[i]) && 
                    !(i == 0 && arg_max_depth[i] == '+')) {
                MESSAGE("invalid max depth -- '%s'", arg_max_depth);
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                return EXIT_FAILURE;
            }
        }

        // Parse argument
        max_depth = strtol(arg_max_depth, NULL, 10);
        // Check for overflow
        if (errno == ERANGE) {
            WARNING("Overflow of max depth '%s'. Setting to %ld instead.", 
                    arg_max_depth, max_depth);
        }
    }

    // Parse output argument
    if (arg_fname_out != NULL) { 

        // Create new file or
        // Truncate existing file
        // 664 permissions
        int new_fd = open(arg_fname_out, O_CREAT|O_WRONLY|O_TRUNC,
                          S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH);

        // An error occured
        if (new_fd == -1) {
            ERROR("File '%s' could not be opened - %s.", 
                  arg_fname_out, strerror(errno));
            return EXIT_FAILURE;
            
        } else {
            fd_out = new_fd;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE("expected fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        return EXIT_FAILURE;
    }


    // Do the converting




    if (fd_out != STDOUT_FILENO) {
        // Close output file
        if (close(fd_out) == -1) {
            ERROR("File '%s' failed on closing - %s.",
                  arg_fname_out, strerror(errno));
        } 
    }

    return EXIT_SUCCESS;
}
