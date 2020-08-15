#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif

#define BT_BUF_SIZE (100)

// Segmentation fault handler
void sig_handler(int sig) {

    ERROR("I regret to inform that a segmentation fault occurred. But at least "
          "it is better than a wrong answer%s",
          ".");

#ifdef HAVE_EXECINFO_H
    void *buffer[BT_BUF_SIZE];
    int size = backtrace(buffer, BT_BUF_SIZE);
    fprintf(stderr,
            "[%s::DEBUG]\033[1;35m Here is the backtrace in case it is of any "
            "use:\n",
            __func__);
    backtrace_symbols_fd(&buffer[2], size - 1, STDERR_FILENO);
    fprintf(stderr, "\033[0m\n");
#endif

    exit(EXIT_FAILURE);
}
