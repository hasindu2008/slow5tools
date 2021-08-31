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
#include "read_fast5.h"
#include "cmd.h"

#define WARNING_LIMIT 1
#define PRIMARY_FIELD_COUNT 7 //without read_group number
#define H5Z_FILTER_VBZ 32020 //We need to find out what the numerical value for this is

// Operator function to be called by H5Aiterate.
herr_t fast5_attribute_itr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data);
// Operator function to be called by H5Literate.
herr_t fast5_group_itr (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);

// from nanopolish_fast5_io.cpp
static inline  std::string fast5_get_string_attribute(fast5_file_t fh, const std::string& group_name, const std::string& attribute_name);

//other functions
int group_check(struct operator_obj *od, haddr_t target_addr);

void search_and_warn(operator_obj *operator_data, std::string key, const char *warn_message);

int print_slow5_header(operator_obj* operator_data) {
    if(slow5_hdr_fwrite(operator_data->slow5File->fp, operator_data->slow5File->header, operator_data->format_out, operator_data->pressMethod) == -1){
        ERROR("%s","Could not write the slow5 header");
        return -1;
    }
    *(operator_data->flag_header_is_written) = 1;
    return 0;
}

int print_record(operator_obj* operator_data) {
    if(slow5_rec_fwrite(operator_data->slow5File->fp, operator_data->slow5_record, operator_data->slow5File->header->aux_meta, operator_data->format_out, operator_data->press_ptr) == -1){
        ERROR("Could not write the slow5 record %s", operator_data->slow5_record->read_id);
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
                ERROR("Could not parse the fast5 version string %s", version_str.c_str());
                exit(EXIT_FAILURE);
            }

            fh.is_multi_fast5 = major >= 1;
        }
    }
    return fh;
}

herr_t fast5_attribute_itr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data){
    hid_t attribute, attribute_type, native_type;
    herr_t return_val = 0;
    int ret = 0;

    struct operator_obj *operator_data = (struct operator_obj *) op_data;
    // Ensure attribute exists
    ret = H5Aexists(loc_id, name);
    if(ret <= 0) {
        WARNING("attribute %s not found\n", name);
        return -1;
    }

    attribute = H5Aopen(loc_id, name, H5P_DEFAULT);
    attribute_type = H5Aget_type(attribute);
    native_type = H5Tget_native_type(attribute_type, H5T_DIR_ASCEND);
    H5T_class_t H5Tclass = H5Tget_class(attribute_type);

    union attribute_data value;
    int flag_value_string = 0;
    std::string h5t_class = "H5T_STRING";
    switch(H5Tclass){
        case H5T_STRING:
            flag_value_string = 1;
            if(H5Tis_variable_str(attribute_type) > 0) {
                // variable length string
                ret = H5Aread(attribute, native_type, &value.attr_string);
                if(ret < 0) {
                    ERROR("error reading attribute %s", name);
                    return -1;
                }
            } else {
                // fixed length string
                size_t storage_size;
                // Get the storage size and allocate
                storage_size = H5Aget_storage_size(attribute);
                value.attr_string = (char*)calloc(storage_size + 1, sizeof(char));

                // finally read the attribute
                ret = H5Aread(attribute, attribute_type, value.attr_string);

                if(ret < 0) {
                    ERROR("error reading attribute %s", name);
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
            h5t_class = "H5T_FLOAT";
            H5Aread(attribute, native_type, &(value.attr_double));
            break;
        case H5T_INTEGER:
            h5t_class = "H5T_INTEGER";
            H5Aread(attribute,native_type,&(value.attr_int));
            break;
        case H5T_ENUM:
            h5t_class = "H5T_ENUM";
            H5Aread(attribute, native_type, &(value.attr_uint8_t));
            break;
        default:
            h5t_class = "UNKNOWN";
            ERROR("f2s cannot handle H5TClass of the atttribute %s/%s.  This is something we haven't seen before. Please open a github issue with an example of the fast5 file so we can implement special handling of such attributes.", operator_data->group_name, name);
            return -1;
    }
    H5Tclose(native_type);
    H5Tclose(attribute_type);
    H5Aclose(attribute);

    int flag_new_group_or_new_attribute_read_group = 1;

    if(H5Tclass==H5T_STRING){
        if (strcmp(value.attr_string,".")==0){
            ERROR("Attribute '%s' in %s has '%s' as a value which is reserved in slow5 for representing empty fields. This is something we haven't seen before. Please open a github issue with an example of the fast5 file so we can implement special handling of such attributes.", name, operator_data->fast5_path, value.attr_string);
            return -1;
        }
        size_t index = 0;

        while(value.attr_string[index]){
            int result = isspace(value.attr_string[index]);
            if (result && value.attr_string[index]!=' '){
                ERROR("Attribute '%s' in %s has a value '%s' with only a single white space. This is something we haven't seen before. Please open a github issue with an example of the fast5 file so we can implement special handling of such attributes.", name, operator_data->fast5_path, value.attr_string);
                return -1;
            }
            index++;
        }
    }
    if(strcmp("pore_type",name)==0 && H5Tclass==H5T_STRING){
        flag_new_group_or_new_attribute_read_group = 0;
        std::string key = "ns_" + std::string(name); //not stored
        char warn_message[300];
        sprintf(warn_message,"Not stored: Attribute read/pore_type is not stored because it is empty");
        search_and_warn(operator_data,key,warn_message);
    }

//            RAW
    else if(strcmp("start_time",name)==0 && H5Tclass==H5T_INTEGER && *(operator_data->flag_lossy) == 0){
        if(*(operator_data->flag_header_is_written)==0 && *(operator_data->flag_lossy)==0){
            if(slow5_aux_meta_add(operator_data->slow5File->header->aux_meta, "start_time", SLOW5_UINT64_T)){
                ERROR("Could not initialize the record attribute '%s'", "start_time");
                return -1;
            }
        }
        flag_new_group_or_new_attribute_read_group = 0;
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "start_time", &value.attr_int) != 0){
            WARNING("start_time auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("duration",name)==0 && H5Tclass==H5T_INTEGER){
        flag_new_group_or_new_attribute_read_group = 0;
//        operator_data->slow5_record->len_raw_signal = value.attr_int;
    }
    else if(strcmp("read_number",name)==0 && H5Tclass==H5T_INTEGER && *(operator_data->flag_lossy) == 0){
        if(*(operator_data->flag_header_is_written)==0 && *(operator_data->flag_lossy)==0){
            if(slow5_aux_meta_add(operator_data->slow5File->header->aux_meta, "read_number", SLOW5_INT32_T)){
                ERROR("Could not initialize the record attribute '%s'", "read_number");
                return -1;
            }
        }
        flag_new_group_or_new_attribute_read_group = 0;
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "read_number", &value.attr_int) != 0){
            WARNING("read_number auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("start_mux",name)==0 && H5Tclass==H5T_INTEGER && *(operator_data->flag_lossy) == 0){
        if(*(operator_data->flag_header_is_written)==0 && *(operator_data->flag_lossy)==0){
            if(slow5_aux_meta_add(operator_data->slow5File->header->aux_meta, "start_mux", SLOW5_UINT8_T)){
                ERROR("Could not initialize the record attribute '%s'", "start_mux");
                return -1;
            }
        }
        flag_new_group_or_new_attribute_read_group = 0;
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "start_mux", &value.attr_int) != 0){
            WARNING("start_mux auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("read_id",name)==0 && H5Tclass==H5T_STRING){
        flag_new_group_or_new_attribute_read_group = 0;
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        //make sure read_id has a proper starting character
        if(isalpha(value.attr_string[0]) || isdigit(value.attr_string[0])){
            operator_data->slow5_record->read_id_len = strlen(value.attr_string);
            operator_data->slow5_record->read_id = strdup(value.attr_string);
        }else{
            ERROR("read_id of this format is not supported [%s]", value.attr_string);
            return -1;
        }
    }
    else if(strcmp("median_before",name)==0 && H5Tclass==H5T_FLOAT && *(operator_data->flag_lossy) == 0){
        if(*(operator_data->flag_header_is_written)==0 && *(operator_data->flag_lossy)==0){
            if(slow5_aux_meta_add(operator_data->slow5File->header->aux_meta, "median_before", SLOW5_DOUBLE)){
                ERROR("Could not initialize the record attribute '%s'", "median_before");
                return -1;
            }
        }
        flag_new_group_or_new_attribute_read_group = 0;
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "median_before", &value.attr_double) != 0){
            WARNING("median_before auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("end_reason",name)==0 && H5Tclass==H5T_ENUM){
//        operator_data->slow5_record->end_reason = value.attr_uint8_t;
        std::string key = "ns_" + std::string(name); //not stored
        char warn_message[300];
        sprintf(warn_message,"Not stored: Attribute %s/%s is not stored yet until we confirm from ONT about its datatype", operator_data->group_name,name);
        search_and_warn(operator_data,key,warn_message);
    }
//            CHANNEL_ID
    else if(strcmp("channel_number",name)==0 && H5Tclass==H5T_STRING && *(operator_data->flag_lossy) == 0){
        if(*(operator_data->flag_header_is_written)==0 && *(operator_data->flag_lossy)==0){
            if(slow5_aux_meta_add(operator_data->slow5File->header->aux_meta, "channel_number", SLOW5_STRING)){
                ERROR("Could not initialize the record attribute '%s'", "channel_number");
                return -1;
            }
        }
        flag_new_group_or_new_attribute_read_group = 0;
        if(slow5_rec_set_string(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "channel_number", value.attr_string) != 0){
            WARNING("channel_number auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("digitisation",name)==0 && H5Tclass==H5T_FLOAT){
        flag_new_group_or_new_attribute_read_group = 0;
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->digitisation = value.attr_double;
    }
    else if(strcmp("offset",name)==0 && H5Tclass==H5T_FLOAT){
        flag_new_group_or_new_attribute_read_group = 0;
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->offset = value.attr_double;
    }
    else if(strcmp("range",name)==0 && H5Tclass==H5T_FLOAT){
        flag_new_group_or_new_attribute_read_group = 0;
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->range = value.attr_double;
    }
    else if(strcmp("sampling_rate",name)==0 && H5Tclass==H5T_FLOAT){
        flag_new_group_or_new_attribute_read_group = 0;
        *(operator_data->primary_fields_count) = *(operator_data->primary_fields_count) + 1;
        operator_data->slow5_record->sampling_rate = value.attr_double;
    }

    // if group is ROOT or CONTEXT_TAGS or TRACKING_ID or attribute is 'run_id' create an attribute in the header and store value
    if(strcmp(name,"run_id")==0 || strcmp(operator_data->group_name,"")==0 || strcmp(operator_data->group_name,"context_tags")==0 || strcmp(operator_data->group_name,"tracking_id")==0){
        flag_new_group_or_new_attribute_read_group = 0;
        if (H5Tclass != H5T_STRING) {
            flag_value_string = 1;
            size_t storage_size = 50;
            value.attr_string = (char *) calloc(storage_size + 1, sizeof(char));
            switch (H5Tclass) {
                case H5T_FLOAT:
                    sprintf(value.attr_string, "%.1f", value.attr_double);
                    break;
                case H5T_INTEGER:
                    sprintf(value.attr_string, "%d", value.attr_int);
                    break;
                case H5T_ENUM:
                    sprintf(value.attr_string, "%u", value.attr_uint8_t);
                    break;
                default:
                    ERROR("%s", "This should not be printed");
                    return -1;
            }
            std::string key = "co_" + std::string(name); //convert
            char warn_message[300];
            sprintf(warn_message,"Convert: Converting the attribute %s/%s from %s to string",operator_data->group_name, name, h5t_class.c_str());
            search_and_warn(operator_data,key,warn_message);
        }

        int flag_attribute_exists = 0;
        char* existing_attribute;
        int flag_existing_attr_value_mismatch = 0;

        ret = slow5_hdr_add_attr(name, operator_data->slow5File->header);
        if(ret == -1){
            ERROR("Could not add the header attribute '%s/%s'. Input parameter is NULL", operator_data->group_name, name);
            return -1;
        } else if(ret == -2){
            flag_attribute_exists = 1;
//            WARNING("Attribute '%s/%s' is already added to the header.", operator_data->group_name, name);
            existing_attribute = slow5_hdr_get("run_id", 0, operator_data->slow5File->header);
            if(strcmp(existing_attribute,value.attr_string)){
                flag_existing_attr_value_mismatch = 1;
            }
        } else if(ret == -3){
            ERROR("Could not add the header attribute '%s/%s'. Internal error occured", operator_data->group_name, name);
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
                    char warn_message[300];
                    sprintf(warn_message,"Mismatch: Different run_ids found in a single fast5 file. First seen run_id will be set in slow5 header");
                    search_and_warn(operator_data,key,warn_message);
                }else{
                    ERROR("Different run_ids found in a single fast5 file. Cannot create a single header slow5/blow5. Please consider --allow option.%s", "");
                    return -1;
                }
            }
        }
    }

    if(flag_new_group_or_new_attribute_read_group){
        std::string key = "at_" + std::string(name); //Alert
        char warn_message[300];
        sprintf(warn_message,"Alert: Attribute %s/%s in %s is something we haven't seen before. Please open a github issue with an example of the fast5 file so we can implement special handling of such attributes.", name, operator_data->group_name, operator_data->fast5_path);
        search_and_warn(operator_data,key,warn_message);
    }

    if(flag_value_string){
        free(value.attr_string);
    }
    return return_val;
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

int read_dataset(hid_t loc_id, const char *name, slow5_rec_t* slow5_record) {

    hid_t dset = H5Dopen(loc_id, name, H5P_DEFAULT);
    if (dset < 0) {
        WARNING("Failed to open dataset '%s' to read raw signal.", name);
        return -1;
        // goto cleanup2;
    }

    hid_t space;
    hsize_t h5_nsample;
    space = H5Dget_space(dset);
    if (space < 0) {
        WARNING("Failed to create copy of dataspace for raw signal %s.",
                name);
        H5Dclose(dset);
        return -1;
        // goto cleanup3;
    }

    int32_t ret1 = H5Sget_simple_extent_dims(space, &h5_nsample, NULL);
    if (ret1 < 0) {
        WARNING("Failed to get the dataspace dimension for raw signal %s.", name);
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

int read_fast5(fast5_file_t *fast5_file,
               slow5_fmt format_out,
               slow5_press_method_t press_out,
               int lossy,
               int write_header_flag,
               int flag_allow_run_id_mismatch,
               struct program_meta *meta,
               slow5_file_t *slow5File,
               std::unordered_map<std::string,
               uint32_t>* warning_map) {

    struct operator_obj tracker;
    tracker.group_level = ROOT;
    tracker.prev = NULL;
    H5O_info_t infobuf;
    H5Oget_info(fast5_file->hdf5_file, &infobuf);
    tracker.addr = infobuf.addr;

    tracker.meta = meta;
    tracker.fast5_file = fast5_file;
    tracker.format_out = format_out;
    tracker.pressMethod = press_out;
    tracker.press_ptr = slow5_press_init(press_out);

    tracker.fast5_path = fast5_file->fast5_path;
    tracker.slow5File = slow5File;

    int flag_context_tags = 0;
    int flag_tracking_id = 0;
    int flag_run_id = 0;
    int flag_lossy = lossy;
    int primary_fields_count = 0;
    int flag_header_is_written = write_header_flag;

    size_t zero0 = 0;

    tracker.flag_context_tags = &flag_context_tags;
    tracker.flag_tracking_id = &flag_tracking_id;
    tracker.flag_run_id = &flag_run_id;
    tracker.flag_lossy = &flag_lossy;
    tracker.flag_write_header = &write_header_flag;
    tracker.flag_allow_run_id_mismatch = &flag_allow_run_id_mismatch;
    tracker.flag_header_is_written = &flag_header_is_written;
    tracker.nreads = &zero0;
    tracker.slow5_record = slow5_rec_init();
    tracker.group_name = "";
    tracker.primary_fields_count = &primary_fields_count;

    tracker.warning_map = warning_map;

    herr_t iterator_ret;

    if (fast5_file->is_multi_fast5) {
        hsize_t number_of_groups = 0;
        H5Gget_num_objs(fast5_file->hdf5_file,&number_of_groups);
        tracker.num_read_groups = &number_of_groups; //todo:check if the assumption is valid
        //obtain the root group attributes
        iterator_ret = H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &tracker);
        if(iterator_ret<0){
            return -1;
        }
        //now iterate over read groups. loading records and writing them are done inside fast5_group_itr
        iterator_ret =H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, fast5_group_itr, (void *) &tracker);
        if(iterator_ret<0){
            return -1;
        }
    }else{ // single-fast5
        //obtain the root group attributes
        iterator_ret = H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &tracker);
        if(iterator_ret<0){
            return -1;
        }
        hsize_t number_of_groups = 1;
        tracker.num_read_groups = &number_of_groups;
        //now iterate over read groups. loading records and writing them are done inside fast5_group_itr
        iterator_ret = H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, fast5_group_itr, (void *) &tracker);
        if(iterator_ret<0){
            return -1;
        }
        //        todo: compare header values with the previous singlefast5
        if(*(tracker.flag_run_id) != 1){
            ERROR("run_id information is missing in the %s in read_id %s.", tracker.fast5_path, tracker.slow5_record->read_id);
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
            ERROR("A primary attribute is missing in the %s. Check the fast5 files", tracker.fast5_path);
            return -1;
        }
        slow5_rec_free(tracker.slow5_record);
    }
    slow5_press_free(tracker.press_ptr);
    return 1;
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
    if(strcmp(name,"Analyses")==0)return return_val;
    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library.
     */
    H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
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
                printf ("%*s  Warning: Loop detected!\n", spaces, "");
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
                    if (strcmp(name, "tracking_id") == 0 && *(operator_data->flag_tracking_id) == 0) {
                        return_val = H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                        *(operator_data->flag_tracking_id) = 1;
                    } else if (strcmp(name, "context_tags") == 0 && *(operator_data->flag_context_tags) == 0) {
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
                                ERROR("The first read does not have context_tags information%s", ".");
                                return -1;
                            }
                            if (*(operator_data->flag_tracking_id) != 1) {
                                ERROR("The first read does not have tracking_id information%s", ".");
                                return -1;
                            }
                            if(*(operator_data->flag_run_id) != 1){
                                ERROR("run_id information is missing in the %s in read_id %s.", operator_data->fast5_path, operator_data->slow5_record->read_id);
                                return -1;
                            }
                            if(*(operator_data->flag_write_header) == 0){
                                int ret = print_slow5_header(operator_data);
                                if(ret < 0){
                                    return ret;
                                }
                            }
                        }
                        *(operator_data->nreads) = *(operator_data->nreads) + 1;
                        if(*(operator_data->primary_fields_count) == PRIMARY_FIELD_COUNT){
                            if(*(operator_data->flag_run_id) != 1){
                                ERROR("run_id information is missing in the %s in read_id %s.", operator_data->fast5_path, operator_data->slow5_record->read_id);
                                return -1;
                            }
                            int ret = print_record(operator_data);
                            if(ret < 0){
                                return ret;
                            }
                            *(operator_data->primary_fields_count) = 0;
                            *(operator_data->flag_run_id) = 0;
                        }else{
                            ERROR("A primary attribute is missing in the %s. Check the fast5 files", operator_data->fast5_path);
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
            WARNING("Datatype: %s is not seen in fast5 before. Please check %s", name, operator_data->fast5_path);
            break;
        default:
            WARNING("Unknown: %s is not seen in fast5 before. Please check %s", name, operator_data->fast5_path);
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

// given a directory path, recursively find all files
void list_all_items(const std::string& path, std::vector<std::string>& files, int count_dir, const char* extension){
    if(extension){
        STDERR("Looking for '*%s' files in %s", extension, path.c_str());
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
                    if(full_fn.find(extension) != std::string::npos){
                        files.push_back(full_fn);
                    }
                }else{
                    files.push_back(full_fn);
                }
            }
        }
    }else{
        if(extension){
            if(path.find(extension) != std::string::npos){
                files.push_back(path);
            }
        }else{
            files.push_back(path);
        }
    }

    if(is_directory(path) && count_dir){
        files.push_back(path);
    }
}

int slow5_hdr_initialize(slow5_hdr *header, int lossy){
    slow5_hdr_add_rg(header);
    header->num_read_groups = 1;
    if(lossy==0){
        struct slow5_aux_meta *aux_meta = slow5_aux_meta_init_empty();
        header->aux_meta = aux_meta;
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
            ERROR("error reading attribute %s", attribute_name.c_str());
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
