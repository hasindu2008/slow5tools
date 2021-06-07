//
// Sasha Jenner
// Hiruna Samarakoon
//
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
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Convert FAST5 files to SLOW5/BLOW5 format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    --to [format_type]                 output in the format specified in STR. slow5 for SLOW5 ASCII. blow5 for SLOW5 binary (BLOW5) [default: BLOW5]\n" \
    "    -c, --compress [compression_type]  convert to compressed blow5. [default: gzip]\n" \
    "    -d, --out-dir [STR]                output directory where slow5files are written to\n" \
    "    -o, --output [FILE]                output contents to FILE [default: stdout]\n" \
    "    -p, --iop [INT]                    number of I/O processes to read fast5 files [default: 8]\n" \
    "    -l, --lossless [STR]               retain information in auxilliary fields during the conversion.[default: true].\n" \
    "    -a, --allow                        allow run id mismatches in a multi-fast5 file or in a single-fast5 directory\n" \
    "    -h, --help                         display this message and exit\n" \

static double init_realtime = 0;

// what a child process should do, i.e. open a tmp file, go through the fast5 files
void f2s_child_worker(enum slow5_fmt format_out, enum press_method pressMethod, int lossy, int flag_allow_run_id_mismatch, proc_arg_t args, std::vector<std::string>& fast5_files, char* output_dir, struct program_meta *meta, reads_count* readsCount, char* arg_fname_out){
    static size_t call_count = 0;
    slow5_file_t* slow5File = NULL;
    slow5_file_t* slow5File_outputdir_single_fast5 = NULL;
    FILE *slow5_file_pointer = NULL;
    FILE *slow5_file_pointer_outputdir_single_fast5 = NULL;
    std::string slow5_path;
    std::string slow5_path_outputdir_single_fast5;

    khash_t(warncount) *warncount_hash = kh_init(warncount);  // allocate a hash table

    std::string extension = ".blow5";
    if(format_out==FORMAT_ASCII){
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
            WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_files[i].c_str());
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
                    ERROR("File '%s' could not be opened - %s.",
                          slow5_path.c_str(), strerror(errno));
                    continue;
                }
                slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), FORMAT_ASCII);
                slow5_hdr_initialize(slow5File->header, lossy);
                read_fast5(&fast5_file, format_out, pressMethod, lossy, 0, flag_allow_run_id_mismatch, meta, slow5File,
                           &warncount_hash);

                if(format_out == FORMAT_BINARY){
                    slow5_eof_fwrite(slow5File->fp);
                }
                slow5_close(slow5File);
                slow5_path = std::string(output_dir);

            }else{ // single-fast5
                if(!slow5_file_pointer_outputdir_single_fast5){
                    slow5_path_outputdir_single_fast5 += "/"+std::to_string(args.starti)+extension;
                    slow5_file_pointer_outputdir_single_fast5 = fopen(slow5_path_outputdir_single_fast5.c_str(), "w");
                    // An error occured
                    if (!slow5_file_pointer_outputdir_single_fast5) {
                        ERROR("File '%s' could not be opened - %s.",
                              slow5_path_outputdir_single_fast5.c_str(), strerror(errno));
                        continue;
                    }
                    slow5File_outputdir_single_fast5 = slow5_init_empty(slow5_file_pointer_outputdir_single_fast5, slow5_path_outputdir_single_fast5.c_str(), FORMAT_BINARY);
                    slow5_hdr_initialize(slow5File_outputdir_single_fast5->header, lossy);
                }
                read_fast5(&fast5_file, format_out, pressMethod, lossy, call_count++, flag_allow_run_id_mismatch, meta,
                           slow5File_outputdir_single_fast5, &warncount_hash);
            }
        }
        else{ // output dir not set hence, writing to stdout
            if(call_count==0){
                if(arg_fname_out){
                    slow5_file_pointer = fopen(arg_fname_out, "wb");
                    if (!slow5_file_pointer) {
                        ERROR("Output file %s could not be opened - %s.", arg_fname_out, strerror(errno));
                        return;
                    }
                }else{
                    slow5_file_pointer = fdopen(1,"w");  //obtain a pointer to stdout file stream
                    if (!slow5_file_pointer) {
                        ERROR("Could not open stdout file stream - %s.", strerror(errno));
                        return;
                    }
                }
                slow5File = slow5_init_empty(slow5_file_pointer, slow5_path.c_str(), FORMAT_BINARY);
                slow5_hdr_initialize(slow5File->header, lossy);
            }
            read_fast5(&fast5_file, format_out, pressMethod, lossy, call_count++, flag_allow_run_id_mismatch, meta,
                       slow5File, &warncount_hash);
        }
        H5Fclose(fast5_file.hdf5_file);
    }
    if(slow5_file_pointer_outputdir_single_fast5) {
        if(format_out == FORMAT_BINARY){
            slow5_eof_fwrite(slow5File_outputdir_single_fast5->fp);
        }
        slow5_close(slow5File_outputdir_single_fast5);
    }
    if(!output_dir) {
        if(format_out == FORMAT_BINARY){
            slow5_eof_fwrite(slow5File->fp);
        }
        slow5_close(slow5File); //if stdout was used stdout is now closed.
    }
    kh_destroy(warncount, warncount_hash);              // deallocate the hash table
    if(meta->verbosity_level >= LOG_VERBOSE){
        fprintf(stderr, "The processed - total fast5: %lu, bad fast5: %lu\n", readsCount->total_5, readsCount->bad_5_file);
    }
}

void f2s_iop(enum slow5_fmt format_out, enum press_method pressMethod, int lossy, int flag_allow_run_id_mismatch, int iop, std::vector<std::string>& fast5_files, char* output_dir, struct program_meta *meta, reads_count* readsCount, char* arg_fname_out){
    int64_t num_fast5_files = fast5_files.size();
    if (iop > num_fast5_files) {
        iop = num_fast5_files;
        WARNING("Only %d proceses will be used",iop);
    }

    //create processes
    pid_t pids[iop];
    proc_arg_t proc_args[iop];

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
        f2s_child_worker(format_out, pressMethod, lossy, flag_allow_run_id_mismatch, proc_args[0], fast5_files, output_dir, meta, readsCount, arg_fname_out);
//        goto skip_forking;
        return;
    }

    //create processes
    STDERR("Spawning %d I/O processes to circumvent HDF hell", iop);
    for(t = 0; t < iop; t++){
        pids[t] = fork();

        if(pids[t]==-1){
            ERROR("%s","Fork failed");
            perror("");
            exit(EXIT_FAILURE);
        }
        if(pids[t]==0){ //child
            f2s_child_worker(format_out, pressMethod, lossy, flag_allow_run_id_mismatch, proc_args[t], fast5_files, output_dir, meta, readsCount, arg_fname_out);
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
            ERROR("%s","waitpid failed");
            perror("");
            exit(EXIT_FAILURE);
        }
        else if (WIFEXITED(status)){
//            if(opt::verbose>1){
//                STDERR("child process %d exited, status=%d", pids[t], WEXITSTATUS(status));
//            }
            if(WEXITSTATUS(status)!=0){
                ERROR("child process %d exited with status=%d",pids[t], WEXITSTATUS(status));
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (WIFSIGNALED(status)) {
                ERROR("child process %d killed by signal %d", pids[t], WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                ERROR("child process %d stopped by signal %d", pids[t], WSTOPSIG(status));
            } else {
                ERROR("child process %d did not exit propoerly: status %d", pids[t], status);
            }
            exit(EXIT_FAILURE);
        }
    }
    return;
}

int f2s_main(int argc, char **argv, struct program_meta *meta) {

    // Turn off HDF's exception printing, which is generally unhelpful for users
    H5Eset_auto(0, NULL, NULL);

    int iop = 8;
    int lossy = 0;
    int flag_allow_run_id_mismatch = 0;

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
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    // Default options
    static struct option long_opts[] = {
            {"to", required_argument, NULL, 'b'},    //0
            {"compress", required_argument, NULL, 'c'},  //1
            {"help", no_argument, NULL, 'h'},  //2
            {"output", required_argument, NULL, 'o'},   //3
            { "iop", required_argument, NULL, 'p'}, //4
            { "lossless", required_argument, NULL, 'l'}, //4
            { "out-dir", required_argument, NULL, 'd'}, //5
            { "allow", no_argument, NULL, 'a'}, //6
            {NULL, 0, NULL, 0 }
    };

    enum slow5_fmt format_out = FORMAT_BINARY;
    enum press_method pressMethod = COMPRESS_GZIP;

    // Input arguments
    char *arg_dir_out = NULL;
    char *arg_fname_out = NULL;

    int opt;
    int longindex = 0;
    // Parse options
    while ((opt = getopt_long(argc, argv, "c:hb:o:d:l:ap:", long_opts, &longindex)) != -1) {
        if (meta->verbosity_level >= LOG_DEBUG) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'b':
                if(strcmp(optarg,"slow5")==0){
                    format_out = FORMAT_ASCII;
                    pressMethod = COMPRESS_NONE;
                }else if(strcmp(optarg,"blow5")==0){
                    format_out = FORMAT_BINARY;
                }else{
                    ERROR("Incorrect output format%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'c':
                if(strcmp(optarg,"none")==0){
                    pressMethod = COMPRESS_NONE;
                }else if(strcmp(optarg,"gzip")==0){
                    pressMethod = COMPRESS_GZIP;
                }else{
                    ERROR("Incorrect compression type%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                if(strcmp(optarg,"true")==0){
                    lossy = 0;
                }else if(strcmp(optarg,"false")==0){
                    lossy = 1;
                }else{
                    ERROR("Incorrect argument%s", "");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'a':
                flag_allow_run_id_mismatch = 1;
                break;
            case 'h':
                if (meta->verbosity_level >= LOG_VERBOSE) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'd':
                arg_dir_out = optarg;
                break;
            case 'p':
                iop = atoi(optarg);
                if (iop < 1) {
                    ERROR("Number of I/O processes should be larger than 0. You entered %d", iop);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'o':
                arg_fname_out = optarg;
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    if(arg_fname_out && arg_dir_out){
        ERROR("output file name and output directory both cannot be set%s","");
        return EXIT_FAILURE;
    }
    if(iop>1 && !arg_dir_out){
        ERROR("output directory should be specified when using multiprocessing iop=%d",iop);
        return EXIT_FAILURE;
    }

    // compression option is only effective with -b blow5
    if(format_out==FORMAT_ASCII && pressMethod!=COMPRESS_NONE){
        ERROR("Compression option is only effective with SLOW5 binary format%s","");
        return EXIT_FAILURE;
    }

    std::string output_file;
    std::string extension;
    if(arg_fname_out){
        output_file = std::string(arg_fname_out);
        extension = output_file.substr(output_file.length()-6, output_file.length());
    }
    if(arg_fname_out && format_out==FORMAT_ASCII && extension!=".slow5"){
        ERROR("Output file extension '%s' does not match with the output format:FORMAT_ASCII", extension.c_str());
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }else if(arg_fname_out && format_out==FORMAT_BINARY && extension!=".blow5"){
        ERROR("Output file extension '%s' does not match with the output format:FORMAT_BINARY", extension.c_str());
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    reads_count readsCount;
    std::vector<std::string> fast5_files;

    if(arg_dir_out){
        struct stat st = {0};
        if (stat(arg_dir_out, &st) == -1) {
            mkdir(arg_dir_out, 0700);
        }else{
            std::vector< std::string > dir_list = list_directory(arg_dir_out);
            if(dir_list.size()>2){
                ERROR("Output director %s is not empty",arg_dir_out);
                return EXIT_FAILURE;
            }
        }
    }
    if(lossy){
        WARNING("[%s] Flag 'lossy' is set. Hence, auxiliary fields are not stored", SLOW5_FILE_FORMAT_SHORT);
    }

    //measure file listing time
    init_realtime = slow5_realtime();
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], fast5_files, 0, FAST5_EXTENSION);
    }
    fprintf(stderr, "[%s] %ld fast5 files found - took %.3fs\n", __func__, fast5_files.size(), slow5_realtime() - init_realtime);

    //measure fast5 conversion time
    init_realtime = slow5_realtime();
    f2s_iop(format_out, pressMethod, lossy, flag_allow_run_id_mismatch, iop, fast5_files, arg_dir_out, meta, &readsCount, arg_fname_out);
    fprintf(stderr, "[%s] Converting %ld fast5 files using %d process - took %.3fs\n", __func__, fast5_files.size(), iop, slow5_realtime() - init_realtime);

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}
