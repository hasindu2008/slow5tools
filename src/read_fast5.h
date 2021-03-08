
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
#define SLOW5_FILE_FORMAT_SHORT SLOW5_NAME "v" VERSION
#define SLOW5_NAME "slow5"

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
    unsigned long long start_time;
    unsigned int duration;
    int read_number;
    uint8_t start_mux;
    char* read_id;
    double median_before;
    uint8_t end_reason;
    //    CHANNEL_ID
    float digitisation;
    float offset;
    float range;
    float sampling_rate;
    char* channel_number;

    int16_t *raw_signal;

}slow5_record_t;


struct operator_obj {
    //attributes to track hdf5 hierarchy
    unsigned        group_level;         /* Recursion level.  0=root */
    struct operator_obj   *prev;          /* Pointer to previous opdata */
    haddr_t         addr;           /* Group address */
    //attributes are useful when writing. They are also passed to the op_func_group function along with the struct
    struct program_meta *meta;
    FILE *f_out;
    enum slow5_fmt format_out;
    enum press_method pressMethod;
    press_t* press_ptr;
    const char *fast5_path;
    fast5_file_t* fast5_file;
    const char * group_name;
    //attributes store infomation
    slow5_header_t *slow5_header;
    slow5_rec_t *slow5_record;
    int *flag_context_tags;
    int *flag_tracking_id;
    int *flag_run_id;
    hsize_t* num_read_groups;
    size_t* nreads;
    slow5_file_t* slow5File;
};

//implemented in read_fast5.c
int read_fast5(fast5_file_t *fast5_file, enum slow5_fmt format_out, enum press_method pressMethod, int write_header_flag, struct program_meta *meta, slow5_file_t* slow5File);
fast5_file_t fast5_open(const char* filename);
void print_slow5_header(operator_obj* operator_data);
//void free_attributes(group_flags group_flag, operator_obj* operator_data);
void print_slow5_header(operator_obj* operator_data);
void find_all_5(const std::string& path, std::vector<std::string>& fast5_files, const char* extension);



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


// from nanopolish_fast5_io.cpp
static inline  std::string fast5_get_string_attribute(fast5_file_t fh, const std::string& group_name, const std::string& attribute_name)
{
    hid_t group, attribute, attribute_type, native_type;
    std::string out;

    // according to http://hdf-forum.184993.n3.nabble.com/check-if-dataset-exists-td194725.html
    // we should use H5Lexists to check for the existence of a group/dataset using an arbitrary path
    // HDF5 1.8 returns 0 on the root group, so we explicitly check for it
    int ret = group_name == "/" ? 1 : H5Lexists(fh.hdf5_file, group_name.c_str(), H5P_DEFAULT);
    if(ret <= 0) {
        return "";
    }

    // Open the group containing the attribute we want
    group = H5Gopen(fh.hdf5_file, group_name.c_str(), H5P_DEFAULT);
    if(group < 0) {
#ifdef DEBUG_FAST5_IO
        fprintf(stderr, "could not open group %s\n", group_name.c_str());
#endif
        goto close_group;
    }

    // Ensure attribute exists
    ret = H5Aexists(group, attribute_name.c_str());
    if(ret <= 0) {
        goto close_group;
    }

    // Open the attribute
    attribute = H5Aopen(group, attribute_name.c_str(), H5P_DEFAULT);
    if(attribute < 0) {
#ifdef DEBUG_FAST5_IO
        fprintf(stderr, "could not open attribute: %s\n", attribute_name.c_str());
#endif
        goto close_attr;
    }

    // Get data type and check it is a fixed-length string
    attribute_type = H5Aget_type(attribute);
    if(attribute_type < 0) {
#ifdef DEBUG_FAST5_IO
        fprintf(stderr, "failed to get attribute type %s\n", attribute_name.c_str());
#endif
        goto close_type;
    }

    if(H5Tget_class(attribute_type) != H5T_STRING) {
#ifdef DEBUG_FAST5_IO
        fprintf(stderr, "attribute %s is not a string\n", attribute_name.c_str());
#endif
        goto close_type;
    }

    native_type = H5Tget_native_type(attribute_type, H5T_DIR_ASCEND);
    if(native_type < 0) {
#ifdef DEBUG_FAST5_IO
        fprintf(stderr, "failed to get native type for %s\n", attribute_name.c_str());
#endif
        goto close_native_type;
    }

    if(H5Tis_variable_str(attribute_type) > 0) {
        // variable length string
        char* buffer;
        ret = H5Aread(attribute, native_type, &buffer);
        if(ret < 0) {
            fprintf(stderr, "error reading attribute %s\n", attribute_name.c_str());
            exit(EXIT_FAILURE);
        }
        out = buffer;
        free(buffer);
        buffer = NULL;

    } else {
        // fixed length string
        size_t storage_size;
        char* buffer;

        // Get the storage size and allocate
        storage_size = H5Aget_storage_size(attribute);
        buffer = (char*)calloc(storage_size + 1, sizeof(char));

        // finally read the attribute
        ret = H5Aread(attribute, attribute_type, buffer);
        if(ret >= 0) {
            out = buffer;
        }

        // clean up
        free(buffer);
    }

close_native_type:
    H5Tclose(native_type);
close_type:
    H5Tclose(attribute_type);
close_attr:
    H5Aclose(attribute);
close_group:
    H5Gclose(group);

    return out;
}
