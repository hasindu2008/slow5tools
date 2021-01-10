// Sasha Jenner

//#include "fast5lite.h"
#include "slow5.h"
#include "error.h"
#include <sys/wait.h>

#define USAGE_MSG "Usage: %s [OPTION]... [FAST5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Convert fast5 file(s) to slow5 or (compressed) blow5.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -b, --binary               convert to blow5\n" \
    "    -c, --compress             convert to compressed blow5\n" \
    "    -h, --help                 display this message and exit\n" \
    "    -i, --index=[FILE]         index converted file to FILE -- not default\n" \
    "    -o, --output=[FILE]        output converted contents to FILE -- stdout\n" \
    "    --iop INT                  number of I/O processes to read fast5 files\n" \
    "    -d, --output_dir=[dir]     output directory where slow5files are written to when iop>1\n" \

static double init_realtime = 0;


// args for processes
typedef struct {
    int32_t starti;
    int32_t endi;
    int32_t proc_index;
    std::string slow5_file;
}proc_arg_t;

typedef struct {
    uint64_t bad_fast5_file = 0;
    uint64_t total_fast5 = 0;
}reads_count;

// adapted from https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
/*
bool is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == -1) {
        ERROR("Stat failed to retrive file information%s", "");
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}
*/


int z_deflate_write(z_streamp strmp, const void *ptr, uLong size, FILE *f_out, int flush) {
    int ret = Z_OK;

    strmp->avail_in = size;
    strmp->next_in = (Bytef *) ptr;

    uLong out_sz = Z_OUT_CHUNK;
    unsigned char *out = (unsigned char *) malloc(sizeof *out * out_sz);

    do {
        strmp->avail_out = out_sz;
        strmp->next_out = out;

        ret = deflate(strmp, flush);
        if (ret == Z_STREAM_ERROR) {
            ERROR("deflate failed\n%s", ""); // testing
            return ret;
        }

        unsigned have = out_sz - strmp->avail_out;
        if (fwrite(out, sizeof *out, have, f_out) != have || ferror(f_out)) {
            ERROR("fwrite\n%s", ""); // testing
            return Z_ERRNO;
        }

    } while (strmp->avail_out == 0);

    free(out);
    out = NULL;

    // If still input to deflate
    if (strmp->avail_in != 0) {
        ERROR("still more input to deflate\n%s", ""); // testing
        return Z_ERRNO;
    }

    return ret;
}

bool has_fast5_ext(const char *f_path) {
    bool ret = false;

    if (f_path != NULL) {
        size_t f_path_len = strlen(f_path);
        size_t fast5_ext_len = strlen(FAST5_EXTENSION);

        if (f_path_len >= fast5_ext_len &&
                strcmp(f_path + (f_path_len - fast5_ext_len), FAST5_EXTENSION) == 0) {
            ret = true;
        }
    }

    return ret;
}

void write_data(FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx,
        const std::string read_id, const fast5_t f5, const char *fast5_path) {

    off_t start_pos = -1;
    if (f_idx != NULL) {
        start_pos = ftello(f_out);
        fprintf(f_idx, "%s\t%ld\t", read_id.c_str(), start_pos);
    }

    // Interpret file parameter
    switch (format_out) {
        case OUT_ASCII: {

            fprintf(f_out, "%s\t%ld\t%.1f\t%.1f\t%.1f\t%.1f\t", read_id.c_str(),
                    f5.nsample,f5.digitisation, f5.offset, f5.range, f5.sample_rate);

            for (uint64_t j = 0; j < f5.nsample; ++ j) {
                if (j == f5.nsample - 1) {
                    fprintf(f_out, "%hu", f5.rawptr[j]);
                } else {
                    fprintf(f_out, "%hu,", f5.rawptr[j]);
                }
            }

            fprintf(f_out, "\t%d\t%s\t%s\n", 0, ".", fast5_path);
        } break;

        case OUT_BINARY: {
            // write length of string
            size_t read_id_len = read_id.length();
            fwrite(&read_id_len, sizeof read_id_len, 1, f_out);

            // write string
            const char *read_id_c_str = read_id.c_str();
            fwrite(read_id_c_str, sizeof *read_id_c_str, read_id_len, f_out);

            // write other data
            fwrite(&f5.nsample, sizeof f5.nsample, 1, f_out);
            fwrite(&f5.digitisation, sizeof f5.digitisation, 1, f_out);
            fwrite(&f5.offset, sizeof f5.offset, 1, f_out);
            fwrite(&f5.range, sizeof f5.range, 1, f_out);
            fwrite(&f5.sample_rate, sizeof f5.sample_rate, 1, f_out);

            fwrite(f5.rawptr, sizeof *f5.rawptr, f5.nsample, f_out);

            //todo change to variable

            uint64_t num_bases = 0;
            fwrite(&num_bases, sizeof num_bases, 1, f_out);

            const char *sequences = ".";
            size_t sequences_len = strlen(sequences);
            fwrite(&sequences_len, sizeof sequences_len, 1, f_out);
            fwrite(sequences, sizeof *sequences, sequences_len, f_out);

            size_t fast5_path_len = strlen(fast5_path);
            fwrite(&fast5_path_len, sizeof fast5_path_len, 1, f_out);
            fwrite(fast5_path, sizeof *fast5_path, fast5_path_len, f_out);
         } break;

        case OUT_COMP: {
            // write length of string
            size_t read_id_len = read_id.length();
            z_deflate_write(strmp, &read_id_len, sizeof read_id_len, f_out, Z_NO_FLUSH);

            // write string
            const char *read_id_c_str = read_id.c_str();
            z_deflate_write(strmp, read_id_c_str, sizeof *read_id_c_str * read_id_len, f_out, Z_NO_FLUSH);

            // write other data
            z_deflate_write(strmp, &f5.nsample, sizeof f5.nsample, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.digitisation, sizeof f5.digitisation, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.offset, sizeof f5.offset, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.range, sizeof f5.range, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, &f5.sample_rate, sizeof f5.sample_rate, f_out, Z_NO_FLUSH);

            z_deflate_write(strmp, f5.rawptr, sizeof *f5.rawptr * f5.nsample, f_out, Z_NO_FLUSH);

            //todo change to variable

            uint64_t num_bases = 0;
            z_deflate_write(strmp, &num_bases, sizeof num_bases, f_out, Z_NO_FLUSH);

            const char *sequences = ".";
            size_t sequences_len = strlen(sequences);
            z_deflate_write(strmp, &sequences_len, sizeof sequences_len, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, sequences, sizeof *sequences * sequences_len, f_out, Z_NO_FLUSH);

            size_t fast5_path_len = strlen(fast5_path);
            z_deflate_write(strmp, &fast5_path_len, sizeof fast5_path_len, f_out, Z_NO_FLUSH);
            z_deflate_write(strmp, fast5_path, sizeof *fast5_path * fast5_path_len, f_out, Z_FINISH);

            int ret = deflateReset(strmp);

            if (ret != Z_OK) {
                ERROR("deflateReset failed\n%s", ""); // testing
            }
        } break;
    }

    if (f_idx != NULL) {
        off_t end_pos = ftello(f_out);
        fprintf(f_idx, "%ld\n", end_pos - start_pos);
    }
}

// from nanopolish
// ref: http://stackoverflow.com/a/612176/717706
// return true if the given name is a directory
bool is_directory(const std::string& file_name){
    DIR * dir = opendir(file_name.c_str());
    if(!dir) {
        return false;
    }
    closedir(dir);
    return true;
}

// from nanopolish
std::vector< std::string > list_directory(const std::string& file_name)
{
    std::vector< std::string > res;
    DIR* dir;
    struct dirent *ent;

    dir = opendir(file_name.c_str());
    if(not dir) {
        return res;
    }
    while((ent = readdir(dir))) {
        res.push_back(ent->d_name);
    }
    closedir(dir);
    return res;
}

// given a directory path, recursively find all fast5 files
void find_all_fast5(const std::string& path, std::vector<std::string>& fast5_files)
{
    STDERR("Looking for fast5 in %s", path.c_str());
    if (is_directory(path)) {
        std::vector< std::string > dir_list = list_directory(path);
        for (const auto& fn : dir_list) {
            if(fn == "." or fn == "..") {
                continue;
            }

            std::string full_fn = path + "/" + fn;
            bool is_fast5 = full_fn.find(".fast5") != std::string::npos;
            // JTS 04/19: is_directory is painfully slow
            if(is_directory(full_fn)) {
                // recurse
                find_all_fast5(full_fn,fast5_files);
            } else if (is_fast5) {
                fast5_files.push_back(full_fn);
                //add to the list
            }
        }
    }else{
        bool is_fast5 = path.find(".fast5") != std::string::npos;
        if(is_fast5){
            fast5_files.push_back(path);
        }
    }
}

// what a child process should do, i.e. open a tmp file, go through the fast5 files
void f2s_child_worker(FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx, proc_arg_t args, std::vector<std::string>& fast5_files, char* output_dir, struct program_meta *meta, reads_count* readsCount){


    static size_t call_count = 0;

    FILE *slow5_file_pointer = NULL;
    std::string slow5_path;
    if(output_dir){
        slow5_path = std::string(output_dir);
    }
    fast5_file_t fast5_file;

//    fprintf(stderr,"starti %d\n",args.starti);
    for (int i = args.starti; i < args.endi; i++) {
        readsCount->total_fast5++;
        fast5_file = fast5_open(fast5_files[i].c_str());
        fast5_file.fast5_path = fast5_files[i].c_str();

        if (fast5_file.hdf5_file < 0){
            WARNING("Fast5 file [%s] is unreadable and will be skipped", fast5_files[i].c_str());
            H5Fclose(fast5_file.hdf5_file);
            readsCount->bad_fast5_file++;
            continue;
        }

        if(output_dir){
            if (fast5_file.is_multi_fast5) {
                std::string slow5file = fast5_files[i].substr(fast5_files[i].find_last_of('/'),
                                                              fast5_files[i].length() -
                                                              fast5_files[i].find_last_of('/') - 6) + ".slow5";
                slow5_path += slow5file;
                //fprintf(stderr,"slow5path = %s\n fast5_path = %s\nslow5file = %s\n",slow5_path.c_str(), fast5_files[i].c_str(),slow5file.c_str());

                slow5_file_pointer = fopen(slow5_path.c_str(), "w");

                // An error occured
                if (!slow5_file_pointer) {
                    ERROR("File '%s' could not be opened - %s.",
                          slow5_path.c_str(), strerror(errno));
                    continue;
                } else {
                    f_out = slow5_file_pointer;
                }
                read_fast5(&fast5_file, f_out, format_out, strmp, f_idx, call_count, meta);

            }else{ // single-fast5

                if(!slow5_file_pointer){
                    slow5_path += "/"+std::to_string(args.starti)+".slow5";
                    if(call_count==0){
                        slow5_file_pointer = fopen(slow5_path.c_str(), "w");
                    }else{
                        slow5_file_pointer = fopen(slow5_path.c_str(), "a");
                    }
                    // An error occured
                    if (!slow5_file_pointer) {
                        ERROR("File '%s' could not be opened - %s.",
                              slow5_path.c_str(), strerror(errno));
                        continue;
                    } else {
                        f_out = slow5_file_pointer;
                    }
                }
                read_fast5(&fast5_file, f_out, format_out, strmp, f_idx, call_count++, meta);
            }
        } else{
            if (fast5_file.is_multi_fast5) {
                read_fast5(&fast5_file, f_out, format_out, strmp, f_idx, call_count, meta);
            }else{
                read_fast5(&fast5_file, f_out, format_out, strmp, f_idx, call_count++, meta);
            }
        }


        H5Fclose(fast5_file.hdf5_file);
        if(output_dir && fast5_file.is_multi_fast5){
            if(fclose(slow5_file_pointer) == EOF) {
                WARNING("File '%s' failed on closing - %s.", slow5_path.c_str());
            }
            slow5_path = std::string(output_dir);
            slow5_file_pointer = NULL;
        }
    }
    if(output_dir && !fast5_file.is_multi_fast5) {
        if(fclose(slow5_file_pointer) == EOF) {
            WARNING("File '%s' failed on closing - %s.", slow5_path.c_str());
        }
        slow5_file_pointer = NULL;
    }

    if(meta->verbose){
        fprintf(stderr, "The processed - total fast5: %lu, bad fast5: %lu\n", readsCount->total_fast5, readsCount->bad_fast5_file);
    }

}

void f2s_iop(FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx, int iop, std::vector<std::string>& fast5_files, char* output_dir, struct program_meta *meta, reads_count* readsCount){
    double realtime0 = realtime();
    int64_t num_fast5_files = fast5_files.size();

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
        f2s_child_worker(f_out, format_out, strmp, f_idx, proc_args[0],fast5_files, output_dir, meta, readsCount);
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
            f2s_child_worker(f_out, format_out, strmp, f_idx, proc_args[t],fast5_files,output_dir, meta, readsCount);
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
//    skip_forking:

    fprintf(stderr, "[%s] Parallel converting to slow5 is done - took %.3fs\n", __func__,  realtime() - realtime0);

}

void recurse_dir(const char *f_path, FILE *f_out, enum FormatOut format_out,
                 z_streamp strmp, FILE *f_idx, reads_count* readsCount, char* output_dir, struct program_meta *meta) {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(f_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // If it has the fast5 extension
            if (has_fast5_ext(f_path)) {
                std::vector<std::string> fast5_files;
                fast5_files.push_back(f_path);
                f2s_iop(f_out, format_out, strmp, f_idx, 1, fast5_files, output_dir, meta, readsCount);
            }

        } else {
            WARNING("File '%s' failed to open - %s.",
                    f_path, strerror(errno));
        }

    } else {
        fprintf(stderr, "[%s::%.3f*%.2f] Extracting fast5 from %s\n", __func__,
                realtime() - init_realtime, cputime() / (realtime() - init_realtime), f_path);

        // Iterate through sub files
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 &&
                strcmp(ent->d_name, "..") != 0) {

                // Make sub path string
                // f_path + '/' + ent->d_name + '\0'
                size_t sub_f_path_len = strlen(f_path) + 1 + strlen(ent->d_name) + 1;
                char *sub_f_path = (char *) malloc(sizeof *sub_f_path * sub_f_path_len);
                MALLOC_CHK(sub_f_path);
                snprintf(sub_f_path, sub_f_path_len, "%s/%s", f_path, ent->d_name);

                // Recurse
                recurse_dir(sub_f_path, f_out, format_out, strmp, f_idx, readsCount, output_dir, meta);

                free(sub_f_path);
                sub_f_path = NULL;
            }
        }

        closedir(dir);
    }
}

int f2s_main(int argc, char **argv, struct program_meta *meta) {
    init_realtime = realtime();

    int ret; // For checking return values of functions
    z_stream strm; // Declare stream for compression output if specified

    int iop = 1;

    // Debug: print arguments
    if (meta != NULL && meta->debug) {
        if (meta->verbose) {
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

    static struct option long_opts[] = {
        {"binary", no_argument, NULL, 'b'},    //0
        {"compress", no_argument, NULL, 'c'},  //1
        {"help", no_argument, NULL, 'h'},  //2
        {"index", required_argument, NULL, 'i'},    //3
        {"output", required_argument, NULL, 'o'},   //4
        { "iop", required_argument, NULL, 0}, //5
        { "output_dir", required_argument, NULL, 'd'}, //6
        {NULL, 0, NULL, 0 }
    };

    // Default options
    FILE *f_out = stdout;
    enum FormatOut format_out = OUT_ASCII;
    FILE *f_idx = NULL;

    // Input arguments
    char *arg_fname_out = NULL;
    char *arg_dir_out = NULL;
    char *arg_fname_idx = NULL;

    char opt;
    int longindex = 0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "bchi:o:d:", long_opts, &longindex)) != -1) {
        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }
        switch (opt) {
            case 'b':
                format_out = OUT_BINARY;
                break;
            case 'c':
                format_out = OUT_COMP;
                break;
            case 'h':
                if (meta->verbose) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);
                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                return EXIT_SUCCESS;
            case 'i':
                arg_fname_idx = optarg;
                break;
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'd':
                arg_dir_out = optarg;
                break;
            case  0 :
                if (longindex == 5) {
                    iop = atoi(optarg);
                    if (iop < 1) {
                        ERROR("Number of I/O processes should be larger than 0. You entered %d", iop);
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    if(iop>1 && !arg_dir_out){
        ERROR("output directory should be specified when using multiprocessing iop=%d",iop);
        return EXIT_FAILURE;
    }

    // Parse output argument
    if (arg_fname_out != NULL) {
        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing output filename%s","");
        }
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(arg_fname_out, "w");

        // An error occured
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_out, strerror(errno));

            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            f_out = new_file;
        }
    }

    // Parse index argument
    if (arg_fname_idx != NULL) {
        if (meta != NULL && meta->verbose) {
            VERBOSE("parsing index filename%s","");
        }
        // Create new file or
        // Truncate existing file
        FILE *new_file;
        new_file = fopen(arg_fname_idx, "w");

        // An error occured
        if (new_file == NULL) {
            ERROR("File '%s' could not be opened - %s.",
                  arg_fname_idx, strerror(errno));
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        } else {
            f_idx = new_file;
        }
    }

    // Check for remaining files to parse
    if (optind >= argc) {
        MESSAGE(stderr, "missing fast5 files or directories%s", "");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if (format_out == OUT_COMP) {
    }
    // Output slow5 header
    switch (format_out) {
        case OUT_ASCII:
//            fprintf(f_out, SLOW5_FILE_FORMAT);
//            fprintf(f_out, SLOW5_HEADER);
            break;
        case OUT_BINARY:
            fprintf(f_out, BLOW5_FILE_FORMAT);
            fprintf(f_out, SLOW5_HEADER);
            break;
        case OUT_COMP:
            // Initialise zlib stream structure
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;

            ret = deflateInit2(&strm,
                    Z_DEFAULT_COMPRESSION,
                    Z_DEFLATED,
                    MAX_WBITS | GZIP_WBITS, // Gzip compatible compression
                    Z_MEM_DEFAULT,
                    Z_DEFAULT_STRATEGY);
            if (ret != Z_OK) {
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }

            char header[] = BLOW5_FILE_FORMAT SLOW5_HEADER;
            ret = z_deflate_write(&strm, header, strlen(header), f_out, Z_FINISH);
            if (ret != Z_STREAM_END) {
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
            ret = deflateReset(&strm);
            if (ret != Z_OK) {
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
            }
            break;
    }

    double realtime0 = realtime();
    reads_count readsCount;
    std::vector<std::string> fast5_files;

    for (int i = optind; i < argc; ++ i) {
        if(iop==1){
            // Recursive way
            recurse_dir(argv[i], f_out, format_out, &strm, f_idx, &readsCount, arg_dir_out, meta);
        }else{
            find_all_fast5(argv[i], fast5_files);
        }
    }

    if(iop==1){
        MESSAGE(stderr, "total fast5: %lu, bad fast5: %lu", readsCount.total_fast5, readsCount.bad_fast5_file);
    }else{
        fprintf(stderr, "[%s] %ld fast5 files found - took %.3fs\n", __func__, fast5_files.size(), realtime() - realtime0);
        f2s_iop(f_out, format_out, &strm, f_idx, iop, fast5_files, arg_dir_out, meta, &readsCount);
    }

    if (format_out == OUT_COMP) {
        ret = deflateEnd(&strm);

        if (ret != Z_OK) {
            EXIT_MSG(EXIT_FAILURE, argv, meta);
            return EXIT_FAILURE;
        }
    }

    // Close output file
    if (arg_fname_out != NULL && fclose(f_out) == EOF) {
        ERROR("File '%s' failed on closing - %s.",
              arg_fname_out, strerror(errno));

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if (arg_fname_idx != NULL && fclose(f_idx) == EOF) {
        ERROR("File '%s' failed on closing - %s.",
              arg_fname_idx, strerror(errno));

        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    EXIT_MSG(EXIT_SUCCESS, argv, meta);
    return EXIT_SUCCESS;
}

