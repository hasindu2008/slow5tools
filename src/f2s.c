/**
 * @file f2s.c
 * @brief fast5 to slow5 conversion
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au) Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 27/02/2021
 */
#include <getopt.h>
#include <sys/wait.h>

#include <string>
#include <vector>

#include <slow5/slow5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "error.h"
#include "cmd.h"
#include "slow5_extra.h"
#include "read_fast5.h"
#include "misc.h"

#define USAGE_MSG "Usage: %s [OPTION]... [FAST5_FILE/DIR]...\n"
#define HELP_LARGE_MSG \
    "Convert FAST5 files to SLOW5/BLOW5 format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    HELP_MSG_OUTPUT_FORMAT \
    HELP_MSG_OUTPUT_FILE                           \
    HELP_MSG_PRESS \
    HELP_MSG_PROCESSES \
    HELP_MSG_LOSSLESS \
    HELP_MSG_OUTPUT_DIRECTORY \
    "    -a, --allow                   allow run id mismatches in a multi-fast5 file or in a single-fast5 directory\n" \
    "    -h, --help                    display this message and exit\n" \
    HELP_FORMATS_METHODS

extern int slow5tools_verbosity_level;

// what a child process should do, i.e. open a tmp file, go through the fast5 files
void f2s_child_worker(opt_t *user_opts, std::vector<std::string>& fast5_files, reads_count* readsCount, proc_arg_t args){
    int ret = 0;
    static size_t call_count = 0;
    slow5_file_t* slow5File = NULL;
    slow5_file_t* slow5File_outputdir_single_fast5 = NULL;
    FILE *slow5_file_pointer = NULL;
    FILE *slow5_file_pointer_outputdir_single_fast5 = NULL;
    std::string slow5_path;
    std::string slow5_path_outputdir_single_fast5;
    std::unordered_map<std::string, uint32_t> warning_map;
    std::string extension = ".blow5";
    char *output_dir = user_opts->arg_dir_out;
    if(user_opts->fmt_out==SLOW5_FORMAT_ASCII){
        extension = ".slow5";
    }
    if(output_dir){
        slow5_path = std::string(output_dir);
        slow5_path_outputdir_single_fast5 = slow5_path;
    }
    fast5_file_t fast5_file;
    for (int i = args.starti; i < args.endi; i++) {
        readsCount->total_5++;
        fast5_file = fast5_open(fast5_files[i].c_str());
        fast5_file.fast5_path = fast5_files[i].c_str();

        if (fast5_file.hdf5_file < 0){
            WARNING("Fast5 file '%s' is unreadable and will be skipped.", fast5_files[i].c_str());
            H5Fclose(fast5_file.hdf5_file);
            readsCount->bad_5_file++;
            continue;
        }
        if(output_dir){
            if (fast5_file.is_multi_fast5) {
                std::string slow5file = fast5_files[i].substr(fast5_files[i].find_last_of('/'),
                                                              fast5_files[i].length() -
                                                              fast5_files[i].find_last_of('/') - extension.length()) + extension;
                slow5_path += slow5file;
                //fprintf(stderr,"slow5path = %s\n fast5_path = %s\nslow5file = %s\n",slow5_path.c_str(), fast5_files[i].c_str(),slow5file.c_str());

                slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                // An error occured
                if (!slow5_file_pointer) {
                    ERROR("File '%s' could not be opened for writing. %s.",
                          slow5_path.c_str(), strerror(errno));
                    continue;
                }
                slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), SLOW5_FORMAT_ASCII);
                if (slow5File == NULL){
                    ERROR("%s","Could not initialise the slow5lib data structure.");
                    exit(EXIT_FAILURE);
                }

                ret = slow5_hdr_initialize(slow5File->header, user_opts->flag_lossy);
                if(ret < 0){
                    ERROR("%s","Could not initialise the SLOW5 header.");
                    exit(EXIT_FAILURE);
                }
                ret = read_fast5(user_opts, &fast5_file, slow5File, 0, &warning_map);
                if(ret < 0){
                    ERROR("Could not read contents of the fast5 file '%s'.", fast5_files[i].c_str());
                    exit(EXIT_FAILURE);
                }
                if(user_opts->fmt_out == SLOW5_FORMAT_BINARY){
                    if (slow5_eof_fwrite(slow5File->fp) < 0){
                        ERROR("Could write the BLOW5 end of file marker in '%s'.", slow5_path.c_str());
                        exit(EXIT_FAILURE);
                    }
                }
                slow5_close(slow5File);
                slow5_path = std::string(output_dir);

            }else{ // single-fast5
                if(!slow5_file_pointer_outputdir_single_fast5){
                    slow5_path_outputdir_single_fast5 += "/"+std::to_string(args.starti)+extension;
                    slow5_file_pointer_outputdir_single_fast5 = fopen(slow5_path_outputdir_single_fast5.c_str(), "w");
                    // An error occured
                    if (!slow5_file_pointer_outputdir_single_fast5) {
                        ERROR("File '%s' could not be opened for writing. %s.",
                              slow5_path_outputdir_single_fast5.c_str(), strerror(errno));
                        continue;
                    }
                    slow5File_outputdir_single_fast5 = slow5_init_empty(slow5_file_pointer_outputdir_single_fast5, slow5_path_outputdir_single_fast5.c_str(), SLOW5_FORMAT_BINARY);
                    if (slow5File_outputdir_single_fast5 == NULL){
                        ERROR("%s","Could not initialise the slow5lib data structure.");
                        exit(EXIT_FAILURE);
                    }
                    ret = slow5_hdr_initialize(slow5File_outputdir_single_fast5->header, user_opts->flag_lossy);
                    if(ret < 0){
                        ERROR("%s","Could not initialise the SLOW5 header.");
                        exit(EXIT_FAILURE);
                    }
                }
                ret = read_fast5(user_opts, &fast5_file, slow5File_outputdir_single_fast5, call_count++, &warning_map);
                if(ret<0){
                    ERROR("Could not read contents of the fast5 file '%s'.", fast5_files[i].c_str());
                    exit(EXIT_FAILURE);
                }
            }
        }
        else{ // output dir not set hence, writing to file/stdout
            if(call_count==0){
                slow5_path = "stdout";
                if(user_opts->arg_fname_out){
//                    slow5_path = user_opts->arg_fname_out;
                    slow5_file_pointer = fopen(user_opts->arg_fname_out, "wb");
                    if (!slow5_file_pointer) {
                        ERROR("Output file %s could not be opened for writing. %s.", user_opts->arg_fname_out, strerror(errno));
                        return;
                    }
                }else{
                    slow5_file_pointer = fdopen(1,"w");  //obtain a pointer to stdout file stream
                    if (!slow5_file_pointer) {
                        ERROR("Could not open the stdout file stream. %s.", strerror(errno));
                        return;
                    }
                }
                slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), SLOW5_FORMAT_BINARY);
                if (slow5File == NULL){
                        ERROR("%s","Could not initialise the slow5lib data structure.");
                        exit(EXIT_FAILURE);
                }
                ret = slow5_hdr_initialize(slow5File->header, user_opts->flag_lossy);
                if(ret<0){
                    ERROR("%s","Could not initialise the SLOW5 header.");
                    exit(EXIT_FAILURE);
                }
            }
            ret = read_fast5(user_opts, &fast5_file, slow5File, call_count++, &warning_map);
            if(ret<0){
                ERROR("Could not read contents of the fast5 file '%s'.", fast5_files[i].c_str());
                exit(EXIT_FAILURE);
            }
        }
        H5Fclose(fast5_file.hdf5_file);
    }

    if(slow5File_outputdir_single_fast5 && slow5_file_pointer_outputdir_single_fast5) {
        if(user_opts->fmt_out == SLOW5_FORMAT_BINARY){
            if(slow5_eof_fwrite(slow5File_outputdir_single_fast5->fp) < 0){
                ERROR("Could write the BLOW5 end of file marker in '%s'.", slow5_path.c_str());
                exit(EXIT_FAILURE);
            }
        }
        slow5_close(slow5File_outputdir_single_fast5);
    }
    if(slow5File && !output_dir) {
        if(user_opts->fmt_out == SLOW5_FORMAT_BINARY){
            if(slow5_eof_fwrite(slow5File->fp) < 0){
                ERROR("Could write the BLOW5 end of file marker in '%s'.", slow5_path.c_str());
                exit(EXIT_FAILURE);
            }
        }
        slow5_close(slow5File); //if stdout was used stdout is now closed.
    }
    INFO("Summary - total fast5: %lu, bad fast5: %lu\n", readsCount->total_5, readsCount->bad_5_file);
}

void f2s_iop(opt_t *user_opts, std::vector<std::string>& fast5_files, reads_count* readsCount){
    int32_t num_fast5_files = fast5_files.size();
    int32_t iop = user_opts->num_processes;
    if (iop > num_fast5_files) {
        iop = num_fast5_files;
        user_opts->num_processes = iop;
    }
    VERBOSE("%zu proceses will be used.",user_opts->num_processes);

    //create processes
//    pid_t pids[iop];
//    proc_arg_t proc_args[iop];
    pid_t* pids = (pid_t*) malloc(iop*sizeof(pid_t));
    proc_arg_t* proc_args = (proc_arg_t*)malloc(iop*sizeof(proc_arg_t));
    MALLOC_CHK(pids);
    MALLOC_CHK(proc_args);

    int32_t t;
    int32_t i = 0;
    int32_t step = (num_fast5_files + iop - 1) / iop;
    //todo : check for higher num of procs than the data
    //current works but many procs are created despite

    //set the data structures
    for (t = 0; t < iop; t++) {
        proc_args[t].starti = i;
        i += step;
        if (i > num_fast5_files) {
            proc_args[t].endi = num_fast5_files;
        } else {
            proc_args[t].endi = i;
        }
        proc_args[t].proc_index = t;
    }

    if(iop==1){
        f2s_child_worker(user_opts, fast5_files,readsCount, proc_args[0]);
        free(proc_args);
        free(pids);
        return;
    }

    //create processes
    VERBOSE("Spawning %d I/O processes to circumvent HDF hell.", iop);
    for(t = 0; t < iop; t++){
        pids[t] = fork();

        if(pids[t]==-1){
            ERROR("%s","Forking processes failed.");
            perror("");
            exit(EXIT_FAILURE);
        }
        if(pids[t]==0){ //child
            f2s_child_worker(user_opts, fast5_files, readsCount,  proc_args[t]);
            exit(EXIT_SUCCESS);
        }
        if(pids[t]>0){ //parent
            continue;
        }
    }

    //wait for processes
    int status,w;
    for (t = 0; t < iop; t++) {
//        if(opt::verbose>1){
//            STDERR("parent : Waiting for child with pid %d",pids[t]);
//        }
        w = waitpid(pids[t], &status, 0);
        if (w == -1) {
            ERROR("%s","Waitpid failed.");
            perror("");
            exit(EXIT_FAILURE);
        }
        else if (WIFEXITED(status)){
//            if(opt::verbose>1){
//                STDERR("child process %d exited, status=%d", pids[t], WEXITSTATUS(status));
//            }
            if(WEXITSTATUS(status)!=0){
                VERBOSE("Child process %d exited with status=%d.",pids[t], WEXITSTATUS(status));
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (WIFSIGNALED(status)) {
                VERBOSE("child process %d killed by signal %d", pids[t], WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                VERBOSE("child process %d stopped by signal %d", pids[t], WSTOPSIG(status));
            } else {
                VERBOSE("child process %d did not exit propoerly: status %d", pids[t], status);
            }
            exit(EXIT_FAILURE);
        }
    }
    free(proc_args);
    free(pids);
}

int f2s_main(int argc, char **argv, struct program_meta *meta) {

    // Turn off HDF's exception printing, which is generally unhelpful for users
    // This can cause a 'still reachable' memory leak on a valgrind check
    H5Eset_auto(0, NULL, NULL);

    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    // Default options
    static struct option long_opts[] = {
            {"to",          required_argument, NULL, 'b'},  //0
            {"compress",    required_argument, NULL, 'c'},  //1
            {"sig-compress",required_argument,  NULL, 's'}, //2
            {"help",        no_argument, NULL, 'h'},        //3
            {"output",      required_argument, NULL, 'o'},  //4
            { "iop",        required_argument, NULL, 'p'},  //5
            { "lossless",   required_argument, NULL, 'l'},  //6
            { "out-dir",    required_argument, NULL, 'd'},  //7
            { "allow",      no_argument, NULL, 'a'},        //8
            { "dump-all",   required_argument, NULL, 'e'},        //9
            {NULL, 0, NULL, 0 }
    };

    opt_t user_opts;
    init_opt(&user_opts);

    int opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "b:c:s:ho:p:l:d:ae:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'b':
                user_opts.arg_fmt_out = optarg;
                break;
            case 'c':
                user_opts.arg_record_press_out = optarg;
                break;
            case 's':
                user_opts.arg_signal_press_out = optarg;
                break;
            case 'l':
                user_opts.arg_lossless = optarg;
                break;
            case 'a':
                user_opts.flag_allow_run_id_mismatch = 1;
                break;
            case 'h':
                DEBUG("Displaying the large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'd':
                user_opts.arg_dir_out = optarg;
                break;
            case 'p':
                user_opts.arg_num_processes = optarg;
                break;
            case 'o':
                user_opts.arg_fname_out = optarg;
                break;
            case 'e':
                user_opts.arg_dump_all = optarg;
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }
    if(parse_num_processes(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_arg_lossless(&user_opts, argc, argv, meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_arg_dump_all(&user_opts, argc, argv, meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_format_args(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(auto_detect_formats(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_compression_opts(&user_opts) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(user_opts.arg_fname_out && user_opts.arg_dir_out){
        ERROR("Both output file name (-o) and output directory (-d) cannot be set simultaneously. %s","");
        return EXIT_FAILURE;
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        ERROR("%s", "Not enough arguments. Enter one or more fast5 files or directories as arguments.");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    //measure file listing time
    double init_realtime = slow5_realtime();
    std::vector<std::string> fast5_files;
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], fast5_files, 0, ".fast5");
    }
    VERBOSE("%ld fast5 files found - took %.3fs",fast5_files.size(), slow5_realtime() - init_realtime);
    if(fast5_files.size()==0){
        ERROR("No fast5 files found. Exiting.%s","");
        return EXIT_FAILURE;
    }
    if(fast5_files.size()==1){
        user_opts.num_processes = 1;
    }
    if(user_opts.num_processes>1 && !user_opts.arg_dir_out){
        ERROR("An output directory (-d) must be specified when requesting more than one I/O process. %s","");
        return EXIT_FAILURE;
    }
    if(user_opts.arg_dir_out){
        struct stat st = {0};
        if (stat(user_opts.arg_dir_out, &st) == -1) {
            mkdir(user_opts.arg_dir_out, 0700);
        }else{
            std::vector< std::string > dir_list = list_directory(user_opts.arg_dir_out);
            if(dir_list.size()>2){
                ERROR("Output directory %s is not empty. Please remove it or specify another directory.",user_opts.arg_dir_out);
                return EXIT_FAILURE;
            }
        }
    }

    //measure fast5 conversion time
    init_realtime = slow5_realtime();

    reads_count readsCount;
    f2s_iop(&user_opts, fast5_files, &readsCount);
    VERBOSE("Converting %ld fast5 files took %.3fs",fast5_files.size(), slow5_realtime() - init_realtime);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
