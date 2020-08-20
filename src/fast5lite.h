#ifndef FAST5LITE_H
#define FAST5LITE_H

#ifndef HAVE_CONFIG_H
    #define HAVE_CONFIG_H
    #include "config.h"
#endif

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

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "error.h"
#include <string>
#include <vector>

typedef struct {
    int16_t* rawptr;   // raw signal (float is not the best datatype type though)
    uint64_t nsample; // number of samples

    //	Information for scaling raw data from ADC values to pA (are these duplicates?)
    float digitisation;
    float offset;
    float range;
    float sample_rate;

    // computed scaling paramtersd
    float scale;
    float shift;
    float drift;
    float var;
    float scale_sd;
    float var_sd;

    // derived parameters that are cached for efficiency. do we need these?
    float log_var;
    float scaled_var;
    float log_scaled_var;

} fast5_t;


#define LEGACY_FAST5_RAW_ROOT "/Raw/Reads/"

// The following are either directly from Nanopolish with minor modification: nanopolish_fast5_io.cpp

typedef struct{
    hid_t hdf5_file;
    bool is_multi_fast5;
}  fast5_file_t;


//
std::vector<std::string> fast5_get_multi_read_groups(fast5_file_t fh)
{
    std::vector<std::string> out;
    ssize_t buffer_size = 0;
    char* buffer = NULL;

    // get the number of groups in the root group
    H5G_info_t group_info;
    int ret = H5Gget_info_by_name(fh.hdf5_file, "/", &group_info, H5P_DEFAULT);
    if(ret < 0) {
        fprintf(stderr, "error getting group info\n");
        exit(EXIT_FAILURE);
    }

    for(size_t group_idx = 0; group_idx < group_info.nlinks; ++group_idx) {

        // retrieve the size of this group name
        ssize_t size = H5Lget_name_by_idx(fh.hdf5_file, "/", H5_INDEX_NAME, H5_ITER_INC, group_idx, NULL, 0, H5P_DEFAULT);

        if(size < 0) {
            fprintf(stderr, "error getting group name size\n");
            exit(EXIT_FAILURE);
        }
        size += 1; // for null terminator

        if(size > buffer_size) {
            buffer = (char*)realloc(buffer, size);
            buffer_size = size;
        }

        // copy the group name
        H5Lget_name_by_idx(fh.hdf5_file, "/", H5_INDEX_NAME, H5_ITER_INC, group_idx, buffer, buffer_size, H5P_DEFAULT);
        buffer[size-1] = '\0';
        out.push_back(buffer);
    }

    free(buffer);
    buffer = NULL;
    buffer_size = 0;
    return out;
}


// from nanopolish_fast5_io.cpp
std::string fast5_get_raw_read_internal_name(fast5_file_t fh)
{
    // This code is From scrappie's fast5_interface

    // retrieve the size of the read name
    ssize_t size =
        H5Lget_name_by_idx(fh.hdf5_file, LEGACY_FAST5_RAW_ROOT, H5_INDEX_NAME, H5_ITER_INC, 0, NULL, 0, H5P_DEFAULT);

    if (size < 0) {
        return "";
    }

    // copy the read name out of the fast5
    char* name = (char*)calloc(1 + size, sizeof(char));
    H5Lget_name_by_idx(fh.hdf5_file, LEGACY_FAST5_RAW_ROOT, H5_INDEX_NAME, H5_ITER_INC, 0, name, 1 + size, H5P_DEFAULT);

    // cleanup
    std::string out(name);
    free(name);
    return out;
}

// from nanopolish_fast5_io.cpp
std::string fast5_get_string_attribute(fast5_file_t fh, const std::string& group_name, const std::string& attribute_name)
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


// from nanopolish_fast5_io.cpp
std::string fast5_get_raw_read_group(fast5_file_t fh, const std::string& read_id)
{
    if(fh.is_multi_fast5) {
        return "/read_" + read_id + "/Raw";
    } else {
        std::string internal_read_name = fast5_get_raw_read_internal_name(fh);
        return internal_read_name != "" ? std::string(LEGACY_FAST5_RAW_ROOT) + "/" + internal_read_name : "";
    }
}




// from nanopolish_fast5_io.cpp
static inline fast5_file_t fast5_open(char* filename) {
    fast5_file_t fh;
    fh.hdf5_file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);

    // read and parse the file version to determine if this is a multi-fast5 structured file
    std::string version_str = fast5_get_string_attribute(fh, "/", "file_version");
    if(version_str != "") {
        int major;
        int minor;
        int ret = sscanf(version_str.c_str(), "%d.%d", &major, &minor);
        if(ret != 2) {
            fprintf(stderr, "Could not parse version string %s\n", version_str.c_str());
            exit(EXIT_FAILURE);
        }

        fh.is_multi_fast5 = major >= 1;
    } else {
        fh.is_multi_fast5 = false;
    }



    return fh;
}

//from nanopolish_fast5_io.cpp
static inline void fast5_close(fast5_file_t fh) {
    H5Fclose(fh.hdf5_file);
}

//from nanopolish_fast5_io.cpp
static inline float fast5_read_float_attribute(hid_t group,
                                               const char* attribute) {
    float val = NAN;

    hid_t attr = H5Aopen(group, attribute, H5P_DEFAULT);
    if (attr < 0) {
        WARNING("Failed to open attribute '%s'.", attribute);
        return NAN;
    }

    herr_t ret = H5Aread(attr, H5T_NATIVE_FLOAT, &val);
    if (ret < 0) {
        WARNING("Failed to read attribute '%s'.", attribute);
        return NAN;
    }

    H5Aclose(attr);

    return val;
}



static inline int32_t fast5_read_single_fast5(fast5_file_t fh, fast5_t* f5) {

    hid_t hdf5_file = fh.hdf5_file;
    f5->rawptr = NULL;
    hid_t space;
    hsize_t nsample;
    herr_t status;

    // retrieve the size of the read name
    ssize_t size = H5Lget_name_by_idx(hdf5_file, LEGACY_FAST5_RAW_ROOT, H5_INDEX_NAME,
                                      H5_ITER_INC, 0, NULL, 0, H5P_DEFAULT);

    if (size < 0) {
        WARNING("Failed to retrieve the size of the read name.%s", "");
        return -1;
    }

    // copy the read name out of the fast5
    char* read_name = (char*)calloc(1 + size, sizeof(char));
    ssize_t ret =
        H5Lget_name_by_idx(hdf5_file, LEGACY_FAST5_RAW_ROOT, H5_INDEX_NAME, H5_ITER_INC, 0,
                           read_name, 1 + size, H5P_DEFAULT);
    if (ret < 0) {
        WARNING("Failed to retrieve the read name.%s", "");
        return -1;
    }

    // not the most efficient and safest, but being a bit lazy for the moment
    char* signal_path = (char*)malloc(
        (size + 1 + strlen(LEGACY_FAST5_RAW_ROOT) + strlen("/Signal")) * sizeof(char));
    MALLOC_CHK(signal_path);

    sprintf(signal_path, "%s%s%s", LEGACY_FAST5_RAW_ROOT, read_name, "/Signal");

#ifdef DEBUG_SIGNAL_PATH
    printf("Signal path : %s\n", signal_path);
#endif
    free(read_name);

    hid_t dset = H5Dopen(hdf5_file, signal_path, H5P_DEFAULT);
    if (dset < 0) {
        WARNING("Failed to open dataset '%s' to read raw signal.", signal_path);
        free(signal_path);
        return -1;
        // goto cleanup2;
    }

    space = H5Dget_space(dset);
    if (space < 0) {
        WARNING("Failed to create copy of dataspace for raw signal %s.",
                signal_path);
        H5Dclose(dset);
        free(signal_path);
        return -1;
        // goto cleanup3;
    }

    int32_t ret1 = H5Sget_simple_extent_dims(space, &nsample, NULL);
    if (ret1 < 0) {
        WARNING("Failed to get the dataspace dimension for raw signal %s.",
                signal_path);
        H5Sclose(space);
        H5Dclose(dset);
        free(signal_path);
        return -1;
    }

    f5->nsample = nsample;
    f5->rawptr = (int16_t*)calloc(nsample, sizeof(float));
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                     f5->rawptr);

    if (status < 0) {
        free(f5->rawptr);
        WARNING("Failed to read raw data from dataset %s.", signal_path);
        H5Sclose(space);
        H5Dclose(dset);
        free(signal_path);
        return -1;
    }

    H5Sclose(space);
    H5Dclose(dset);

    // get channel parameters
    const char* scaling_path = "/UniqueGlobalKey/channel_id";

    hid_t scaling_group = H5Gopen(hdf5_file, scaling_path, H5P_DEFAULT);
    if (scaling_group < 0) {
        WARNING("Failed to open group %s.", scaling_path);
        free(signal_path);
        return -1;
    }

    f5->digitisation =
        fast5_read_float_attribute(scaling_group, "digitisation");
    f5->offset = fast5_read_float_attribute(scaling_group, "offset");
    f5->range = fast5_read_float_attribute(scaling_group, "range");
    f5->sample_rate =
        fast5_read_float_attribute(scaling_group, "sampling_rate");

    if (f5->digitisation == NAN || f5->offset == NAN || f5->range == NAN ||
        f5->sample_rate == NAN) {
        WARNING("Read a NAN value for scaling parameters for '%s'.",
                signal_path);
        H5Gclose(scaling_group);
        free(signal_path);
        return -1;
    }

    H5Gclose(scaling_group);
    free(signal_path);

    return 0;
}




static inline int32_t fast5_read_multi_fast5(fast5_file_t fh, fast5_t* f5, std::string read_id) {

    hid_t hdf5_file = fh.hdf5_file;
    f5->rawptr = NULL;
    hid_t space;
    hsize_t nsample;
    herr_t status;

    // mostly from scrappie
    std::string raw_read_group = fast5_get_raw_read_group(fh, read_id);

    // Create data set name
    std::string signal_path_str = raw_read_group + "/Signal";
    const char *signal_path = signal_path_str.c_str();

    hid_t dset = H5Dopen(hdf5_file, signal_path, H5P_DEFAULT);
    if (dset < 0) {
        WARNING("Failed to open dataset '%s' to read raw signal.", signal_path);
        return -1;
        // goto cleanup2;
    }

    space = H5Dget_space(dset);
    if (space < 0) {
        WARNING("Failed to create copy of dataspace for raw signal %s.",
                signal_path);
        H5Dclose(dset);
        return -1;
        // goto cleanup3;
    }

    int32_t ret1 = H5Sget_simple_extent_dims(space, &nsample, NULL);
    if (ret1 < 0) {
        WARNING("Failed to get the dataspace dimension for raw signal %s.",
                signal_path);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }

    f5->nsample = nsample;
    f5->rawptr = (int16_t*)calloc(nsample, sizeof(float));
    status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                     f5->rawptr);

    if (status < 0) {
        free(f5->rawptr);
        WARNING("Failed to read raw data from dataset %s.", signal_path);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }

    H5Sclose(space);
    H5Dclose(dset);

    // get channel parameters
    std::string scaling_path_str = fh.is_multi_fast5 ? "/read_" + read_id + "/channel_id"
                                                 :  "/UniqueGlobalKey/channel_id";
    const char *scaling_path = scaling_path_str.c_str();

    hid_t scaling_group = H5Gopen(hdf5_file, scaling_path, H5P_DEFAULT);
    if (scaling_group < 0) {
        WARNING("Failed to open group %s.", scaling_path);
        return -1;
    }

    f5->digitisation =
        fast5_read_float_attribute(scaling_group, "digitisation");
    f5->offset = fast5_read_float_attribute(scaling_group, "offset");
    f5->range = fast5_read_float_attribute(scaling_group, "range");
    f5->sample_rate =
        fast5_read_float_attribute(scaling_group, "sampling_rate");

    if (f5->digitisation == NAN || f5->offset == NAN || f5->range == NAN ||
        f5->sample_rate == NAN) {
        WARNING("Read a NAN value for scaling parameters for '%s'.",
                signal_path);
        H5Gclose(scaling_group);
        return -1;
    }

    H5Gclose(scaling_group);

    return 0;
}


static inline int32_t fast5_read(fast5_file_t fh, fast5_t* f5, std::string read_id) {

    if(fh.is_multi_fast5){
        return fast5_read_multi_fast5(fh, f5, read_id);
    }
    else{
        return fast5_read_single_fast5(fh, f5);
    }

}

//from nanopolish_fast5_io.cpp //used by the indexer
//todo convert to C
static inline std::string fast5_get_fixed_string_attribute(fast5_file_t fh, const std::string& group_name, const std::string& attribute_name)
{
    hid_t hdf5_file = fh.hdf5_file;
    size_t storage_size;
    char* buffer;
    hid_t group, attribute, attribute_type;
    int ret;
    std::string out;

    // according to http://hdf-forum.184993.n3.nabble.com/check-if-dataset-exists-td194725.html
    // we should use H5Lexists to check for the existence of a group/dataset using an arbitrary path
    ret = H5Lexists(hdf5_file, group_name.c_str(), H5P_DEFAULT);
    if(ret <= 0) {
        return "";
    }

    // Open the group /Raw/Reads/Read_nnn
    group = H5Gopen(hdf5_file, group_name.c_str(), H5P_DEFAULT);
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
    if(H5Tis_variable_str(attribute_type)) {
#ifdef DEBUG_FAST5_IO
        fprintf(stderr, "variable length string detected -- ignoring attribute\n");
#endif
        goto close_type;
    }

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
close_type:
    H5Tclose(attribute_type);
close_attr:
    H5Aclose(attribute);
close_group:
    H5Gclose(group);

    return out;
}

static inline std::string fast5_get_read_id_single_fast5(fast5_file_t fh)
{

    // this function is not supported for multi-fast5 files
    assert(!fh.is_multi_fast5);

    hid_t hdf5_file = fh.hdf5_file;
    std::string out = "";

    // Get the path to the raw read group
    // retrieve the size of the read name
    ssize_t size = H5Lget_name_by_idx(hdf5_file, LEGACY_FAST5_RAW_ROOT, H5_INDEX_NAME,
                                      H5_ITER_INC, 0, NULL, 0, H5P_DEFAULT);

    if (size < 0) {
        WARNING("Failed to retrieve the size of the read name.%s", "");
        return out;
    }

    // copy the read name out of the fast5
    char* read_name = (char*)calloc(1 + size, sizeof(char));
    ssize_t ret =
        H5Lget_name_by_idx(hdf5_file, LEGACY_FAST5_RAW_ROOT, H5_INDEX_NAME, H5_ITER_INC, 0,
                           read_name, 1 + size, H5P_DEFAULT);
    if (ret < 0) {
        WARNING("Failed to retrieve the read name.%s", "");
        return out;
    }

    std::string raw_read_group= std::string(LEGACY_FAST5_RAW_ROOT) + read_name;
    free(read_name);


    return fast5_get_fixed_string_attribute(fh, raw_read_group, "read_id");
}

#endif
