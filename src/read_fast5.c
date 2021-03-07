//
// Created by shan on 2020-12-22.
//
#include "slow5_old.h"
#include "slow5.h"
#include <float.h>
#include "error.h"
#include "slow5_extra.h"

// Operator function to be called by H5Aiterate.
herr_t op_func_attr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *operator_data);
// Operator function to be called by H5Literate.
herr_t op_func_group (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);

// function to read signal data
int read_dataset(hid_t dset, const char *name, slow5_record_t* slow5_record);
//other functions
int group_check(struct operator_obj *od, haddr_t target_addr);

//remove this later; added for the sake of slow5 format completeness
void print_record(operator_obj* operator_data);

// from nanopolish_fast5_io.cpp
fast5_file_t fast5_open(const char* filename) {
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

herr_t op_func_attr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data){
    hid_t attribute, attribute_type, native_type;
    herr_t return_val = 0;
    int ret = 0;

    struct operator_obj *operator_data = (struct operator_obj *) op_data;
    // Ensure attribute exists
    ret = H5Aexists(loc_id, name);
    if(ret <= 0) {
        fprintf(stdout,"attribute %s not found\n",name);
    }

    attribute = H5Aopen(loc_id, name, H5P_DEFAULT);
    attribute_type = H5Aget_type(attribute);
    native_type = H5Tget_native_type(attribute_type, H5T_DIR_ASCEND);
    H5T_class_t H5Tclass = H5Tget_class(attribute_type);

    union attribute_data value;
    int flag_value_string = 0;
    switch(H5Tclass){
        case H5T_STRING:
            flag_value_string = 1;
            if(H5Tis_variable_str(attribute_type) > 0) {
                // variable length string
                ret = H5Aread(attribute, native_type, &value.attr_string);
                if(ret < 0) {
                    fprintf(stderr, "error reading attribute %s\n", name);
                    exit(EXIT_FAILURE);
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
                    fprintf(stderr, "error reading attribute %s\n", name);
                    exit(EXIT_FAILURE);
                }
            }
            if (value.attr_string && !value.attr_string[0]) {
                WARNING("The attribute value of %s/%s is an empty string",operator_data->group_name, name);
                if(flag_value_string){ // hack to skip the free() at the bottom of the function
                    free(value.attr_string);
                    flag_value_string = 0;
                }
                value.attr_string = (char*)".";
            }
            break;
        case H5T_FLOAT:
            H5Aread(attribute, native_type, &(value.attr_double));
            break;
        case H5T_INTEGER:
            H5Aread(attribute,native_type,&(value.attr_int));
            break;
        case H5T_ENUM:
            H5Aread(attribute, native_type, &(value.attr_uint8_t));
            break;
        default:
            fprintf (stderr,"Unknown: %s\n", name);
    }
    H5Tclose(native_type);
    H5Tclose(attribute_type);
    H5Aclose(attribute);


    if(strcmp("file_type",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("file_type", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("file_version",name)==0){
        if (H5Tclass == H5T_STRING) {
            slow5_hdr_set("file_version", "value.attr_string", 0, operator_data->slow5File->header);
        } else if (H5Tclass == H5T_FLOAT) {
            char buf[50];
            sprintf(buf,"%.1f", value.attr_double);
            WARNING("Converting the attribute %s/%s from H5T_FLOAT to string ",operator_data->group_name,name);
//            operator_data->slow5_header->file_version = buf);
           slow5_hdr_set("file_version", buf, 0, operator_data->slow5File->header);
        }
    }
//            READ
    else if(strcmp("run_id",name)==0 && H5Tclass==H5T_STRING){
        *(operator_data->flag_run_id) = 1;

        slow5_hdr_set("run_id", value.attr_string, 0, operator_data->slow5File->header);
        slow5_hdr_set("tracking_id_run_id", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("pore_type",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("pore_type", value.attr_string, 0, operator_data->slow5File->header);
    }
//            RAW
    else if(strcmp("start_time",name)==0 && H5Tclass==H5T_INTEGER){
        slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "start_time", &value.attr_int);
    }
    else if(strcmp("duration",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->len_raw_signal = value.attr_int;
    }
    else if(strcmp("read_number",name)==0 && H5Tclass==H5T_INTEGER){
        slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "read_number", &value.attr_int);
    }
    else if(strcmp("start_mux",name)==0 && H5Tclass==H5T_INTEGER){
        slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "start_mux", &value.attr_int);
    }
    else if(strcmp("read_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_record->read_id_len = strlen(value.attr_string);
        operator_data->slow5_record->read_id = strdup(value.attr_string);
    }
    else if(strcmp("median_before",name)==0 && H5Tclass==H5T_FLOAT){
        slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "median_before", &value.attr_double);
    }
//    else if(strcmp("end_reason",name)==0 && H5Tclass==H5T_ENUM){
//        operator_data->slow5_record->end_reason = value.attr_uint8_t;
//    }
//            CHANNEL_ID
    else if(strcmp("channel_number",name)==0 && H5Tclass==H5T_STRING){
        slow5_rec_set_string(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "channel_number", value.attr_string);
    }
    else if(strcmp("digitisation",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->digitisation = value.attr_double;
    }
    else if(strcmp("offset",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->offset = value.attr_double;
    }
    else if(strcmp("range",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->range = value.attr_double;
    }
    else if(strcmp("sampling_rate",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->sampling_rate = value.attr_double;
    }
//            CONTEXT_TAGS
    else if(strcmp("sample_frequency",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("sample_frequency", value.attr_string, 0, operator_data->slow5File->header);
    }
        //additional attributes in 2.2
    else if(strcmp("barcoding_enabled",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("barcoding_enabled", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("experiment_duration_set",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("experiment_duration_set", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("experiment_type",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("experiment_type", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("local_basecalling",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("local_basecalling", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("package",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("package", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("package_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("package_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("sequencing_kit",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("sequencing_kit", value.attr_string, 0, operator_data->slow5File->header);
    }
        //additional attributes in 2.0
    else if(strcmp("filename",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("filename", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("experiment_kit",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("experiment_kit", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("user_filename_input",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("user_filename_input", value.attr_string, 0, operator_data->slow5File->header);
    }
//            TRACKING_ID
    else if(strcmp("asic_id",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("asic_id", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("asic_id_eeprom",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("asic_id_eeprom", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("asic_temp",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("asic_temp", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("auto_update",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("auto_update", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("auto_update_source",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("auto_update_source", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("bream_is_standard",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("bream_is_standard", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("device_id",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("device_id", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("exp_script_name",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("exp_script_name", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("exp_script_purpose",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("exp_script_purpose", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("exp_start_time",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("exp_start_time", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("flow_cell_id",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("flow_cell_id", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("heatsink_temp",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("heatsink_temp", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("hostname",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("hostname", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("installation_type",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("installation_type", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("local_firmware_file",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("local_firmware_file", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("operating_system",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("operating_system", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("protocol_run_id",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("protocol_run_id", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("protocols_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("protocols_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("usb_config",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("usb_config", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("version", value.attr_string, 0, operator_data->slow5File->header);
    }
        //additional attributes in 2.0
    else if(strcmp("bream_core_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("bream_core_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("bream_ont_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("bream_ont_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("bream_prod_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("bream_prod_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("bream_rnd_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("bream_rnd_version", value.attr_string, 0, operator_data->slow5File->header);
    }
        //additional attributes in 2.2
    else if(strcmp("asic_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("asic_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("configuration_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("configuration_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("device_type",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("device_type", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("distribution_status",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("distribution_status", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("distribution_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("distribution_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("flow_cell_product_code",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("flow_cell_product_code", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("guppy_version",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("guppy_version", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("protocol_group_id",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("protocol_group_id", value.attr_string, 0, operator_data->slow5File->header);
    }
    else if(strcmp("sample_id",name)==0 && H5Tclass==H5T_STRING){
        slow5_hdr_set("sample_id", value.attr_string, 0, operator_data->slow5File->header);
    }else{
        WARNING("The attribute %s/%s is not stored ", operator_data->group_name, name);
    }

    if(flag_value_string){
        free(value.attr_string);
    }
    return return_val;


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

    slow5_record->raw_signal = (int16_t *) malloc(h5_nsample * sizeof *(slow5_record->raw_signal));
    hid_t status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, slow5_record->raw_signal);

    if (status < 0) {
        WARNING("Failed to read raw data from dataset %s.", name);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }
    H5Sclose(space);
    H5Dclose(dset);
    return 0;
}

int read_fast5(fast5_file_t *fast5_file, enum slow5_fmt format_out, enum press_method pressMethod, int call_count, struct program_meta *meta, slow5_file_t* slow5File){

    struct operator_obj tracker;
    tracker.group_level = ROOT;
    tracker.prev = NULL;
    H5O_info_t infobuf;
    H5Oget_info(fast5_file->hdf5_file, &infobuf);
    tracker.addr = infobuf.addr;

    tracker.meta = meta;
    tracker.fast5_file = fast5_file;
    tracker.format_out = format_out;
    tracker.pressMethod = pressMethod;
    tracker.fast5_path = fast5_file->fast5_path;
    tracker.slow5File = slow5File;

    slow5_header_t slow5_header;
//    struct slow5_rec *read = slow5_rec_init();


    int flag_context_tags = 0;
    int flag_tracking_id = 0;
    int flag_run_id = 0;
    size_t zero = 0;

//    tracker.slow5_header = &slow5_header;
//    tracker.slow5_record = &slow5_record;
    tracker.flag_context_tags = &flag_context_tags;
    tracker.flag_tracking_id = &flag_tracking_id;
    tracker.flag_run_id = &flag_run_id;
    tracker.nreads = &zero;
    tracker.slow5_record = slow5_rec_init();

    slow5_hdr_set("file_format", SLOW5_FILE_FORMAT_SHORT, 0, tracker.slow5File->header);

//    slow5_rec_free(read);

    if (fast5_file->is_multi_fast5) {
        fprintf(stderr,"multi\n");
        hsize_t number_of_groups = 0;
        H5Gget_num_objs(fast5_file->hdf5_file,&number_of_groups);
        tracker.num_read_groups = &number_of_groups; //todo:check if the assumption is valid
        //obtain the root group attributes
        H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &tracker);
        //now iterate over read groups. loading records and writing them are done inside op_func_group
        H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, op_func_group, (void *) &tracker);
    }else{ // single-fast5
        //obtain the root group attributes
        tracker.group_name = "";
        H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &tracker);
        hsize_t number_of_groups = 1;
        tracker.num_read_groups = &number_of_groups;
        //now iterate over read groups. loading records and writing them are done inside op_func_group
        H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, op_func_group, (void *) &tracker);
        //        todo: compare header values with the previous singlefast5
        if(call_count==0){
            if(*(tracker.flag_run_id) != 1){
                fprintf(stderr, "run_id is not set%s\n", "");
                exit(EXIT_FAILURE);
            }
            print_slow5_header(&tracker);
        }
        print_record(&tracker);
        slow5_rec_free(tracker.slow5_record);
    }
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
herr_t op_func_group (hid_t loc_id, const char *name, const H5L_info_t *info, void *op_data){

    herr_t return_val = 0;
    H5O_info_t infobuf;
    struct operator_obj *operator_data = (struct operator_obj *) op_data;
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
                        H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                        *(operator_data->flag_tracking_id) = 1;
                    } else if (strcmp(name, "context_tags") == 0 && *(operator_data->flag_context_tags) == 0) {
                        H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                        *(operator_data->flag_context_tags) = 1;
                    } else if (strcmp(name, "tracking_id") != 0 && strcmp(name, "context_tags") != 0) {
                        H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                    }
                }else{
                    H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &next_op);
                }

                H5Gclose(group);
                //the recursive call
                return_val = H5Literate_by_name(loc_id, name, H5_INDEX_NAME,H5_ITER_INC, 0, op_func_group, (void *) &next_op, H5P_DEFAULT);

                if (operator_data->fast5_file->is_multi_fast5) {
                    //check if we are at a root-level group
                    if (operator_data->group_level == ROOT) {
                        if (*(operator_data->nreads) == 0) {
                            if (*(operator_data->flag_context_tags) != 1) {
                                fprintf(stderr, "The first read does not have context_tags information\n");
                                exit(EXIT_FAILURE);
                            }
                            if (*(operator_data->flag_tracking_id) != 1) {
                                fprintf(stderr, "The first read does not have tracking_id information\n");
                                exit(EXIT_FAILURE);
                            }
                            if(*(operator_data->flag_run_id) != 1){
                                fprintf(stderr, "run_id is not set%s\n", "");
                                exit(EXIT_FAILURE);
                            }
                            print_slow5_header(operator_data);//remove this later; added for the sake of slow5 format completeness
                        }
                        *(operator_data->nreads) = *(operator_data->nreads) + 1;

                        print_record(operator_data);
                        slow5_rec_free(operator_data->slow5_record);
                        if(*(operator_data->nreads) < *(operator_data->num_read_groups)){
                            operator_data->slow5_record = slow5_rec_init();
                        }
                    }
                }
            }
            break;
        case H5O_TYPE_DATASET:
            read_dataset(loc_id, name, operator_data->slow5_record);
            break;
        case H5O_TYPE_NAMED_DATATYPE:
            printf ("Datatype: %s\n", name);
            break;
        default:
            printf ( "Unknown: %s\n", name);
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

void print_slow5_header(operator_obj* operator_data) {
    if(slow5_hdr_fwrite(operator_data->slow5File->fp, operator_data->slow5File->header, operator_data->format_out, operator_data->pressMethod) == -1){
        fprintf(stderr, "Could not write the header\n");
        exit(EXIT_FAILURE);
    }
}

void print_record(operator_obj* operator_data) {
//    todo - create struct press and pass to slow5_rec_fwrite()
    if(slow5_rec_fwrite(operator_data->slow5File->fp, operator_data->slow5_record, operator_data->slow5File->header->aux_meta, operator_data->format_out, NULL) == -1){
        fprintf(stderr, "Could not write the record %s\n", operator_data->slow5_record->read_id);
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
void find_all_5(const std::string& path, std::vector<std::string>& fast5_files, const char* extension)
{
    STDERR("Looking for %s files in %s", extension, path.c_str());
    if (is_directory(path)) {
        std::vector< std::string > dir_list = list_directory(path);
        for (const auto& fn : dir_list) {
            if(fn == "." or fn == "..") {
                continue;
            }

            std::string full_fn = path + "/" + fn;
            bool is_fast5 = full_fn.find(extension) != std::string::npos;
            // JTS 04/19: is_directory is painfully slow
            if(is_directory(full_fn)) {
                // recurse
                find_all_5(full_fn, fast5_files, extension);
            } else if (is_fast5) {
                fast5_files.push_back(full_fn);
                //add to the list
            }
        }
    }else{
        bool is_fast5 = path.find(extension) != std::string::npos;
        if(is_fast5){
            fast5_files.push_back(path);
        }
    }
}
