// definitions used for FAST5 to SLOW5 conversion
#include <vector>

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
#include <unordered_map>
#include "misc.h"

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
    FILE *f_out;
    enum slow5_fmt format_out;
    slow5_press_method_t pressMethod;
    slow5_press_t* press_ptr;
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
    int *flag_header_is_written;
    int *flag_dump_all;
    hsize_t* num_read_groups;
    size_t* nreads;
    slow5_file_t* slow5File;
    std::unordered_map<std::string, uint32_t>* warning_map;
    int *primary_fields_count;
};

//implemented in read_fast5.c
int read_fast5(opt_t *user_opts,
               fast5_file_t *fast5_file,
               slow5_file_t *slow5File,
               int write_header_flag,
               std::unordered_map<std::string, uint32_t>* warning_map);
fast5_file_t fast5_open(const char* filename);
//void free_attributes(group_flags group_flag, operator_obj* operator_data);
std::vector< std::string > list_directory(const std::string& file_name);
void list_all_items(const std::string& path, std::vector<std::string>& files, int count_dir, const char* extension);
int slow5_hdr_initialize(slow5_hdr *header, int lossy);

// args for processes
typedef struct {
    int32_t starti;
    int32_t endi;
    int32_t proc_index;
}proc_arg_t;

union attribute_data {
    int attr_int;
    double attr_double;
    uint8_t attr_uint8_t;
    char* attr_string;
};
