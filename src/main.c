/**
 * @file main.c
 * @brief main programme
 * @author Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 27/02/2021
 */

#include <getopt.h>
#include <signal.h>
#include <slow5/slow5.h>
#include "misc.h"
#include "error.h"
#include "cmd.h"

// TODO put all in header file

#define USAGE_MSG "Usage: %s [OPTION]... [COMMAND] [ARG]...\n"
#define SLOW5TOOLS_VERSION "%s " "0.1" "\n" // TODO change
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    USAGE_MSG \
    "Tools for using slow5 files.\n" \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help       Display this message and exit.\n" \
    "    -v, --verbose    Verbosity level.\n" \
    "    -V, --version    Output version information and exit.\n" \
    "\n" \
    "COMMANDS:\n" \
    "    f2s        convert fast5 file(s) to slow5\n" \
    "    s2f        convert slow5 file(s) to fast5\n" \
    "    merge      merge slow5 files\n" \
    "    split      split slow5 files\n" \
    "    index      create a slow5 or blow5 index file.\n" \
    "    get        display the read entry for each specified read id.\n" \
    "    view       TODO.\n" \
    "\n" \
    "ARGS:    Try '%s [COMMAND] --help' for more information.\n" \

// Backtrace buffer threshold of functions
#define BT_BUF_SIZE (100)
// Number of backtrace calls from the segmentation fault source to the handler
#define SEG_FAULT_BT_SIZE (2)
#define SEG_FAULT_MSG "I regret to inform that a segmentation fault occurred. " \
                      "But at least it is better than a wrong answer."

int (f2s_main)(int, char **, struct program_meta *);
int (s2f_main)(int, char **, struct program_meta *);
int (merge_main)(int, char **, struct program_meta *);
int (split_main)(int, char **, struct program_meta *);
int (index_main)(int, char **, struct program_meta *);
int (get_main)(int, char **, struct program_meta *);
int (view_main)(int, char **, struct program_meta *);

// Segmentation fault handler
void segv_handler(int sig) {

    ERROR(SEG_FAULT_MSG "%s", "");

#ifdef HAVE_EXECINFO_H
    void *buffer[BT_BUF_SIZE];
    int size = backtrace(buffer, BT_BUF_SIZE);
    NEG_CHK(size);
    fprintf(stderr, DEBUG_PREFIX "Here is the backtrace:\n",
            __FILE__, __func__, __LINE__);
    backtrace_symbols_fd(buffer + SEG_FAULT_BT_SIZE, size - SEG_FAULT_BT_SIZE,
                         STDERR_FILENO);
    fprintf(stderr, NO_COLOUR);
#endif

    // TODO add exit msg here?
    exit(EXIT_FAILURE);
}

int main(const int argc, char **argv){

    // Initial time
    double init_realtime = slow5_realtime();

    // Assume success
    int ret = EXIT_SUCCESS;

    // Default options
    struct program_meta meta = {
        .verbosity_level = LOG_VERBOSE
    };

    // Setup segmentation fault handler
    if (signal(SIGSEGV, segv_handler) == SIG_ERR) {
        WARNING("Segmentation fault signal handler failed to be setup.%s", "");
    }

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0], argv[0]);
        ret = EXIT_FAILURE;

    } else {
        const struct command cmds[] = {
            {"f2s", f2s_main},
            {"s2f", s2f_main},
            {"merge", merge_main},
            {"split", split_main},
            {"index", index_main},
            {"get", get_main},
            {"view", view_main},
        };
        const size_t num_cmds = sizeof (cmds) / sizeof (*cmds);

        static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h' },
            {"verbose", required_argument, NULL, 'v'},
            {"version", no_argument, NULL, 'V'},
            {NULL, 0, NULL, 0 }
        };

        int opt;
        bool break_flag = false;
        // Parse options up to first non-option argument (command)
        while (!break_flag && (opt = getopt_long(argc, argv, "+hVv:", long_opts, NULL)) != -1) {

            if (meta.verbosity_level >= LOG_DEBUG) {
                DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                      opt, optarg, optind, opterr, optopt);
            }

            switch (opt) {
                case 'h':
                    if (meta.verbosity_level >= LOG_GOSSIP) {
                        VERBOSE("displaying large help message%s","");
                    }
                    fprintf(stdout, HELP_LARGE_MSG, argv[0], argv[0]);
                    exit(EXIT_SUCCESS);
                    // ret = EXIT_SUCCESS;
                    // break_flag = true;
                    //break;
                case 'v':
                    meta.verbosity_level = atoi(optarg);
                    if (meta.verbosity_level >= LOG_GOSSIP) {
                        VERBOSE("printing the arguments given%s","");
                    }

                    if (meta.verbosity_level >= LOG_DEBUG) {
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
                    break;
                case 'V':
                    if (meta.verbosity_level >= LOG_GOSSIP) {
                        VERBOSE("displaying version information%s","");
                    }
                    fprintf(stdout, SLOW5TOOLS_VERSION, argv[0]);

                    ret = EXIT_SUCCESS;
                    break_flag = true;
                    break;

                default: // case '?'
                    fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                    ret = EXIT_FAILURE;
                    break_flag = true;
                    break;
            }
        }

        if (!break_flag) {

            // Reset optind for future use
            const int optind_copy = optind;
            optind = 0;

            // Parse command

            // There are remaining non-option arguments
            if (optind_copy < argc) {
                bool cmd_found = false;
                char *combined_name = NULL;
                char **cmd_argv;

                for (size_t i = 0; i < num_cmds; ++ i) {
                    if (strcmp(argv[optind_copy], cmds[i].name) == 0) {
                        cmd_found = true;

                        // Combining argv[0] and the command name

                        size_t argv_0_len = strlen(argv[0]);
                        size_t cmd_len = strlen(argv[optind_copy]);
                        size_t combined_len = argv_0_len + 1 + cmd_len + 1; // +2 for ' ' and '\0'

                        combined_name = (char *) malloc(combined_len * sizeof *combined_name);
                        MALLOC_CHK(combined_name);
                        memcpy(combined_name, argv[0], argv_0_len);
                        combined_name[argv_0_len] = ' ';
                        memcpy(combined_name + argv_0_len + 1, argv[optind_copy], cmd_len);
                        combined_name[combined_len - 1] = '\0';

                        // Creating new argv array for the command program
                        cmd_argv = (char **) malloc(argc * sizeof *cmd_argv);
                        memcpy(cmd_argv, argv, argc * sizeof *cmd_argv);
                        cmd_argv[optind_copy] = combined_name;

                        slow5_set_log_level((enum slow5_log_level_opt)meta.verbosity_level);
                        slow5_set_exit_condition(SLOW5_EXIT_ON_ERR);

                        // Calling command program
                        if (meta.verbosity_level >= LOG_GOSSIP) {
                            VERBOSE("using command '%s'", cmds[i].name);
                        }
                        ret = cmds[i].main(argc - optind_copy, cmd_argv + optind_copy, &meta);

                        break;
                    }
                }

                // No command found
                if (!cmd_found) {
                    MESSAGE(stderr, "invalid command -- '%s'", argv[optind_copy]);
                    fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                    ret = EXIT_FAILURE;

                } else {
                    free(combined_name);
                    combined_name = NULL;
                    free(cmd_argv);
                    cmd_argv = NULL;
                }

            // No remaining non-option arguments
            } else {
                MESSAGE(stderr, "missing command%s", "");
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                ret = EXIT_FAILURE;
            }
        }
    }

    if(ret==EXIT_SUCCESS){
        fprintf(stderr, "\n");
        if (meta.verbosity_level >= LOG_GOSSIP) {
            VERBOSE("printing command given%s", "");
        }
        fprintf(stderr, "cmd: ");
        for (int i = 0; i < argc; ++ i) {
            fprintf(stderr, "%s", argv[i]);
            if (i == argc - 1) {
                fprintf(stderr, "\n");
            } else {
                fprintf(stderr, " ");
            }
        }

        if (meta.verbosity_level >= LOG_GOSSIP) {
            VERBOSE("printing resource use%s", "");
        }
        MESSAGE(stderr, "real time = %.3f sec | CPU time = %.3f sec | peak RAM = %.3f GB",
                slow5_realtime() - init_realtime, slow5_cputime(), slow5_peakrss() / 1024.0 / 1024.0 / 1024.0);
    }

    EXIT_MSG(ret, argv, &meta);
    return ret;
}
