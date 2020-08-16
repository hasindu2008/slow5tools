#include "error.h"
#include <signal.h>
#include <unistd.h>

#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif

#define USAGE_MSG \
    "Convert fast5 file(s) to slow5.\n" \
    "\n" \
    "Usage: fast5toslow5 [OPTIONS] [FAST5_FILE/DIR_1 ...]\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -d [NUM], --max-depth=[NUM]\n" \
    "        Sets the maximum depth to search directories for fast5 files.\n" \
    "        Default: No maximum depth.\n" \
    "        E.g. NUM=1: Reads the files within a specified directory but\n" \
    "        not those within subdirectories.\n" \
    "\n" \
    "    -h|--help\n" \
    "        Prints helpful message.\n" \
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
    backtrace_symbols_fd(buffer + SEG_FAULT_BT_SIZE, size - SEG_FAULT_BT_SIZE, STDERR_FILENO);
    fprintf(stderr, NO_COLOUR);
#endif

    exit(EXIT_FAILURE);
}

// Output usage to file
void print_usage(FILE *fp_help) {
    fprintf(fp_help, USAGE_MSG);
}

int main(int argc, char **argv) {
    // Setup segmentation fault handler
    if (signal(SIGSEGV, segv_handler) == SIG_ERR) {
        WARNING("Segmentation fault signal handler failed to be setup.%s", "");
    }

    return EXIT_SUCCESS;
}
