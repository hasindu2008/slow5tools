/* @slow5
**
** slow5 interface
** @author: Hasindu Gamaarachchi (hasindu@garvan.org.au)
** @author: Sasha Jenner (jenner.sasha@gmail.com)
** @@
******************************************************************************/

#ifndef SLOW5_H
#define SLOW5_H

#include "fast5lite.h"
#include "slow5misc.h"
#include "error.h"
#include <sys/wait.h>


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>

#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif


//required for eventalign
//#include <vector>

#define FAST5_NAME "fast5"
#define FAST5_EXTENSION "." FAST5_NAME

#define VERSION "0.1"
#define GLOBAL_HEADER_PREFIX "#" //check
#define COLUMN_HEADER_PREFIX "#"

#define SLOW5_NAME "slow5"
#define SLOW5_EXTENSION "." SLOW5_NAME
#define FILE_FORMAT_HEADER "file_format"
#define SLOW5_FILE_FORMAT GLOBAL_HEADER_PREFIX FILE_FORMAT_HEADER "\t" SLOW5_NAME "v" VERSION "\n"
#define SLOW5_FILE_FORMAT_SHORT SLOW5_NAME "v" VERSION
#define SLOW5_HEADER \
    COLUMN_HEADER_PREFIX \
    "read_id\t" \
    "n_samples\t" \
    "digitisation\t" \
    "offset\t" \
    "range\t" \
    "sample_rate\t" \
    "raw_signal\t" \
    "num_bases\t" \
    "sequence\t" \
    "fast5_path\n"

#define BLOW5_NAME "blow5"
#define BLOW5_EXTENSION "." BLOW5_NAME
#define BLOW5_FILE_FORMAT GLOBAL_HEADER_PREFIX FILE_FORMAT_HEADER "=" BLOW5_NAME "v" VERSION "\n"

#define COLUMN_HEADERS \
    "#read_id\t"\
    "read_group\t"\
    "channel_number\t"\
    "digitisation\t"\
    "offset\t"\
    "range\t"\
    "sampling_rate\t"\
    "duration\t"\
    "raw_signal\t"\
    "read_number\t"\
    "start_time\t"\
    "median_before\t"\
    "end_reason\n"


/* Set windowBits=MAX_WBITS|GZIP_WBITS to obtain gzip deflation and inflation
 * Used in deflateInit2 and inflateInit2 from zlib
 **/
#define GZIP_WBITS (16)
#define Z_MEM_DEFAULT (8)
#define Z_OUT_CHUNK (16384) // 2^14

#include "slow5idx.h" // TODO move?

/// File formats to be dealing with
enum slow5_format {
    SLOW5_ASCII,
    SLOW5_BINARY,
    SLOW5_COMP,
};

struct format_map {
    const char *name;
    enum slow5_format format;
};

static const struct format_map formats[] = {
    { SLOW5_NAME, SLOW5_ASCII },
    { BLOW5_NAME, SLOW5_BINARY},
};

typedef struct {
    uint64_t bad_5_file = 0;
    uint64_t total_5 = 0;
    uint64_t multi_group_slow5 = 0;
}reads_count;

struct program_meta {
    bool debug;
    bool verbose;
};

struct command {
    const char *name;
    int (*main)(int, char **, struct program_meta *);
};


// TODO in misc or here?
#define EXIT_MSG(exit_code, argv, meta) exit_msg(exit_code, argv, meta, __FILE__, __func__, __LINE__);

static inline void exit_msg(const int exit_code, char **argv, struct program_meta *meta,
                            const char *file, const char *func, const int line) {
    if (meta != NULL) {
        if (meta->verbose) {
            VERBOSE("exiting with %s",
                    exit_code == EXIT_SUCCESS ? "SUCCESS" :
                    exit_code == EXIT_FAILURE ? "FAILURE" :
                    "UNKNOWN OUTCOME");
        }
        if (meta->debug) {
            fprintf(stderr, DEBUG_PREFIX "exit code %d" NO_COLOUR,
                    file, func, line, exit_code);
        }
    }
}

enum group_flags{ROOT, READ, RAW, CHANNEL_ID, CONTEXT_TAGS, TRACKING_ID};

typedef struct{
    char *file_format;
    char *file_version;
    char *file_type;
    hsize_t num_read_groups;
    //    READ
    char* pore_type;
    char* run_id;
    //    CONTEXT_TAGS
    char* sample_frequency;
    //additional attributes in 2.2
    char* barcoding_enabled;
    char* sequencing_kit;
    char* experiment_duration_set;
    char* experiment_type;
    char* local_basecalling;
    char* package;
    char* package_version;
    //additional attributes in 2.0
    char* filename;
    char* experiment_kit;
    char* user_filename_input;
    //    TRACKING_ID
    char* asic_id;
    char* asic_id_eeprom;
    char* asic_temp;
    char* auto_update;
    char* auto_update_source;
    char* bream_is_standard;
    char* device_id;
    char* exp_script_name;
    char* exp_script_purpose;
    char* exp_start_time;
    char* flow_cell_id;
    char* heatsink_temp;
    char* hostname;
    char* installation_type;
    char* local_firmware_file;
    char* operating_system;
    char* protocol_run_id;
    char* protocols_version;
    char* tracking_id_run_id;
    char* usb_config;
    char* version;
    //additional attributes in 2.0
    char* bream_core_version;
    char* bream_ont_version;
    char* bream_prod_version;
    char* bream_rnd_version;
    //additional attributes in 2.2
    char* asic_version;
    char* configuration_version;
    char* device_type;
    char* distribution_status;
    char* distribution_version;
    char* flow_cell_product_code;
    char* guppy_version;
    char* protocol_group_id;
    char* sample_id;
}slow5_header_t;

typedef struct{
    //    RAW
    unsigned long start_time;
    unsigned int duration;
    int read_number;
    uint8_t start_mux;
    char* read_id;
    float median_before;
    uint8_t end_reason;
    //    CHANNEL_ID
    float digitisation;
    float offset;
    float range;
    float sampling_rate;
    char* channel_number;

    int16_t *raw_signal;

}slow5_record_t;

enum FormatOut {
    OUT_ASCII,
    OUT_BINARY,
    OUT_COMP,
};

struct operator_obj {
    //attributes to track hdf5 hierarchy
    unsigned        group_level;         /* Recursion level.  0=root */
    struct operator_obj   *prev;          /* Pointer to previous opdata */
    haddr_t         addr;           /* Group address */
    //attributes are useful when writing. They are also passed to the op_func_group function along with the struct
    struct program_meta *meta;
    FILE *f_out;
    enum FormatOut format_out;
    z_streamp strmp;
    FILE *f_idx;
    const char *fast5_path;
    fast5_file_t* fast5_file;
    const char * group_name;
    //attributes store infomation
    slow5_header_t *slow5_header;
    slow5_record_t *slow5_record;
    int *flag_context_tags;
    int *flag_tracking_id;
    size_t* nreads;
};

union attribute_data {
    int attr_int;
    double attr_double;
    uint8_t attr_uint8_t;
    char* attr_string;
};

// args for processes
typedef struct {
    int32_t starti;
    int32_t endi;
    int32_t proc_index;
    std::string slow5_file;
}proc_arg_t;

//implemented in f2s.c
void write_data(FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx, const std::string read_id, const fast5_t f5, const char *fast5_path);

//implemented in read_fast5.c
int read_fast5(fast5_file_t *fast5_file, FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx, int write_header_flag, struct program_meta *meta);
fast5_file_t fast5_open(const char* filename);
void print_slow5_header(operator_obj* operator_data);
void free_attributes(group_flags group_flag, operator_obj* operator_data);

void find_all_5(const std::string& path, std::vector<std::string>& fast5_files, const char* extension);

//implemented in read_slow5.c
int read_slow5_header(FILE *slow5, std::vector<slow5_header_t>& slow5Header, hsize_t num_read_group);
int read_line(FILE* slow5, char ** buffer);
int read_header(std::vector<slow5_header_t>& slow5_headers, FILE* slow5, char** buffer, hsize_t num_read_group);
int find_num_read_group(FILE* slow5, hsize_t* num_read_group);

void print_multi_group_header(FILE *f_out, std::vector<slow5_header_t>& slow5_headers, std::vector<size_t> &run_id_indices, std::vector<std::vector<size_t>> &list, size_t read_group_count);
void print_multi_group_records(FILE *f_out, std::vector<FILE*>& slow5_file_pointers, std::vector<size_t> &run_id_indices, std::vector<std::vector<size_t>> &list, size_t read_group_count, std::vector<size_t> &file_id_tracker);

#endif
