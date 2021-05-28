
#ifndef HAVE_CONFIG_H
    #define HAVE_CONFIG_H
    #include "config.h"
#endif

#define H5_USE_110_API 1

#ifdef HAVE_HDF5_SERIAL_HDF5_H
#    include <hdf5/serial/hdf5.h>
#endif

#ifdef HAVE_HDF5_H
#    include <hdf5.h>
#endif

#ifdef HAVE_HDF5_HDF5_H
#    include <hdf5/hdf5.h>
#endif

#ifdef HAVE___HDF5_INCLUDE_HDF5_H
#    include <hdf5.h>
#endif

#define VERSION "0.1"
#define FAST5_NAME "fast5"
#define FAST5_EXTENSION "." FAST5_NAME
#define SLOW5_NAME "slow5"
#define SLOW5_EXTENSION "." SLOW5_NAME
#define SLOW5_FILE_FORMAT_SHORT SLOW5_NAME "v" VERSION
#define SLOW5_NAME "slow5"

KHASH_MAP_INIT_STR(warncount, uint32_t)

typedef struct {
    uint64_t bad_5_file = 0;
    uint64_t total_5 = 0;
    uint64_t multi_group_slow5 = 0;
}reads_count;

typedef struct{
    hid_t hdf5_file;
    bool is_multi_fast5;
    const char* fast5_path;
}  fast5_file_t;

enum group_flags{ROOT, READ, RAW, CHANNEL_ID, CONTEXT_TAGS, TRACKING_ID};

struct operator_obj {
    //attributes to track hdf5 hierarchy
    unsigned        group_level;         /* Recursion level.  0=root */
    struct operator_obj   *prev;          /* Pointer to previous opdata */
    haddr_t         addr;           /* Group address */
    //attributes are useful when writing. They are also passed to the fast5_group_itr function along with the struct
    struct program_meta *meta;
    FILE *f_out;
    enum slow5_fmt format_out;
    enum press_method pressMethod;
    press_t* press_ptr;
    const char *fast5_path;
    fast5_file_t* fast5_file;
    const char * group_name;
    //attributes store infomation
    slow5_rec_t *slow5_record;
    int *flag_context_tags;
    int *flag_tracking_id;
    int *flag_run_id;
    int *flag_lossy;
    int *flag_write_header;
    int *flag_allow_run_id_mismatch;
    hsize_t* num_read_groups;
    size_t* nreads;
    size_t* warning_flag_allow_run_id_mismatch;
    slow5_file_t* slow5File;
    kh_warncount_s **warncount_hash;
};

//implemented in read_fast5.c
int
read_fast5(fast5_file_t *fast5_file, slow5_fmt format_out, press_method pressMethod, int lossy, int write_header_flag,
           int flag_allow_run_id_mismatch, struct program_meta *meta, slow5_file_t *slow5File, kh_warncount_t **warncount_hash);
fast5_file_t fast5_open(const char* filename);
//void free_attributes(group_flags group_flag, operator_obj* operator_data);
std::vector< std::string > list_directory(const std::string& file_name);
void list_all_items(const std::string& path, std::vector<std::string>& files, int count_dir, const char* extension);
void slow5_hdr_initialize(slow5_hdr *header, int lossy);

// args for processes
typedef struct {
    int32_t starti;
    int32_t endi;
    int32_t proc_index;
    std::string slow5_file;
}proc_arg_t;

union attribute_data {
    int attr_int;
    double attr_double;
    uint8_t attr_uint8_t;
    char* attr_string;
};
