/**
 * @file read_fast5.c
 * @brief supplementary code for FAST5 to SLOW5 conversion
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au)
 * @date 27/02/2021
 */

#include <string>
#include <vector>

#include <slow5/slow5.h>
#include "error.h"
#include "slow5_extra.h"

int slow5_hdr_initialize(slow5_hdr *header, int lossy){
    if (slow5_hdr_add_rg(header) < 0){
        return -1;
    }
    header->num_read_groups = 1;
    if(lossy==0){
        struct slow5_aux_meta *aux_meta = slow5_aux_meta_init_empty();
        header->aux_meta = aux_meta;
    }
    return 0;
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

// from nanopolish
// given a directory path, recursively find all files
//if count_dir==0; then don't list dir
//if count_dir==1; then list dir as well
//if count_dir==2; then list only dir
void list_all_items(const std::string& path, std::vector<std::string>& files, int count_dir, const char* extension){
    std::string extension_string;
    size_t extension_length = 0;
    if(extension){
        extension_string = std::string(extension);
        extension_length = extension_string.length();
        if (is_directory(path)) {
            STDERR("Looking for '*%s' files in %s", extension, path.c_str());
        }
    }
    if(is_directory(path) && count_dir==1){
        files.push_back(path);
    }
    if(is_directory(path) && count_dir==2){
        files.push_back(path);
    }
    if (is_directory(path)) {
        std::vector< std::string > dir_list = list_directory(path);
        for (const auto& fn : dir_list) {
            if(fn == "." or fn == "..") {
                continue;
            }
            std::string full_fn = path + "/" + fn;
            // JTS 04/19: is_directory is painfully slow
            if(is_directory(full_fn)){
                // recurse
                list_all_items(full_fn, files, count_dir, extension);
            }else{
                //add to the list
                if(extension){
                    std::string full_fn_last_part = full_fn.substr(full_fn.length()-extension_length, extension_length);
                    int ret_strcmp = strcmp(full_fn_last_part.c_str(), extension);
                    if(strcmp(extension,".slow5")==0 && ret_strcmp!=0){
                        ret_strcmp = strcmp(full_fn_last_part.c_str(), ".blow5");
                    }
                    if(ret_strcmp==0 && count_dir!=2){
                        files.push_back(full_fn);
                    }
                }else{
                    if(count_dir!=2){
                        files.push_back(full_fn);
                    }
                }
            }
        }
    }else{
        if(extension){
            std::string full_fn_last_part = path.substr(path.length()-extension_length, extension_length);
            int ret_strcmp = strcmp(full_fn_last_part.c_str(), extension);
            if(strcmp(extension,".slow5")==0 && ret_strcmp!=0){
                ret_strcmp = strcmp(full_fn_last_part.c_str(), ".blow5");
            }
            if(ret_strcmp==0 && count_dir!=2){
                files.push_back(path);
            }
        }else{
            if(count_dir!=2){
                files.push_back(path);
            }
        }
    }
}

#ifndef DISABLE_HDF5

#include <set>
#include "cmd.h"
#include "slow5_misc.h"
#include "misc.h"
#include "read_fast5.h"

#define WARNING_LIMIT 1
#define PRIMARY_FIELD_COUNT 7 //without read_group number
#define H5Z_FILTER_VBZ 32020 //We need to find out what the numerical value for this is

#define BUFFER_CAP (20*1024*1024)

#define REPORT_MESG " Please report this with an example FAST5 file at 'https://github.com/hasindu2008/slow5tools/issues' for us to investigate."

extern int slow5tools_verbosity_level;

// Operator function to be called by H5Aiterate.
herr_t fast5_attribute_itr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data);
// Operator function to be called by H5Literate.
herr_t fast5_group_itr (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);

// from nanopolish_fast5_io.cpp
static inline  std::string fast5_get_string_attribute(fast5_file_t fh, const std::string& group_name, const std::string& attribute_name);

//other functions
int group_check(struct operator_obj *od, haddr_t target_addr);

void search_and_warn(operator_obj *operator_data, std::string key, const char *warn_message);

int add_aux_slow5_attribute(const char *name, operator_obj *operator_data, H5T_class_t h5TClass, attribute_data value, enum slow5_aux_type slow5_type, std::vector<const char *> enum_labels_list_ptrs);

void type_inconsistency_warn(const char *name, operator_obj *operator_data, std::string h5t_class_string, const char *expected_type, std::string slow5_class, const char *slow5_expected_type);

int convert_to_correct_datatype(attribute_data data, slow5_aux_type from_type, slow5_aux_type to_type);

int add_slow5_auxiliary_attribute(std::string h5t_class_string, std::string slow5_class_string, const char *hdf5_expected_type, hid_t hdf5_expected_type_, const char *slow5_expected_type,
                                  slow5_aux_type slow5_expected_type_, hid_t native_type, const char *name,
                                  operator_obj *operator_data, H5T_class_t H5Tclass, attribute_data value,
                                  slow5_aux_type slow5_class, std::vector<const char *> enum_labels_list_ptrs);

int print_slow5_header(operator_obj* operator_data) {
    if(slow5_hdr_fwrite(operator_data->slow5File->fp, operator_data->slow5File->header, operator_data->format_out, operator_data->pressMethod) == -1){
        ERROR("Could not write the SLOW5 header to %s", operator_data->slow5File->meta.pathname);
        return -1;
    }
    *(operator_data->flag_header_is_written) = 1;
    return 0;
}

int print_record(operator_obj* operator_data) {
    if(slow5_rec_fwrite(operator_data->slow5File->fp, operator_data->slow5_record, operator_data->slow5File->header->aux_meta, operator_data->format_out, operator_data->press_ptr) == -1){
        ERROR("Could not write the SLOW5 record for read id '%s' to %s.", operator_data->slow5_record->read_id, operator_data->slow5File->meta.pathname);
        return -1;
    }
    return 0;
}

// from nanopolish_fast5_io.cpp
fast5_file_t fast5_open(const char* filename) {

    fast5_file_t fh;
    fh.hdf5_file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    fh.is_multi_fast5 = false;

    // read and parse the file version to determine if this is a multi-fast5 structured file
    std::string version_str = fast5_get_string_attribute(fh, "/", "file_version");
    std::string file_type = fast5_get_string_attribute(fh, "/", "file_type");
    if(file_type == "multi-read"){
        fh.is_multi_fast5 = true;
    } else {
        if(version_str != "") {
            int major;
            int minor;
            int ret = sscanf(version_str.c_str(), "%d.%d", &major, &minor);
            if(ret != 2) {
                ERROR("Bad fast5: Could not parse the fast5 version string '%s'.", version_str.c_str());
                exit(EXIT_FAILURE);
            }
            fh.is_multi_fast5 = major >= 1;
        }
    }
    return fh;
}

int read_fast5(opt_t *user_opts,
               fast5_file_t *fast5_file,
               slow5_file_t *slow5File,
               int write_header_flag,
               std::unordered_map<std::string, uint32_t>* warning_map) {

    slow5_fmt format_out = user_opts->fmt_out;
    slow5_press_method_t press_out = {user_opts->record_press_out, user_opts->signal_press_out};


    struct operator_obj tracker;
    tracker.group_level = ROOT;
    tracker.prev = NULL;
    H5O_info_t infobuf;
    H5Oget_info(fast5_file->hdf5_file, &infobuf);
    tracker.addr = infobuf.addr;

    tracker.fast5_file = fast5_file;
    tracker.format_out = format_out;
    tracker.pressMethod = press_out;
    tracker.press_ptr = slow5_press_init(press_out);
    if(!tracker.press_ptr){
        ERROR("Could not initialise the slow5 compression method. %s","");
        return -1;
    }

    tracker.fast5_path = fast5_file->fast5_path;
    tracker.slow5File = slow5File;

    int flag_context_tags = 0;
    int flag_tracking_id = 0;
    int flag_run_id = 0;
    int flag_run_id_tracking_id = 0;
    int flag_lossy = user_opts->flag_lossy;
    int primary_fields_count = 0;
    int flag_header_is_written = write_header_flag;
    int flag_allow_run_id_mismatch = user_opts->flag_allow_run_id_mismatch;
    int flag_dump_all = user_opts->flag_dump_all;


    size_t zero0 = 0;

    tracker.flag_context_tags = &flag_context_tags;
    tracker.flag_tracking_id = &flag_tracking_id;
    tracker.flag_run_id = &flag_run_id;
    tracker.flag_run_id_tracking_id = &flag_run_id_tracking_id;
    tracker.flag_lossy = &flag_lossy;
    tracker.flag_allow_run_id_mismatch = &flag_allow_run_id_mismatch;
    tracker.flag_header_is_written = &flag_header_is_written;
    tracker.flag_dump_all = &flag_dump_all;
    tracker.nreads = &zero0;
    tracker.slow5_record = slow5_rec_init();
    if(tracker.slow5_record == NULL){
        ERROR("%s","Could not allocate space for a slow5 record.");
        return -1;
    }

    tracker.group_name = "";
    tracker.primary_fields_count = &primary_fields_count;

    tracker.warning_map = warning_map;

    herr_t iterator_ret;

    if (fast5_file->is_multi_fast5) {
        hsize_t number_of_groups = 0;
        herr_t h5_get_num_objs = H5Gget_num_objs(fast5_file->hdf5_file,&number_of_groups);
        if(h5_get_num_objs < 0){
            ERROR("Bad fast5: Could not get the number of objects from the fast5 file %s.", tracker.fast5_path);
        }
        tracker.num_read_groups = &number_of_groups;
        //obtain the root group attributes
        iterator_ret = H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &tracker);
        if(iterator_ret<0){
            ERROR("Bad fast5: Could not obtain root group attributes from the fast5 file %s.", tracker.fast5_path);
            return -1;
        }
        //now iterate over read groups. loading records and writing them are done inside fast5_group_itr
        iterator_ret =H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, fast5_group_itr, (void *) &tracker);
        if(iterator_ret < 0){
            ERROR("Bad fast5: Could not iterate over the read groups in the fast5 file %s.", tracker.fast5_path);
            return -1;
        }
    }else{ // single-fast5
        //obtain the root group attributes
        iterator_ret = H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &tracker);
        if(iterator_ret < 0){
            ERROR("Bad fast5: Could not obtain root group attributes from the fast5 file %s.", tracker.fast5_path);
            return -1;
        }
        hsize_t number_of_groups = 1;
        tracker.num_read_groups = &number_of_groups;
        //now iterate over read groups. loading records and writing them are done inside fast5_group_itr
        iterator_ret = H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, fast5_group_itr, (void *) &tracker);
        if(iterator_ret < 0){
            ERROR("Bad fast5: Could not iterate over the read groups in the fast5 file %s.", tracker.fast5_path);
            return -1;
        }
        //        todo: compare header values with the previous singlefast5
        if(*(tracker.flag_run_id) != 1 && *(tracker.flag_run_id_tracking_id) != 1){
            ERROR("Bad fast5: The run_id attribute is missing in fast5 file '%s' for read id '%s'.", tracker.fast5_path, tracker.slow5_record->read_id);
            return -1;
        }
        if(write_header_flag == 0){
            int ret = print_slow5_header(&tracker);
            if(ret < 0){
                return ret;
            }
        }
        if(*(tracker.primary_fields_count) == PRIMARY_FIELD_COUNT){
            int ret = print_record(&tracker);
            if(ret < 0){
                return ret;
            }
            *(tracker.primary_fields_count) = 0;
        }else{
            ERROR("Bad fast5: A primary data field (read_id=%s) is missing in the %s.", tracker.slow5_record->read_id, tracker.fast5_path);
            return -1;
        }
        slow5_rec_free(tracker.slow5_record);
    }
    slow5_press_free(tracker.press_ptr);
    return 1;
}

herr_t fast5_attribute_itr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data){
    hid_t attribute, attribute_type, native_type;
    herr_t return_val = 0;
    htri_t ret_H5Aread = 0;
    htri_t ret = 0;

    struct operator_obj *operator_data = (struct operator_obj *) op_data;
    // Ensure attribute exists
    ret = H5Aexists(loc_id, name);
    if(ret <= 0) {
        ERROR("Bad fast5: In fast5 file %s, the attribute '%s/%s' does not exist on the HDF5 object.\n", operator_data->fast5_path, operator_data->group_name, name);
        return -1;
    }
    attribute = H5Aopen(loc_id, name, H5P_DEFAULT);
    if(attribute < 0){
        ERROR("Bad fast5: In fast5 file %s, failed to open the HDF5 attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
        return -1;
    }
    attribute_type = H5Aget_type(attribute);
    if(attribute_type < 0){
        ERROR("Bad fast5: In fast5 file %s, failed to get the datatype of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
        return -1;
    }
    native_type = H5Tget_native_type(attribute_type, H5T_DIR_ASCEND);
    if(native_type < 0){
        ERROR("Bad fast5: In fast5 file %s, failed to get the native datatype of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
        return -1;
    }

    std::string h5t_class_string = "H5T_STRING";
    H5T_class_t H5Tclass = H5Tget_class(attribute_type);
    if(H5Tclass == -1){
        ERROR("Bad fast5: In fast5 file %s, failed to get the datatype class identifier of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
        return -1;
    }

    union attribute_data value;
    int flag_value_string = 0;
    enum slow5_aux_type slow5_class = SLOW5_STRING;
    std::string slow5_class_string = "SLOW5_STRING";

    switch(H5Tclass){
        case H5T_STRING:
            h5t_class_string = "H5T_STRING";
            slow5_class = SLOW5_STRING;
            slow5_class_string = "SLOW5_STRING";
            flag_value_string = 1;
            if(H5Tis_variable_str(attribute_type) > 0) {
                // variable length string
                ret_H5Aread = H5Aread(attribute, native_type, &value.attr_string);
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
            } else {
                // fixed length string
                size_t storage_size;
                // Get the storage size and allocate
                storage_size = H5Aget_storage_size(attribute);
                value.attr_string = (char*)calloc(storage_size + 1, sizeof(char));

                // finally read the attribute
                ret_H5Aread = H5Aread(attribute, attribute_type, value.attr_string);
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
            }
            if (value.attr_string && !value.attr_string[0]) {
                /*
                std::string key = "em_" + std::string(name); //empty
                char warn_message[300];
                sprintf(warn_message,"Attribute %s/%s is an empty string",operator_data->group_name, name);
                search_and_warn(operator_data,key,warn_message);
                */
                if(flag_value_string){ // hack to skip the free() at the bottom of the function
                    free(value.attr_string);
                    flag_value_string = 0;
                }
                value.attr_string = (char*)"";
            }
            break;
        case H5T_FLOAT:
            if(H5Tequal(H5T_IEEE_F32LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute, native_type, &(value.attr_float));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_IEEE_F32LE=%s\n",name);
                slow5_class = SLOW5_FLOAT;
                slow5_class_string = "SLOW5_FLOAT";
                h5t_class_string = "H5T_IEEE_F32LE";
            }else if(H5Tequal(H5T_IEEE_F64LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute, native_type, &(value.attr_double));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_IEEE_F64LE=%s\n",name);
                slow5_class = SLOW5_DOUBLE;
                slow5_class_string = "SLOW5_DOUBLE";
                h5t_class_string = "H5T_IEEE_F64LE";
            }else{
                ERROR("Found an unexpected float datatype in the attribute %s/%s in %s (expected datatype(s): %s).", operator_data->group_name, name, operator_data->fast5_path, "H5T_IEEE_F32LE,H5T_IEEE_F64LE");
                return -1;
            }
            break;
        case H5T_INTEGER:
            if(H5Tequal(H5T_STD_I32LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute,native_type,&(value.attr_int32_t));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_STD_I32LE=%s\n",name);
                slow5_class = SLOW5_INT32_T;
                slow5_class_string = "SLOW5_INT32_T";
                h5t_class_string = "H5T_STD_I32LE";
            }else if(H5Tequal(H5T_STD_I64LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute,native_type,&(value.attr_int64_t));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_STD_I64LE=%s\n",name);
                slow5_class = SLOW5_INT64_T;
                slow5_class_string = "SLOW5_INT64_T";
                h5t_class_string = "H5T_STD_I64LE";
            }else if(H5Tequal(H5T_STD_U8LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute,native_type,&(value.attr_uint8_t));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_STD_U8LE=%s\n",name);
                slow5_class = SLOW5_UINT8_T;
                slow5_class_string = "SLOW5_UINT8_T";
                h5t_class_string = "H5T_STD_U8LE";
            }else if(H5Tequal(H5T_STD_U32LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute,native_type,&(value.attr_uint32_t));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_STD_U32LE=%s\n",name);
                slow5_class = SLOW5_UINT32_T;
                slow5_class_string = "SLOW5_UINT32_T";
                h5t_class_string = "H5T_STD_U32LE";
            }else if(H5Tequal(H5T_STD_U64LE,native_type)>0){
                ret_H5Aread = H5Aread(attribute,native_type,&(value.attr_uint64_t));
                if(ret_H5Aread < 0){
                    ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                    return -1;
                }
                DEBUG("H5T_STD_U64LE=%s\n",name);
                slow5_class = SLOW5_UINT64_T;
                slow5_class_string = "SLOW5_UINT64_T";
                h5t_class_string = "H5T_STD_U64LE";
            }else{
                ERROR("Found an unexpected integer datatype in the attribute %s/%s in %s (expected datatype(s): %s).", operator_data->group_name, name, operator_data->fast5_path, "H5T_STD_I32LE,H5T_STD_I64LE,H5T_STD_U8LE,H5T_STD_U32LE,H5T_STD_U64LE");
                return -1;
            }
            break;
        case H5T_ENUM:
            ret_H5Aread = H5Aread(attribute,native_type,&(value.attr_uint8_t));
            if(ret_H5Aread < 0){
                ERROR("Bad fast5: In fast5 file %s, failed to get the read the value of the attribute '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                return -1;
            }
            slow5_class = SLOW5_ENUM;
            slow5_class_string = "SLOW5_ENUM";
            h5t_class_string = "H5T_ENUM";
            break;
        default:
            ERROR("Bad fast5: In fast5 file %s, H5TClass of the atttribute %s/%s is 'UNKNOWN'.", operator_data->fast5_path, operator_data->group_name, name);
            return -1;
    }

    std::vector<std::string> enum_labels_list;
    std::vector<const char*> enum_labels_list_ptrs;
    if(H5Tclass==H5T_ENUM && *(operator_data->flag_header_is_written)==0 && *(operator_data->flag_lossy)==0){ //assumption - same run_id has the same order and labels for the enum array
        //https://support.hdfgroup.org/HDF5/doc/H5.user/DatatypesEnum.html
        int n = H5Tget_nmembers(native_type);
        if(n<0){
            ERROR("Bad fast5: In fast5 file %s, H5Tget_nmembers() failed for the attribute %s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
            return -1;
        }
        unsigned u;
        for (u=0; u<(unsigned)n; u++) {
            char *symbol = H5Tget_member_name(native_type, u);
            if(symbol==NULL){
                ERROR("Bad fast5: In fast5 file %s, H5Tget_member_name() failed for the attribute %s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                return -1;
            }
            short val;
            herr_t ret_H5Tget_member_value = H5Tget_member_value(native_type, u, &val);
            if(ret_H5Tget_member_value<0){
                ERROR("Bad fast5: In fast5 file %s, H5Tget_member_name() failed for the attribute %s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
                return -1;
            }
            enum_labels_list.push_back(std::string(symbol));
//            fprintf(stderr,"#%u %20s = %d\n", u, symbol, val);
            free(symbol);
        }
        for (std::string const& str : enum_labels_list) {
            enum_labels_list_ptrs.push_back(str.data());
        }
        if(enum_labels_list_ptrs.size() > UINT8_MAX){
            ERROR("The number of enum (%zu) for the attribute %s/%s in %s exceeds the slow5 enum array size limit (%d)", enum_labels_list_ptrs.size(), operator_data->group_name, name, operator_data->fast5_path, UINT8_MAX);
            return  -1;
        }
    }

    if (strcmp(name,".")==0){
        ERROR("Bad fast5: Name of the attribute '%s/%s' in %s has a prohibited character '%s'.", operator_data->group_name, name, operator_data->fast5_path, name);
        return -1;
    }
    size_t index = 0;
    while(name[index]){
        int result = isspace(name[index]);
        if (result && name[index]!=' '){
            ERROR("Bad fast5: Name of the attribute '%s/%s' in %s  has a white-space character other than a space ('%s')." REPORT_MESG , operator_data->group_name, name, operator_data->fast5_path, name);
            return -1;
        }
        index++;
    }

    int flag_new_group_or_new_attribute_read_group = 1;

    if(H5Tclass==H5T_STRING){
        if (strcmp(value.attr_string,".")==0){
            ERROR("Bad fast5: Attribute '%s/%s' in %s has a prohibited character '.' '%s'.", operator_data->group_name, name, operator_data->fast5_path, value.attr_string);
            return -1;
        }
        size_t index = 0;

        while(value.attr_string[index]){
            int result = isspace(value.attr_string[index]);
            if (result && value.attr_string[index]!=' '){
                ERROR("Bad fast5: Attribute '%s/%s' in %s  has a white-space character other than a space ('%s')." REPORT_MESG, operator_data->group_name, name, operator_data->fast5_path, value.attr_string);
                return -1;
            }
            index++;
        }
    }
    if(strcmp("pore_type",name)==0){
        if(strcmp(value.attr_string,"not_set")!=0){
            ERROR("The value of the attribute %s/%s is expected to be 'not_set', which is '%s' in %s." REPORT_MESG , operator_data->group_name, name, value.attr_string, operator_data->fast5_path);
            return -1;
        }
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tclass!=H5T_STRING){
            type_inconsistency_warn(name, operator_data, h5t_class_string, "H5T_STRING", slow5_class_string, "SLOW5_STRING");
        }
        // no warning required as this is quite common
        // std::string key = "sh_" + std::string(name); //stored in header
        // size_t buf_cap = BUFFER_CAP;
        // char* warn_message = (char*) malloc(buf_cap * sizeof(char));
        // MALLOC_CHK(warn_message);
        // sprintf(warn_message,"The attribute '%s/%s' in %s is empty (not_set) and will be stored in the SLOW5 header", operator_data->group_name, name, operator_data->fast5_path);
        // search_and_warn(operator_data,key,warn_message);
        // free(warn_message);
    }

//            RAW
    else if(strcmp("start_time",name)==0){
        const char* hdf5_expected_type = "H5T_STD_U64LE";
        hid_t hdf5_expected_type_ = H5T_STD_U64LE;
        const char* slow5_expected_type = "SLOW5_UINT64_T";
        slow5_aux_type slow5_expected_type_  = SLOW5_UINT64_T;
        flag_new_group_or_new_attribute_read_group = 0;
        int ret_add_slow5_attribute = add_slow5_auxiliary_attribute(h5t_class_string, slow5_class_string,
                                                                    hdf5_expected_type, hdf5_expected_type_,
                                                                    slow5_expected_type, slow5_expected_type_,
                                                                    native_type, name, operator_data, H5Tclass, value,
                                                                    slow5_class, enum_labels_list_ptrs);
        if(ret_add_slow5_attribute<0){
            ERROR("Could not add the auxiliary attribute %s/%s in %s to the slow5 record", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }

    }
    else if(strcmp("duration",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
//        operator_data->slow5_record->len_raw_signal = value.attr_int;
    }
    else if(strcmp("read_number",name)==0){
        const char* hdf5_expected_type = "H5T_STD_I32LE";
        hid_t hdf5_expected_type_ = H5T_STD_I32LE;
        const char* slow5_expected_type = "SLOW5_INT32_T";
        slow5_aux_type slow5_expected_type_  = SLOW5_INT32_T;
        flag_new_group_or_new_attribute_read_group = 0;
        int ret_add_slow5_attribute = add_slow5_auxiliary_attribute(h5t_class_string, slow5_class_string,
                                                                    hdf5_expected_type, hdf5_expected_type_,
                                                                    slow5_expected_type, slow5_expected_type_,
                                                                    native_type, name, operator_data, H5Tclass, value,
                                                                    slow5_class, enum_labels_list_ptrs);
        if(ret_add_slow5_attribute<0){
            ERROR("Could not add the auxiliary attribute %s/%s in %s to the slow5 record", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }

    }
    else if(strcmp("start_mux",name)==0){
        const char* hdf5_expected_type = "H5T_STD_U8LE";
        hid_t hdf5_expected_type_ = H5T_STD_U8LE;
        const char* slow5_expected_type = "SLOW5_UINT8_T";
        slow5_aux_type slow5_expected_type_  = SLOW5_UINT8_T;
        flag_new_group_or_new_attribute_read_group = 0;
        int ret_add_slow5_attribute = add_slow5_auxiliary_attribute(h5t_class_string, slow5_class_string,
                                                                    hdf5_expected_type, hdf5_expected_type_,
                                                                    slow5_expected_type, slow5_expected_type_,
                                                                    native_type, name, operator_data, H5Tclass, value,
                                                                    slow5_class, enum_labels_list_ptrs);
        if(ret_add_slow5_attribute<0){
            ERROR("Could not add the auxiliary attribute %s/%s in %s to the slow5 record", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }
    }
    else if(strcmp("read_id",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tclass!=H5T_STRING){
            ERROR("The datatype of the attribute %s/%s in %s is %s instead of %s",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_STRING");
            return -1;
        }
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        //make sure read_id has a proper starting character
        if(isalpha(value.attr_string[0]) || isdigit(value.attr_string[0])){
            operator_data->slow5_record->read_id_len = strlen(value.attr_string);
            operator_data->slow5_record->read_id = strdup(value.attr_string);
        }else{
            ERROR("Bad fast5: The attribute %s/%s in %s which is supposed to conform to UUID V4 is malformed (%s).", operator_data->group_name, name, operator_data->fast5_path, value.attr_string);
            return -1;
        }
    }
    else if(strcmp("median_before",name)==0){
        const char* hdf5_expected_type = "H5T_IEEE_F64LE";
        hid_t hdf5_expected_type_ = H5T_IEEE_F64LE;
        const char* slow5_expected_type = "SLOW5_DOUBLE";
        slow5_aux_type slow5_expected_type_  = SLOW5_DOUBLE;
        flag_new_group_or_new_attribute_read_group = 0;
        int ret_add_slow5_attribute = add_slow5_auxiliary_attribute(h5t_class_string, slow5_class_string,
                                                                    hdf5_expected_type, hdf5_expected_type_,
                                                                    slow5_expected_type, slow5_expected_type_,
                                                                    native_type, name, operator_data, H5Tclass, value,
                                                                    slow5_class, enum_labels_list_ptrs);
        if(ret_add_slow5_attribute<0){
            ERROR("Could not add the auxiliary attribute %s/%s in %s to the slow5 record", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }

    }
    else if(strcmp("end_reason",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tclass!=H5T_ENUM && H5Tequal(H5T_STD_U8LE,native_type)<=0) {
            ERROR("The datatype of the attribute %s/%s in %s is %s instead of %s",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_ENUM or H5T_STD_U8LE");
            return -1;
        }
        if(H5Tequal(H5T_STD_U8LE,native_type)>0) {
            ERROR("Attribute %s/%s in %s is corrupted (datatype %s instead of expected %s). This is a known issue in ont_fast5_api's compress_fast5 (see https://github.com/hasindu2008/slow5tools/issues/59 and https://github.com/nanoporetech/ont_fast5_api/issues/70).\n Please get your FAST5 files fixed before SLOW5 coversion, by bugging ONT through GitHub issues.",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_ENUM");
            return -1;
            // std::string key = "sh_" + std::string(name); //stored in header
            // size_t buf_cap = BUFFER_CAP;
            // char* warn_message = (char*) malloc(buf_cap * sizeof(char));
            // MALLOC_CHK(warn_message);
            // sprintf(warn_message,"Attribute %s/%s in %s is corrupted (datatype %s instead of expected %s). This is a known issue in ont_fast5_api's compress_fast5 (see https://github.com/hasindu2008/slow5tools/issues/59 and https://github.com/nanoporetech/ont_fast5_api/issues/70).\nCorrupted attribute will be dumped as it is, but would cause issues when merging. It is recommended that you fix your FAST5 files before SLOW5 coversion, by bugging ONT through GitHub issues",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_ENUM");
            // search_and_warn(operator_data,key,warn_message);
            // free(warn_message);
        }
        if(add_aux_slow5_attribute(name, operator_data, H5Tclass, value, slow5_class, enum_labels_list_ptrs) == -1) {
            ERROR("Could not add the auxiliary attribute %s/%s in %s to the slow5 record", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }
    }
//            CHANNEL_ID
    else if(strcmp("channel_number",name)==0){
        const char* hdf5_expected_type = "H5T_STRING";
        hid_t hdf5_expected_type_ = H5T_STRING;
        const char* slow5_expected_type = "SLOW5_STRING";
        slow5_aux_type slow5_expected_type_  = SLOW5_STRING;
        flag_new_group_or_new_attribute_read_group = 0;
        int ret_add_slow5_attribute = add_slow5_auxiliary_attribute(h5t_class_string, slow5_class_string,
                                                                    hdf5_expected_type, hdf5_expected_type_,
                                                                    slow5_expected_type, slow5_expected_type_,
                                                                    native_type, name, operator_data, H5Tclass, value,
                                                                    slow5_class, enum_labels_list_ptrs);
        if(ret_add_slow5_attribute<0){
            ERROR("Could not add the auxiliary attribute %s/%s in %s to the slow5 record", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }

    }
    else if(strcmp("digitisation",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tequal(H5T_IEEE_F64LE,native_type)==0){
            ERROR("The datatype of the attribute %s/%s in %s is %s instead of %s",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_IEEE_F64LE");
            return -1;
        }
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->digitisation = value.attr_double;
    }
    else if(strcmp("offset",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tequal(H5T_IEEE_F64LE,native_type)==0){
            ERROR("The datatype of the attribute %s/%s in %s is %s instead of %s",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_IEEE_F64LE");
            return -1;
        }
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->offset = value.attr_double;
    }
    else if(strcmp("range",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tequal(H5T_IEEE_F64LE,native_type)==0){
            ERROR("The datatype of the attribute %s/%s in %s is %s instead of %s",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_IEEE_F64LE");
            return -1;
        }
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->range = value.attr_double;
    }
    else if(strcmp("sampling_rate",name)==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if(H5Tequal(H5T_IEEE_F64LE,native_type)==0){
            ERROR("The datatype of the attribute %s/%s in %s is %s instead of %s",operator_data->group_name, name, operator_data->fast5_path, h5t_class_string.c_str(), "H5T_IEEE_F64LE");
            return -1;
        }
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->sampling_rate = value.attr_double;
    }

    // if group is ROOT or CONTEXT_TAGS or TRACKING_ID or attribute is 'run_id' or 'pore_type' create an attribute in the header and store value
    if(strcmp(name,"pore_type")==0 || strcmp(name,"run_id")==0 || strcmp(operator_data->group_name,"")==0 || strcmp(operator_data->group_name,"context_tags")==0 || strcmp(operator_data->group_name,"tracking_id")==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if (H5Tclass != H5T_STRING) {
            flag_value_string = 1;
            size_t storage_size = 50;
            char* value_string_temp = (char *) calloc(storage_size + 1, sizeof(char));
            switch (H5Tclass) {
                case H5T_FLOAT:
                    if(H5Tequal(H5T_IEEE_F32LE,native_type)>0){
                        sprintf(value_string_temp, "%.1f", value.attr_float);
                    }else if(H5Tequal(H5T_IEEE_F64LE,native_type)>0){
                        sprintf(value_string_temp, "%.1f", value.attr_double);
                    }else{
                        ERROR("Found an unexpected float datatype in the attribute %s/%s in %s (expected datatype(s): %s).", operator_data->group_name, name, operator_data->fast5_path, "H5T_IEEE_F32LE,H5T_IEEE_F64LE");
                        return -1;
                    }
                    value.attr_string = value_string_temp;
                    flag_value_string = 1;
                    break;
                case H5T_INTEGER:
                    if(H5Tequal(H5T_STD_I32LE,native_type)>0){
                        sprintf(value_string_temp, "%" PRId32 "", value.attr_int32_t);
                    }else if(H5Tequal(H5T_STD_I64LE,native_type)>0){
                        sprintf(value_string_temp, "%" PRId64 "", value.attr_int64_t);
                    }else if(H5Tequal(H5T_STD_U8LE,native_type)>0){
                        sprintf(value_string_temp, "%" PRIu8 "", value.attr_uint8_t);
                    }else if(H5Tequal(H5T_STD_U32LE,native_type)>0){
                        sprintf(value_string_temp, "%" PRIu32 "", value.attr_uint32_t);
                    }else if(H5Tequal(H5T_STD_U64LE,native_type)>0){
                        sprintf(value_string_temp, "%" PRIu64 "", value.attr_uint64_t);
                    }else{
                        ERROR("Found an unexpected integer datatype in the attribute %s/%s in %s (expected datatype(s): %s).", operator_data->group_name, name, operator_data->fast5_path, "H5T_STD_I32LE,H5T_STD_I64LE,H5T_STD_U8LE,H5T_STD_U32LE,H5T_STD_U64LE");
                        return -1;
                    }
                    value.attr_string = value_string_temp;
                    flag_value_string = 1;
                    break;
                case H5T_ENUM:
                    sprintf(value_string_temp, "%u", value.attr_uint8_t);
                    value.attr_string = value_string_temp;
                    flag_value_string = 1;
                    break;
                default:
                    ERROR("%s", "Something impossible just happened. The code should never have reached here.");
                    return -1;
            }
            std::string key = "co_" + std::string(name); //convert
            size_t buf_cap = BUFFER_CAP;
            char* warn_message = (char*) malloc(buf_cap * sizeof(char));
            MALLOC_CHK(warn_message);
            sprintf(warn_message,"Weird or ancient fast5: converting the attribute %s/%s from %s to string for consitency",operator_data->group_name, name, h5t_class_string.c_str());
            search_and_warn(operator_data,key,warn_message);
            free(warn_message);
        }
        int flag_attribute_exists = 0;
        char* existing_attribute;
        int flag_existing_attr_value_mismatch = 0;

        ret = slow5_hdr_add_attr(name, operator_data->slow5File->header);
        if(ret == -1){
            ERROR("Could not add the header attribute '%s/%s'. Internal error occured.", operator_data->group_name, name);
            return -1;
        } else if(ret == -2){
            flag_attribute_exists = 1;
//            WARNING("Attribute '%s/%s' is already added to the header.", operator_data->group_name, name);
            existing_attribute = slow5_hdr_get("run_id", 0, operator_data->slow5File->header);
            if(strcmp(existing_attribute,value.attr_string)){
                flag_existing_attr_value_mismatch = 1;
            }
        } else if(ret == -3){
            ERROR("Could not add the header attribute '%s/%s'. Internal error occured.", operator_data->group_name, name);
            return -1;
        }

        if(flag_attribute_exists==0){
            ret = slow5_hdr_set(name, value.attr_string, 0, operator_data->slow5File->header);
            if(ret == -1){
                ERROR("Could not set the header attribute '%s/%s' value to %s.", operator_data->group_name, name, value.attr_string);
                return -1;
            }
        }

        if(strcmp("run_id",name)==0){
            *(operator_data->flag_run_id) = 1;
            if(flag_existing_attr_value_mismatch){
                if(*(operator_data->flag_allow_run_id_mismatch)){
                    std::string key = "rm_" + std::string(name); //runid mismatch
                    size_t buf_cap = BUFFER_CAP;
                    char* warn_message = (char*) malloc(buf_cap * sizeof(char));
                    MALLOC_CHK(warn_message);
                    sprintf(warn_message,"Ancient fast5: Different run_ids found in an individual multi-fast5 file. First seen run_id will be set in slow5 header");
                    search_and_warn(operator_data,key,warn_message);
                    free(warn_message);

                }else{
                    ERROR("Ancient fast5: Different run_ids found in an individual multi-fast5 file. Cannot create a single header slow5/blow5. Consider --allow option.%s", "");
                    return -1;
                }
            }
        }else if(flag_existing_attr_value_mismatch && *(operator_data->flag_header_is_written)==0){
            ERROR("Bad fast5: Attribute %s/%s in %s is duplicated and has two different values in two different places." REPORT_MESG , operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }
    }

    if(flag_new_group_or_new_attribute_read_group){
        if(*(operator_data->flag_dump_all)==1){
            if(add_aux_slow5_attribute(name, operator_data, H5Tclass, value, slow5_class, enum_labels_list_ptrs) == -1){
                return -1;
            }
            std::string key = "at_" + std::string(name); //Alert
            size_t buf_cap = BUFFER_CAP;
            char* warn_message = (char*) malloc(buf_cap * sizeof(char));
            MALLOC_CHK(warn_message);
            sprintf(warn_message,"Weird fast5: Attribute %s/%s in %s is unexpected", operator_data->group_name, name, operator_data->fast5_path);
            search_and_warn(operator_data,key,warn_message);
            free(warn_message);
        }else if(strcmp(name,"file_version")!=0 && !(strcmp(operator_data->group_name,"Raw")==0 && strcmp(name,"num_minknow_events")==0)){
            ERROR("Bad fast5: Attribute %s/%s in %s is unexpected." REPORT_MESG , operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }

        if(strcmp(name, "file_version")==0){
            int flag_attribute_exists = 0;
            ret = slow5_hdr_add_attr(name, operator_data->slow5File->header);
            if(ret == -1 || ret == -3){
                ERROR("Could not add the header attribute '%s/%s'. Internal error occured.", operator_data->group_name, name);
                return -1;
            } else if(ret == -2){
                flag_attribute_exists = 1;
            }

            if(flag_attribute_exists==0){
                ret = slow5_hdr_set(name, value.attr_string, 0, operator_data->slow5File->header);
                if(ret == -1){
                    ERROR("Could not set the header attribute '%s/%s' value to %s.", operator_data->group_name, name, value.attr_string);
                    return -1;
                }
            }

        }

    }

    if(flag_value_string){
        free(value.attr_string);
    }

    //close attributes
    H5Tclose(native_type);
    H5Tclose(attribute_type);
    H5Aclose(attribute);

    return return_val;
}

int add_slow5_auxiliary_attribute(std::string h5t_class_string, std::string slow5_class_string, const char *hdf5_expected_type, hid_t hdf5_expected_type_, const char *slow5_expected_type,
                                  slow5_aux_type slow5_expected_type_, hid_t native_type, const char *name,
                                  operator_obj *operator_data, H5T_class_t H5Tclass, attribute_data value,
                                  slow5_aux_type slow5_class, std::vector<const char *> enum_labels_list_ptrs) {
    htri_t ret_equal = H5Tequal(hdf5_expected_type_,native_type);
    if(ret_equal==0){
        type_inconsistency_warn(name, operator_data, h5t_class_string, hdf5_expected_type, slow5_class_string, slow5_expected_type);
        int ret_convert_to_correct_datatype = convert_to_correct_datatype(value, slow5_class, slow5_expected_type_);
        if(ret_convert_to_correct_datatype==0){
            ERROR("Could not convert attribute %s/%s in %s from %s to %s", operator_data->group_name, name, operator_data->fast5_path, slow5_class_string.c_str(), slow5_expected_type)
            return -1;
        }
        slow5_class = slow5_expected_type_;
        slow5_class_string = slow5_expected_type;
    }else if(ret_equal<0 && strcmp(h5t_class_string.c_str(), "H5T_STRING") && strcmp(slow5_class_string.c_str(), "SLOW5_STRING")){
        ERROR("Could not convert attribute %s/%s in %s from %s to %s", operator_data->group_name, name, operator_data->fast5_path, slow5_class_string.c_str(), slow5_expected_type)
        return -1;
    }
    if(add_aux_slow5_attribute(name, operator_data, H5Tclass, value, slow5_class, enum_labels_list_ptrs) == -1){
        return -1;
    }
    return 0;
}

//if convertible then 1 else 0
int convert_to_correct_datatype(attribute_data data, slow5_aux_type from_type, slow5_aux_type to_type) {
    if(from_type == SLOW5_UINT8_T && to_type == SLOW5_INT32_T){
        return 1;
    }
    if(from_type == SLOW5_UINT8_T && to_type == SLOW5_UINT32_T){
        return 1;
    }
    if(from_type == SLOW5_UINT8_T && to_type == SLOW5_UINT64_T){
        return 1;
    }
    if(from_type == SLOW5_INT32_T && to_type == SLOW5_UINT8_T){
        return data.attr_int32_t > 0 && data.attr_int32_t < UINT8_MAX;
    }
    if(from_type == SLOW5_INT32_T && to_type == SLOW5_UINT32_T){
        return data.attr_int32_t > 0;
    }
    if(from_type == SLOW5_INT32_T && to_type == SLOW5_UINT64_T){
        return data.attr_int32_t > 0;
    }
    if(from_type == SLOW5_INT64_T && to_type == SLOW5_UINT8_T){
        return data.attr_int64_t > 0 && data.attr_int64_t < UINT8_MAX;
    }
    if(from_type == SLOW5_INT64_T && to_type == SLOW5_INT32_T){
        return data.attr_int64_t < INT32_MAX;
    }
    if(from_type == SLOW5_INT64_T && to_type == SLOW5_UINT64_T){
        return data.attr_int64_t > 0;
    }
    if(from_type == SLOW5_UINT32_T && to_type == SLOW5_UINT8_T){
        return data.attr_uint32_t < UINT8_MAX;
    }
    if(from_type == SLOW5_UINT32_T && to_type == SLOW5_INT32_T){
        return 1;
    }
    if(from_type == SLOW5_UINT32_T && to_type == SLOW5_UINT64_T){
        return 1;
    }
    if(from_type == SLOW5_UINT64_T && to_type == SLOW5_UINT8_T){
        return data.attr_uint64_t < UINT8_MAX;
    }
    if(from_type == SLOW5_UINT64_T && to_type == SLOW5_INT32_T){
        return 1;
    }
    if(from_type == SLOW5_UINT64_T && to_type == SLOW5_UINT32_T){
        return 1;
    }
    return 0;
}

void type_inconsistency_warn(const char *name, operator_obj *operator_data, std::string h5t_class_string, const char *expected_type, std::string slow5_class, const char *slow5_expected_type) {
    std::string key = "type_inconsistent_" + std::string(name); //type inconsistency
    size_t buf_cap = BUFFER_CAP;
    char* warn_message = (char*) malloc(buf_cap * sizeof(char));
    MALLOC_CHK(warn_message);
    sprintf(warn_message,"Weird or ancient fast5: converting the attribute %s/%s from %s to %s for consitency",operator_data->group_name, name, h5t_class_string.c_str(),slow5_expected_type);
    search_and_warn(operator_data,key,warn_message);
    free(warn_message);
}

void search_and_warn(operator_obj *operator_data, std::string key, const char *warn_message) {
    auto search = operator_data->warning_map->find(key);
    if (search != operator_data->warning_map->end()) {
        if(search->second < WARNING_LIMIT){
            search->second = search->second+1;
            WARNING("slow5tools-v%s: %s.", SLOW5TOOLS_VERSION, warn_message);
        }else if(search->second == WARNING_LIMIT){
            WARNING("slow5tools-v%s: %s. This warning is suppressed now onwards.",SLOW5TOOLS_VERSION, warn_message);
            search->second = WARNING_LIMIT+1;
        }
    } else {
        WARNING("slow5tools-v%s: %s.",SLOW5TOOLS_VERSION, warn_message);
        operator_data->warning_map->insert({key,1});
    }
}


int add_aux_slow5_attribute(const char *name, operator_obj *operator_data, H5T_class_t h5TClass, attribute_data value, enum slow5_aux_type slow5_type, std::vector<const char *> enum_labels_list_ptrs) {
    int failed = 0;
    if(*(operator_data->flag_lossy)==1){
        return 0;
    }

    if(*(operator_data->flag_header_is_written)==0){
        if(slow5_type == SLOW5_ENUM){
            if(slow5_aux_meta_add_enum(operator_data->slow5File->header->aux_meta, name, slow5_type, enum_labels_list_ptrs.data(), enum_labels_list_ptrs.size())){
                failed = 1;
            }
        }
        else{
            if(slow5_aux_meta_add(operator_data->slow5File->header->aux_meta, name, slow5_type)){
                failed = 1;
            }
        }
        if(failed){
            ERROR("Could not initialize the record attribute '%s/%s' in %s", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }
    }
    uint32_t attribute_index;
    if(operator_data->slow5File->header->aux_meta && check_aux_fields_in_header(operator_data->slow5File->header, name, 0, &attribute_index) == 0){
        if(slow5_type == SLOW5_ENUM){
            if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, &value.attr_uint8_t) != 0) {
                failed = 1;
            }
        }
        else if(slow5_type == SLOW5_STRING) {
            if (slow5_rec_set_string(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, value.attr_string) != 0) {
                failed = 1;
            }
        }
        else if(slow5_type == SLOW5_DOUBLE){
            if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, &value.attr_double) != 0) {
                failed = 1;
            }
        }
        else if(slow5_type == SLOW5_UINT8_T){
            if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, &value.attr_uint8_t) != 0) {
                failed = 1;
            }
        }
        else if(slow5_type == SLOW5_INT32_T){
            if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, &value.attr_int32_t) != 0) {
                failed = 1;
            }
        }
        else if(slow5_type == SLOW5_UINT32_T){
            if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, &value.attr_uint32_t) != 0) {
                failed = 1;
            }
        }
        else if(slow5_type == SLOW5_UINT64_T){
            if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, name, &value.attr_uint64_t) != 0) {
                failed = 1;
            }
        }
        else {
            failed = 1;
        }
        if(failed){
            ERROR("Could not set the slow5 record auxiliary attribute '%s/%s' in %s", operator_data->group_name, name, operator_data->fast5_path);
            return -1;
        }

    }else if(*(operator_data->flag_header_is_written) == 1 && operator_data->slow5File->header->aux_meta && check_aux_fields_in_header(operator_data->slow5File->header, name, 0, NULL) == -1){
        std::string key = "nh_" + std::string(name); //not stored in header
        size_t buf_cap = BUFFER_CAP;
        char* warn_message = (char*) malloc(buf_cap * sizeof(char));
        MALLOC_CHK(warn_message);
        sprintf(warn_message,"%s auxiliary attribute is not set in the slow5 header", name);
        search_and_warn(operator_data,key,warn_message);
        // WARNING("%s auxiliary attribute is not set in the slow5 header", name);
        free(warn_message);
    }
    return 0;
}


int read_dataset(hid_t loc_id, const char *name, slow5_rec_t* slow5_record) {

    hid_t dset = H5Dopen(loc_id, name, H5P_DEFAULT);
    if (dset < 0) {
        WARNING("Bad fast5: Failed to open dataset '%s' to read raw signal.", name);
        return -1;
        // goto cleanup2;
    }

    hid_t space;
    hsize_t h5_nsample;
    space = H5Dget_space(dset);
    if (space < 0) {
        WARNING("Bad fast5: Failed to create copy of dataspace for raw signal %s.",
                name);
        H5Dclose(dset);
        return -1;
        // goto cleanup3;
    }

    int32_t ret1 = H5Sget_simple_extent_dims(space, &h5_nsample, NULL);
    if (ret1 < 0) {
        WARNING("Bad fast5: Failed to get the dataspace dimension for raw signal %s.", name);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }
    slow5_record->len_raw_signal = h5_nsample;
    slow5_record->raw_signal = (int16_t *) malloc(h5_nsample * sizeof *(slow5_record->raw_signal));
    hid_t status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, slow5_record->raw_signal);

    if (status < 0) {
        hid_t dcpl = H5Dget_create_plist (dset);
        unsigned int flags;
        size_t nelmts = 1; //number of elements in cd_values
        unsigned int values_out[1] = {99};
        char filter_name[80];
        H5Z_filter_t filter_id = H5Pget_filter2 (dcpl, (unsigned) 0, &flags, &nelmts, values_out, sizeof(filter_name) - 1, filter_name, NULL);
        H5Pclose (dcpl);
        if(filter_id == H5Z_FILTER_VBZ){
            ERROR("The fast5 file is compressed with VBZ but the required plugin is not loaded. Please read the instructions here: https://hasindu2008.github.io/slow5tools/faq.html%s","");
        }
        ERROR("Failed to read raw data from dataset %s.", name);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }
    H5Sclose(space);
    H5Dclose(dset);
    return 0;
}



/************************************************************

  Operator function. If the object is a group, it
  is first checked against other groups in its path using
  the group_check function, then if it is not a duplicate,
  H5Literate is called for that group.  This guarantees that
  the program will not enter infinite recursion due to a
  circular path in the file.

 ************************************************************/
herr_t fast5_group_itr (hid_t loc_id, const char *name, const H5L_info_t *info, void *opdata){

    herr_t return_val = 0;
    H5O_info_t infobuf;
    struct operator_obj *operator_data = (struct operator_obj *) opdata;
    /* Type conversion */
    unsigned spaces = 2*(operator_data->group_level + 1);

    // if group is 'Analyses'; then skip
    if(strcmp(name,"Analyses")==0){
        return return_val;
    }
    // if group is 'PreviousReadInfo'; then skip
    if(strcmp(name,"PreviousReadInfo")==0){
        return return_val;
    }
    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library.
     */
    herr_t ret_H5Oget_info_by_name = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    if(ret_H5Oget_info_by_name<0){
        ERROR("Bad fast5: In fast5 file %s, failed to get the information of the HDF5 object '%s/%s'.", operator_data->fast5_path, operator_data->group_name, name);
        return -1;
    }
    switch (infobuf.type) {
        case H5O_TYPE_GROUP:

            /*
             * Check group address against linked list of operator
             * data structures.  We will always run the check, as the
             * reference count cannot be relied upon if there are
             * symbolic links, and H5Oget_info_by_name always follows
             * symbolic links.  Alternatively we could use H5Lget_info
             * and never recurse on groups discovered by symbolic
             * links, however it could still fail if an object's
             * reference count was manually manipulated with
             * H5Odecr_refcount.
             */
            if (group_check(operator_data, infobuf.addr) ) {
                WARNING("%*s  Weird fast5: Loop detected!\n", spaces, "");
            }
            else {
                //@ group number of attributes
                hid_t group = H5Gopen(loc_id, name, H5P_DEFAULT);
                /*
                 * Initialize new operator data structure and
                 * begin recursive iteration on the discovered
                 * group.  The new operator_obj structure is given a
                 * pointer to the current one.
                 */
                struct operator_obj next_op = *operator_data;
                next_op.group_level = operator_data->group_level + 1;
                next_op.prev = operator_data;
                next_op.addr = infobuf.addr;
                next_op.group_name = name;

                //traverse the attributes belonging to the group
                //tracking_id and context_tags groups should be traversed only for the first read

                if (operator_data->fast5_file->is_multi_fast5) {
                    if (strcmp(name, "tracking_id") == 0) {
                        // Ensure attribute exists
                        herr_t ret_run_id = H5Aexists(group, "run_id");
                        *(operator_data->flag_run_id_tracking_id) = ret_run_id;
                    }
                    if (strcmp(name, "tracking_id") == 0){// && *(operator_data->flag_tracking_id) == 0) {
                        return_val = H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                        *(operator_data->flag_tracking_id) = 1;
                    } else if (strcmp(name, "context_tags") == 0){// && *(operator_data->flag_context_tags) == 0) {
                        return_val = H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                        *(operator_data->flag_context_tags) = 1;
                    } else if (strcmp(name, "tracking_id") != 0 && strcmp(name, "context_tags") != 0) {
                        return_val = H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                    }
                }else{
                    return_val = H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                }
                if(return_val < 0){
                    return return_val;
                }

                H5Gclose(group);
                //the recursive call
                return_val = H5Literate_by_name(loc_id, name, H5_INDEX_NAME, H5_ITER_INC, 0, fast5_group_itr,
                                                (void *) &next_op, H5P_DEFAULT);
                if(return_val < 0){
                    return return_val;
                }
                if (operator_data->fast5_file->is_multi_fast5) {
                    //check if we are at a root-level group
                    if (operator_data->group_level == ROOT) {
                        if (*(operator_data->nreads) == 0) {
                            if (*(operator_data->flag_context_tags) != 1) {
                                ERROR("Bad fast5: The first read in the multi-fast5 does not have context_tags information%s", ".");
                                return -1;
                            }
                            if (*(operator_data->flag_tracking_id) != 1) {
                                ERROR("Bad fast5: The first read in the multi-fast5 does not have tracking_id information%s", ".");
                                return -1;
                            }
                            if(*(operator_data->flag_run_id) != 1 && *(operator_data->flag_run_id_tracking_id) != 1){
                                ERROR("Bad fast5: run_id information of the read %s in %s cannot be found.", operator_data->slow5_record->read_id, operator_data->fast5_path);
                                return -1;
                            }
                            if(*(operator_data->flag_header_is_written) == 0){
                                int ret = print_slow5_header(operator_data);
                                if(ret < 0){
                                    return ret;
                                }
                            }
                        }
                        *(operator_data->nreads) = *(operator_data->nreads) + 1;
                        if(*(operator_data->primary_fields_count) == PRIMARY_FIELD_COUNT){
                            if(*(operator_data->flag_run_id) != 1 && *(operator_data->flag_run_id_tracking_id) != 1){
                                ERROR("Bad fast5: run_id information of the read %s in %s cannot be found.", operator_data->slow5_record->read_id, operator_data->fast5_path);
                                return -1;
                            }
                            if(*(operator_data->flag_run_id) != 1){
                                std::string key = "pri_" + std::string("run_id"); //primary attribute run id not found in the read group
                                size_t buf_cap = BUFFER_CAP;
                                char* warn_message = (char*) malloc(buf_cap * sizeof(char));
                                MALLOC_CHK(warn_message);
                                sprintf(warn_message,"primary attribute run_id is not found in the read %s group", operator_data->slow5_record->read_id);
                                search_and_warn(operator_data,key,warn_message);
//                                WARNING("run_id is missing in the %s in read_id %s group.", operator_data->fast5_path, operator_data->slow5_record->read_id);
                                free(warn_message);
                            }
                            int ret = print_record(operator_data);
                            if(ret < 0){
                                return ret;
                            }
                            *(operator_data->primary_fields_count) = 0;
                            *(operator_data->flag_run_id) = 0;
                            *(operator_data->flag_run_id_tracking_id) = 0;
                        }else{
                            ERROR("Bad fast5: A primary attribute is missing in the %s.", operator_data->fast5_path);
                            return -1;
                        }
                        slow5_rec_free(operator_data->slow5_record);
                        if(*(operator_data->nreads) < *(operator_data->num_read_groups)){
                            operator_data->slow5_record = slow5_rec_init();
                        }
                    }
                }
            }
            break;
        case H5O_TYPE_DATASET:
            return_val = read_dataset(loc_id, name, operator_data->slow5_record);
            if(return_val < 0){
                return return_val;
            }
            *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 2;
            break;
        case H5O_TYPE_NAMED_DATATYPE:
            WARNING("Weird fast5: Datatype %s in %s is unexpected", name, operator_data->fast5_path);
            break;
        default:
            WARNING("Weird fast5: %s in %s is unexpected", name, operator_data->fast5_path);
    }
    return return_val;
}

/************************************************************

  This function recursively searches the linked list of
  hdf5group structures for one whose address matches
  target_addr.  Returns 1 if a match is found, and 0
  otherwise.

 ************************************************************/
int group_check(struct operator_obj *od, haddr_t target_addr){
    if (od->addr == target_addr)
        return 1;       /* Addresses match */
    else if (!od->group_level)
        return 0;       /* Root group reached with no matches */
    else
        return group_check(od->prev, target_addr);
    /* Recursively examine the next node */
}

int check_for_similar_file_names(std::vector<std::string> file_list) {
    std::set<std::string> file_set;
    for(size_t i=0; i<file_list.size(); i++){
        int last_slash = file_list[i].find_last_of('/');
        if(last_slash == -1){
            last_slash = 0;
        }
        int extension_length = 5; //fast5 or blow5 or slow5
        std::string file_name = file_list[i].substr(last_slash,file_list[i].length() - last_slash - extension_length);
        file_set.insert(file_name);
    }
    return !(file_list.size()==file_set.size());
}

//return 0; created dir
//return -1; dir exists
//return -2; could not create dir
int create_dir(const char *dir_name) {
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        int ret_mkdir = mkdir(dir_name, 0700);
        if(ret_mkdir == -1){
            return -2;
        }
    }else{
        std::vector< std::string > dir_list = list_directory(dir_name);
        if(dir_list.size()>2){
            return -1;
        }
    }
    return 0;
}



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
        DEBUG("could not open attribute: %s", attribute_name.c_str());
#endif
        goto close_attr;
    }

    // Get data type and check it is a fixed-length string
    attribute_type = H5Aget_type(attribute);
    if(attribute_type < 0) {
#ifdef DEBUG_FAST5_IO
        DEBUG("failed to get attribute type %s", attribute_name.c_str());
#endif
        goto close_type;
    }

    if(H5Tget_class(attribute_type) != H5T_STRING) {
#ifdef DEBUG_FAST5_IO
        DEBUG("attribute %s is not a string", attribute_name.c_str());
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
            ERROR("Bad fast5: error reading attribute %s", attribute_name.c_str());
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

#endif
