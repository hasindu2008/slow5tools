//
// Created by shan on 2020-12-22.
//
#include "slow5_old.h"
#include <float.h>
#include "error.h"


// Operator function to be called by H5Aiterate.
herr_t op_func_attr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *operator_data);
// Operator function to be called by H5Literate.
herr_t op_func_group (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);

// function to read signal data
int read_dataset(hid_t dset, const char *name, slow5_record_t* slow5_record);
//other functions
int group_check(struct operator_obj *od, haddr_t target_addr);

void check_attributes(group_flags group_flag, operator_obj* operator_data);
void reset_attributes(group_flags group_flag, operator_obj* operator_data);
void free_attributes(group_flags group_flag, operator_obj* operator_data);

//remove this later; added for the sake of slow5 format completeness
void print_record(operator_obj* operator_data);
void print_header(operator_obj* operator_data);

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
                fprintf(stderr,"warning: attribute value of %s/%s is an empty string\n",operator_data->group_name, name);
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
        operator_data->slow5_header->file_type = strdup(value.attr_string);
    }
    else if(strcmp("file_version",name)==0){
        if(!operator_data->slow5_header->file_version) {
            if (H5Tclass == H5T_STRING) {
                operator_data->slow5_header->file_version = strdup(value.attr_string);
            } else if (H5Tclass == H5T_FLOAT) {
                char buf[50];
                sprintf(buf,"%.1f", value.attr_double);
                WARNING("Converting the attribute %s/%s from H5T_FLOAT to string ",operator_data->group_name,name);
                operator_data->slow5_header->file_version = strdup(buf);
            }
        }
    }
//            READ
    else if(strcmp("run_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->run_id = strdup(value.attr_string);
        operator_data->slow5_header->tracking_id_run_id = strdup(value.attr_string); //check the assumption : read/run_id and read/tracking_id/run_id are equal
    }
    else if(strcmp("pore_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->pore_type = strdup(value.attr_string);
    }
//            RAW
    else if(strcmp("start_time",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->start_time = value.attr_int;
    }
    else if(strcmp("duration",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->duration = value.attr_int;
    }
    else if(strcmp("read_number",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->read_number = value.attr_int;
    }
    else if(strcmp("start_mux",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->start_mux = value.attr_int;
    }
    else if(strcmp("read_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_record->read_id = strdup(value.attr_string);
    }
    else if(strcmp("median_before",name)==0 && H5Tclass==H5T_FLOAT){
        operator_data->slow5_record->median_before = value.attr_double;
    }
    else if(strcmp("end_reason",name)==0 && H5Tclass==H5T_ENUM){
        operator_data->slow5_record->end_reason = value.attr_uint8_t;
    }
//            CHANNEL_ID
    else if(strcmp("channel_number",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_record->channel_number = strdup(value.attr_string);
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
        operator_data->slow5_header->sample_frequency = strdup(value.attr_string);
    }
        //additional attributes in 2.2
    else if(strcmp("barcoding_enabled",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->barcoding_enabled = strdup(value.attr_string);
    }
    else if(strcmp("experiment_duration_set",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->experiment_duration_set = strdup(value.attr_string);
    }
    else if(strcmp("experiment_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->experiment_type = strdup(value.attr_string);
    }
    else if(strcmp("local_basecalling",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->local_basecalling = strdup(value.attr_string);
    }
    else if(strcmp("package",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->package = strdup(value.attr_string);
    }
    else if(strcmp("package_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->package_version = strdup(value.attr_string);
    }
    else if(strcmp("sequencing_kit",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->sequencing_kit = strdup(value.attr_string);
    }
        //additional attributes in 2.0
    else if(strcmp("filename",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->filename = strdup(value.attr_string);
    }
    else if(strcmp("experiment_kit",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->experiment_kit = strdup(value.attr_string);
    }
    else if(strcmp("user_filename_input",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->user_filename_input = strdup(value.attr_string);
    }
//            TRACKING_ID
    else if(strcmp("asic_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_id = strdup(value.attr_string);
    }
    else if(strcmp("asic_id_eeprom",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_id_eeprom = strdup(value.attr_string);
    }
    else if(strcmp("asic_temp",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_temp = strdup(value.attr_string);
    }
    else if(strcmp("auto_update",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->auto_update = strdup(value.attr_string);
    }
    else if(strcmp("auto_update_source",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->auto_update_source = strdup(value.attr_string);
    }
    else if(strcmp("bream_is_standard",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_is_standard = strdup(value.attr_string);
    }
    else if(strcmp("device_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->device_id = strdup(value.attr_string);
    }
    else if(strcmp("exp_script_name",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->exp_script_name = strdup(value.attr_string);
    }
    else if(strcmp("exp_script_purpose",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->exp_script_purpose = strdup(value.attr_string);
    }
    else if(strcmp("exp_start_time",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->exp_start_time = strdup(value.attr_string);
    }
    else if(strcmp("flow_cell_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->flow_cell_id = strdup(value.attr_string);
    }
    else if(strcmp("heatsink_temp",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->heatsink_temp = strdup(value.attr_string);
    }
    else if(strcmp("hostname",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->hostname = strdup(value.attr_string);
    }
    else if(strcmp("installation_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->installation_type = strdup(value.attr_string);
    }
    else if(strcmp("local_firmware_file",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->local_firmware_file = strdup(value.attr_string);
    }
    else if(strcmp("operating_system",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->operating_system = strdup(value.attr_string);
    }
    else if(strcmp("protocol_run_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->protocol_run_id = strdup(value.attr_string);
    }
    else if(strcmp("protocols_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->protocols_version = strdup(value.attr_string);
    }
    else if(strcmp("usb_config",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->usb_config = strdup(value.attr_string);
    }
    else if(strcmp("version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->version = strdup(value.attr_string);
    }
        //additional attributes in 2.0
    else if(strcmp("bream_core_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_core_version = strdup(value.attr_string);
    }
    else if(strcmp("bream_ont_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_ont_version = strdup(value.attr_string);
    }
    else if(strcmp("bream_prod_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_prod_version = strdup(value.attr_string);
    }
    else if(strcmp("bream_rnd_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->bream_rnd_version = strdup(value.attr_string);
    }
        //additional attributes in 2.2
    else if(strcmp("asic_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->asic_version = strdup(value.attr_string);
    }
    else if(strcmp("configuration_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->configuration_version = strdup(value.attr_string);
    }
    else if(strcmp("device_type",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->device_type = strdup(value.attr_string);
    }
    else if(strcmp("distribution_status",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->distribution_status = strdup(value.attr_string);
    }
    else if(strcmp("distribution_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->distribution_version = strdup(value.attr_string);
    }
    else if(strcmp("flow_cell_product_code",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->flow_cell_product_code = strdup(value.attr_string);
    }
    else if(strcmp("guppy_version",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->guppy_version = strdup(value.attr_string);
    }
    else if(strcmp("protocol_group_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->protocol_group_id = strdup(value.attr_string);
    }
    else if(strcmp("sample_id",name)==0 && H5Tclass==H5T_STRING){
        operator_data->slow5_header->sample_id = strdup(value.attr_string);
    }else{
        fprintf(stderr,"[%s] we don't store the attribute %s/%s\n",__func__ , operator_data->group_name, name);
    }

    if(flag_value_string){
        free(value.attr_string);
    }

    return return_val;
}

int read_dataset(hid_t loc_id, const char *name, slow5_record_t* slow5_record) {
    int16_t* rawptr;   // raw signal

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

    rawptr = (int16_t*)calloc(h5_nsample, sizeof(float));
    hid_t status = H5Dread(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT,rawptr);

    if (status < 0) {
        free(rawptr);
        WARNING("Failed to read raw data from dataset %s.", name);
        H5Sclose(space);
        H5Dclose(dset);
        return -1;
    }
    H5Sclose(space);
    H5Dclose(dset);
    slow5_record->raw_signal = rawptr;
    return 0;
}

int read_fast5(fast5_file_t *fast5_file, FILE *f_out, enum FormatOut format_out, z_streamp strmp, FILE *f_idx, int call_count, struct program_meta *meta){

    struct operator_obj tracker;
    tracker.group_level = ROOT;
    tracker.prev = NULL;
    H5O_info_t infobuf;
    H5Oget_info(fast5_file->hdf5_file, &infobuf);
    tracker.addr = infobuf.addr;

    tracker.meta = meta;
    tracker.fast5_file = fast5_file;
    tracker.f_out = f_out;
    tracker.format_out = format_out;
    tracker.strmp = strmp;
    tracker.f_idx = f_idx;
    tracker.fast5_path = fast5_file->fast5_path;

    slow5_header_t slow5_header;
    slow5_record_t slow5_record;
    int flag_context_tags = 0;
    int flag_tracking_id = 0;
    size_t number_reads = 0;

    tracker.slow5_header = &slow5_header;
    tracker.slow5_record = &slow5_record;
    tracker.flag_context_tags = &flag_context_tags;
    tracker.flag_tracking_id = &flag_tracking_id;
    tracker.nreads = &number_reads;

    reset_attributes(ROOT, &tracker);
    reset_attributes(READ, &tracker);
    reset_attributes(RAW, &tracker);
    reset_attributes(CHANNEL_ID, &tracker);
    reset_attributes(CONTEXT_TAGS, &tracker);
    reset_attributes(TRACKING_ID, &tracker);

    tracker.slow5_header->file_format = SLOW5_FILE_FORMAT;

    if (fast5_file->is_multi_fast5) {
        hsize_t number_of_groups = 0;
        H5Gget_num_objs(fast5_file->hdf5_file,&number_of_groups);
        tracker.slow5_header->num_read_groups = 1; //todo:check if the assumption is valid
        //obtain the root group attributes
        H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &tracker);

        //now iterate over read groups. loading records and writing them are done inside op_func_group
        H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, op_func_group, (void *) &tracker);
    }else{ // single-fast5
        //obtain the root group attributes
        tracker.group_name = "";
        H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, op_func_attr, (void *) &tracker);
        tracker.slow5_header->num_read_groups = 1;

        //now iterate over read groups. loading records and writing them are done inside op_func_group
        H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, op_func_group, (void *) &tracker);
        //        todo: compare header values with the previous singlefast5
        if(call_count==0){
            if(!tracker.slow5_header->run_id){
                fprintf(stderr, "run_id is not set%s\n", "");
                exit(EXIT_FAILURE);
            }
            print_header(&tracker);//remove this later; added for the sake of slow5 format completeness
        }
        print_record(&tracker);//remove this later; added for the sake of slow5 format completeness
        free_attributes(READ, &tracker);
        free_attributes(RAW, &tracker);
        free_attributes(CHANNEL_ID, &tracker);
    }
    free_attributes(ROOT, &tracker);
    free_attributes(CONTEXT_TAGS, &tracker);
    free_attributes(TRACKING_ID, &tracker);

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
                            if(!operator_data->slow5_header->run_id){
                                fprintf(stderr, "run_id is not set%s\n", "");
                                exit(EXIT_FAILURE);
                            }
                            print_header(operator_data);//remove this later; added for the sake of slow5 format completeness
                        }
                        *(operator_data->nreads) = *(operator_data->nreads) + 1;

                        print_record(operator_data);//remove this later; added for the sake of slow5 format completeness

                        /*
                        //todo: we can pass slow5_record directly to write_data()
                        fast5_t f5;
                        f5.rawptr = operator_data->slow5_record->raw_signal;
                        f5.nsample = operator_data->slow5_record->duration;
                        f5.digitisation = operator_data->slow5_record->digitisation;
                        f5.offset = operator_data->slow5_record->offset;
                        f5.range = operator_data->slow5_record->range;
                        f5.sample_rate = operator_data->slow5_record->sampling_rate;
                        */

//                    write_data(operator_data->f_out, operator_data->format_out, operator_data->strmp, operator_data->f_idx, operator_data->slow5_record->read_id, f5, operator_data->fast5_path);
                        free_attributes(READ, operator_data);
                        free_attributes(RAW, operator_data);
                        free_attributes(CHANNEL_ID, operator_data);
                    }
                }
            }
            break;
        case H5O_TYPE_DATASET:
//            printf ("Dataset: %s\n", name);
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

void free_attributes(group_flags group_flag, operator_obj* operator_data) {
    switch (group_flag) {
        case ROOT:
            if(operator_data->slow5_header->file_type){
                free(operator_data->slow5_header->file_type);
            }
            operator_data->slow5_header->file_type = NULL;
            if(operator_data->slow5_header->file_version){
                free(operator_data->slow5_header->file_version);
            }
            operator_data->slow5_header->file_version = NULL;
            break;
        case READ:
            if(operator_data->slow5_header->run_id){
                free(operator_data->slow5_header->run_id);
            }
            operator_data->slow5_header->run_id = NULL;
            if(operator_data->slow5_header->pore_type){
                free(operator_data->slow5_header->pore_type);
            }
            operator_data->slow5_header->pore_type = NULL;
            break;
        case RAW:
            if(operator_data->slow5_record->raw_signal){
                free(operator_data->slow5_record->raw_signal);
            }
            operator_data->slow5_record->raw_signal = NULL;
            operator_data->slow5_record->start_time = ULONG_MAX;
            operator_data->slow5_record->duration = UINT_MAX;
            operator_data->slow5_record->read_number = -1;
            operator_data->slow5_record->start_mux = -1;
            if(operator_data->slow5_record->read_id){
                free(operator_data->slow5_record->read_id);
            }
            operator_data->slow5_record->read_id = NULL;
            operator_data->slow5_record->median_before = FLT_MIN;
            operator_data->slow5_record->end_reason = '6' - '0';
            break;
        case CHANNEL_ID:
            operator_data->slow5_record->digitisation = FLT_MIN;
            operator_data->slow5_record->offset = FLT_MIN;
            operator_data->slow5_record->range = FLT_MIN;
            operator_data->slow5_record->sampling_rate = FLT_MIN;
            if(operator_data->slow5_record->channel_number){
                free(operator_data->slow5_record->channel_number);
            }
            operator_data->slow5_record->channel_number = NULL;
            break;
        case CONTEXT_TAGS:
            if(operator_data->slow5_header->sample_frequency){
                free(operator_data->slow5_header->sample_frequency);
            }
            operator_data->slow5_header->sample_frequency = NULL;
            //additional attributes in 2.2
            if(operator_data->slow5_header->barcoding_enabled){
                free(operator_data->slow5_header->barcoding_enabled);
            }
            operator_data->slow5_header->barcoding_enabled = NULL;
            if(operator_data->slow5_header->experiment_duration_set){
                free(operator_data->slow5_header->experiment_duration_set);
            }
            operator_data->slow5_header->experiment_duration_set = NULL;
            if(operator_data->slow5_header->experiment_type){
                free(operator_data->slow5_header->experiment_type);
            }
            operator_data->slow5_header->experiment_type = NULL;
            if(operator_data->slow5_header->local_basecalling){
                free(operator_data->slow5_header->local_basecalling);
            }
            operator_data->slow5_header->local_basecalling = NULL;
            if(operator_data->slow5_header->package){
                free(operator_data->slow5_header->package);
            }
            operator_data->slow5_header->package = NULL;
            if(operator_data->slow5_header->package_version){
                free(operator_data->slow5_header->package_version);
            }
            operator_data->slow5_header->package_version = NULL;
            if(operator_data->slow5_header->sequencing_kit){
                free(operator_data->slow5_header->sequencing_kit);
            }
            operator_data->slow5_header->sequencing_kit = NULL;
            //additional attributes in 2.0
            if(operator_data->slow5_header->filename){
                free(operator_data->slow5_header->filename);
            }
            operator_data->slow5_header->filename = NULL;
            if(operator_data->slow5_header->experiment_kit){
                free(operator_data->slow5_header->experiment_kit);
            }
            operator_data->slow5_header->experiment_kit = NULL;
            if(operator_data->slow5_header->user_filename_input){
                free(operator_data->slow5_header->user_filename_input);
            }
            operator_data->slow5_header->user_filename_input = NULL;
            break;
        case TRACKING_ID:
            if(operator_data->slow5_header->asic_id){
                free(operator_data->slow5_header->asic_id);
            }
            operator_data->slow5_header->asic_id = NULL;
            if(operator_data->slow5_header->asic_id_eeprom){
                free(operator_data->slow5_header->asic_id_eeprom);
            }
            operator_data->slow5_header->asic_id_eeprom = NULL;
            if(operator_data->slow5_header->asic_temp){
                free(operator_data->slow5_header->asic_temp);
            }
            operator_data->slow5_header->asic_temp = NULL;
            if(operator_data->slow5_header->auto_update){
                free(operator_data->slow5_header->auto_update);
            }
            operator_data->slow5_header->auto_update = NULL;
            if(operator_data->slow5_header->auto_update_source){
                free(operator_data->slow5_header->auto_update_source);
            }
            operator_data->slow5_header->auto_update_source = NULL;
            if(operator_data->slow5_header->bream_is_standard){
                free(operator_data->slow5_header->bream_is_standard);
            }
            operator_data->slow5_header->bream_is_standard = NULL;
            if(operator_data->slow5_header->device_id){
                free(operator_data->slow5_header->device_id);
            }
            operator_data->slow5_header->device_id = NULL;
            if(operator_data->slow5_header->exp_script_name){
                free(operator_data->slow5_header->exp_script_name);
            }
            operator_data->slow5_header->exp_script_name = NULL;
            if(operator_data->slow5_header->exp_script_purpose){
                free(operator_data->slow5_header->exp_script_purpose);
            }
            operator_data->slow5_header->exp_script_purpose = NULL;
            if(operator_data->slow5_header->exp_start_time){
                free(operator_data->slow5_header->exp_start_time);
            }
            operator_data->slow5_header->exp_start_time = NULL;
            if(operator_data->slow5_header->flow_cell_id){
                free(operator_data->slow5_header->flow_cell_id);
            }
            operator_data->slow5_header->flow_cell_id = NULL;
            if(operator_data->slow5_header->heatsink_temp){
                free(operator_data->slow5_header->heatsink_temp);
            }
            operator_data->slow5_header->heatsink_temp = NULL;
            if(operator_data->slow5_header->hostname){
                free(operator_data->slow5_header->hostname);
            }
            operator_data->slow5_header->hostname = NULL;
            if(operator_data->slow5_header->installation_type){
                free(operator_data->slow5_header->installation_type);
            }
            operator_data->slow5_header->installation_type = NULL;
            if(operator_data->slow5_header->local_firmware_file){
                free(operator_data->slow5_header->local_firmware_file);
            }
            operator_data->slow5_header->local_firmware_file = NULL;
            if(operator_data->slow5_header->operating_system){
                free(operator_data->slow5_header->operating_system);
            }
            operator_data->slow5_header->operating_system = NULL;
            if(operator_data->slow5_header->protocol_run_id){
                free(operator_data->slow5_header->protocol_run_id);
            }
            operator_data->slow5_header->protocol_run_id = NULL;
            if(operator_data->slow5_header->protocols_version){
                free(operator_data->slow5_header->protocols_version);
            }
            operator_data->slow5_header->protocols_version = NULL;
            if(operator_data->slow5_header->tracking_id_run_id){
                free(operator_data->slow5_header->tracking_id_run_id);
            }
            operator_data->slow5_header->tracking_id_run_id = NULL;
            if(operator_data->slow5_header->usb_config){
                free(operator_data->slow5_header->usb_config);
            }
            operator_data->slow5_header->usb_config = NULL;
            if(operator_data->slow5_header->version){
                free(operator_data->slow5_header->version);
            }
            operator_data->slow5_header->version = NULL;
            //additional attributes in 2.0
            if(operator_data->slow5_header->bream_core_version){
                free(operator_data->slow5_header->bream_core_version);
            }
            operator_data->slow5_header->bream_core_version = NULL;
            if(operator_data->slow5_header->bream_ont_version){
                free(operator_data->slow5_header->bream_ont_version);
            }
            operator_data->slow5_header->bream_ont_version = NULL;
            if(operator_data->slow5_header->bream_prod_version){
                free(operator_data->slow5_header->bream_prod_version);
            }
            operator_data->slow5_header->bream_prod_version = NULL;
            if(operator_data->slow5_header->bream_rnd_version){
                free(operator_data->slow5_header->bream_rnd_version);
            }
            operator_data->slow5_header->bream_rnd_version = NULL;
            //additional attributes in 2.2
            if(operator_data->slow5_header->asic_version){
                free(operator_data->slow5_header->asic_version);
            }
            operator_data->slow5_header->asic_version = NULL;
            if(operator_data->slow5_header->configuration_version){
                free(operator_data->slow5_header->configuration_version);
            }
            operator_data->slow5_header->configuration_version = NULL;
            if(operator_data->slow5_header->device_type){
                free(operator_data->slow5_header->device_type);
            }
            operator_data->slow5_header->device_type = NULL;
            if(operator_data->slow5_header->distribution_status){
                free(operator_data->slow5_header->distribution_status);
            }
            operator_data->slow5_header->distribution_status = NULL;
            if(operator_data->slow5_header->distribution_version){
                free(operator_data->slow5_header->distribution_version);
            }
            operator_data->slow5_header->distribution_version = NULL;
            if(operator_data->slow5_header->flow_cell_product_code){
                free(operator_data->slow5_header->flow_cell_product_code);
            }
            operator_data->slow5_header->flow_cell_product_code = NULL;
            if(operator_data->slow5_header->guppy_version){
                free(operator_data->slow5_header->guppy_version);
            }
            operator_data->slow5_header->guppy_version = NULL;
            if(operator_data->slow5_header->protocol_group_id){
                free(operator_data->slow5_header->protocol_group_id);
            }
            operator_data->slow5_header->protocol_group_id = NULL;
            if(operator_data->slow5_header->sample_id){
                free(operator_data->slow5_header->sample_id);
            }
            operator_data->slow5_header->sample_id = NULL;
            break;
        default:
            fprintf(stderr,"unexpected behaviour\n");
            exit(EXIT_FAILURE);
    }
}
void reset_attributes(group_flags group_flag, operator_obj* operator_data) {
    switch (group_flag) {
        case ROOT:
            operator_data->slow5_header->file_type = NULL;
            operator_data->slow5_header->file_version = NULL;
            break;
        case READ:
            operator_data->slow5_header->run_id = NULL;
            operator_data->slow5_header->pore_type = NULL;
            break;
        case RAW:
            operator_data->slow5_record->start_time = ULONG_MAX;
            operator_data->slow5_record->duration = UINT_MAX;
            operator_data->slow5_record->read_number = -1;
            operator_data->slow5_record->start_mux = -1;
            operator_data->slow5_record->read_id = NULL;
            operator_data->slow5_record->median_before = FLT_MIN;
            operator_data->slow5_record->end_reason = '6' - '0';
            break;
        case CHANNEL_ID:
            operator_data->slow5_record->digitisation = FLT_MIN;
            operator_data->slow5_record->offset = FLT_MIN;
            operator_data->slow5_record->range = FLT_MIN;
            operator_data->slow5_record->sampling_rate = FLT_MIN;
            operator_data->slow5_record->channel_number = NULL;
            break;
        case CONTEXT_TAGS:
            operator_data->slow5_header->sample_frequency = NULL;
            //additional attributes in 2.2
            operator_data->slow5_header->barcoding_enabled = NULL;
            operator_data->slow5_header->experiment_duration_set = NULL;
            operator_data->slow5_header->sequencing_kit = NULL;
            operator_data->slow5_header->experiment_type = NULL;
            operator_data->slow5_header->local_basecalling = NULL;
            operator_data->slow5_header->package = NULL;
            operator_data->slow5_header->package_version = NULL;
            //additional attributes in 2.0
            operator_data->slow5_header->filename = NULL;
            operator_data->slow5_header->experiment_kit = NULL;
            operator_data->slow5_header->user_filename_input = NULL;
            break;
        case TRACKING_ID:
            operator_data->slow5_header->asic_id = NULL;
            operator_data->slow5_header->asic_id_eeprom = NULL;
            operator_data->slow5_header->asic_temp = NULL;
            operator_data->slow5_header->auto_update = NULL;
            operator_data->slow5_header->auto_update_source = NULL;
            operator_data->slow5_header->bream_is_standard = NULL;
            operator_data->slow5_header->device_id = NULL;
            operator_data->slow5_header->exp_script_name = NULL;
            operator_data->slow5_header->exp_script_purpose = NULL;
            operator_data->slow5_header->exp_start_time = NULL;
            operator_data->slow5_header->flow_cell_id = NULL;
            operator_data->slow5_header->heatsink_temp = NULL;
            operator_data->slow5_header->hostname = NULL;
            operator_data->slow5_header->installation_type = NULL;
            operator_data->slow5_header->local_firmware_file = NULL;
            operator_data->slow5_header->operating_system = NULL;
            operator_data->slow5_header->protocol_run_id = NULL;
            operator_data->slow5_header->protocols_version = NULL;
            operator_data->slow5_header->tracking_id_run_id = NULL;
            operator_data->slow5_header->usb_config = NULL;
            operator_data->slow5_header->version = NULL;
            //additional attributes in 2.0
            operator_data->slow5_header->bream_core_version = NULL;
            operator_data->slow5_header->bream_ont_version = NULL;
            operator_data->slow5_header->bream_prod_version = NULL;
            operator_data->slow5_header->bream_rnd_version = NULL;
            //additional attributes in 2.2
            operator_data->slow5_header->asic_version = NULL;
            operator_data->slow5_header->configuration_version = NULL;
            operator_data->slow5_header->device_type = NULL;
            operator_data->slow5_header->distribution_status = NULL;
            operator_data->slow5_header->distribution_version = NULL;
            operator_data->slow5_header->flow_cell_product_code = NULL;
            operator_data->slow5_header->guppy_version = NULL;
            operator_data->slow5_header->protocol_group_id = NULL;
            operator_data->slow5_header->sample_id = NULL;

            break;
        default:
            fprintf(stderr,"unexpected behaviour\n");
            exit(EXIT_FAILURE);
    }
}

void check_attributes(group_flags group_flag, operator_obj* operator_data) {
    switch (group_flag){
        case ROOT:
            if(operator_data->meta->verbose && operator_data->slow5_header->file_type == NULL){
                fprintf(stderr,"warning: attribute in / file_type is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->file_version == NULL){
                fprintf(stderr,"warning: attribute in / file_version is not set\n");
            }
            break;
        case READ:
            if(operator_data->meta->verbose && operator_data->slow5_header->run_id == NULL){
                fprintf(stderr,"warning: attribute in /read run_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->pore_type == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read pore_type is not set\n");
            }
            break;
        case RAW:
            if(operator_data->meta->verbose && operator_data->slow5_record->start_time == ULONG_MAX){
                fprintf(stderr,"warning: attribute in /read/Raw start_time is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->duration == UINT_MAX){
                fprintf(stderr,"warning: attribute in /read/Raw duration is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->read_number == -1){
                fprintf(stderr,"warning: attribute in /read/Raw read_number is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->start_mux == -1){
                fprintf(stderr,"warning: attribute in /read/Raw start_mux is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->read_id == NULL){
                fprintf(stderr,"warning: attribute in /read/Raw read_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->median_before == FLT_MIN){
                fprintf(stderr,"warning: attribute in /read/Raw median_before is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->end_reason == '6' - '0' && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/Raw end_reason is not set\n");
            };
            break;
        case CHANNEL_ID:
            if(operator_data->meta->verbose && operator_data->slow5_record->digitisation == FLT_MIN){
                fprintf(stderr,"warning: attribute in /read/channel_id digitisation is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->offset == FLT_MIN){
                fprintf(stderr,"warning: attribute in /read/channel_id offset is not set. read_id=%s\n",operator_data->slow5_record->read_id);
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->range == FLT_MIN){
                fprintf(stderr,"warning: attribute in /read/channel_id range is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->sampling_rate == FLT_MIN){
                fprintf(stderr,"warning: attribute in /read/channel_id sampling_rate is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_record->channel_number == NULL){
                fprintf(stderr,"warning: attribute in /read/channel_id channel_number is not set\n");
            }
            break;
        case CONTEXT_TAGS:
            if(operator_data->meta->verbose && operator_data->slow5_header->sample_frequency == NULL){
                fprintf(stderr,"warning: attribute in /read/context_tags sample_frequency is not set\n");
            }
            //additional attributes in 2.2
            if(operator_data->meta->verbose && operator_data->slow5_header->barcoding_enabled == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags barcoding_enabled is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->experiment_duration_set == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags experiment_duration_set is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->experiment_type == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags experiment_type is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->local_basecalling == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags local_basecalling is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->package == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags package is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->package_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags package_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->sequencing_kit == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/context_tags sequencing_kit is not set\n");
            }
            //additional attributes in 2.0
            if(operator_data->meta->verbose && operator_data->slow5_header->filename == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags filename is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->experiment_kit == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags experiment_kit is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->user_filename_input == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags user_filename_input is not set\n");
            }
            break;
        case TRACKING_ID:
            if(operator_data->meta->verbose && operator_data->slow5_header->asic_id == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id asic_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->asic_id_eeprom == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id asic_id_eeprom is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->asic_temp == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id asic_temp is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->auto_update == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id auto_update is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->auto_update_source == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id auto_update_source is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->bream_is_standard == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id bream_is_standard is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->device_id == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id device_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->exp_script_name == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id exp_script_name is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->exp_script_purpose == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id exp_script_purpose is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->exp_start_time == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id exp_start_time is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->flow_cell_id == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id flow_cell_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->heatsink_temp == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id heatsink_temp is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->hostname == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id hostname is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->installation_type == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id installation_type is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->local_firmware_file == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id local_firmware_file is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->operating_system == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id operating_system is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->protocol_run_id == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id protocol_run_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->protocols_version == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id protocols_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->tracking_id_run_id == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id tracking_id_run_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->usb_config == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id usb_config is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->version == NULL){
                fprintf(stderr,"warning: attribute in /read/tracking_id version is not set\n");
            }
            //additional attributes in 2.0
            if(operator_data->meta->verbose && operator_data->slow5_header->bream_core_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags bream_core_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->bream_ont_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags bream_ont_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->bream_prod_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags bream_prod_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->bream_rnd_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.2")){
                fprintf(stderr,"warning: attribute in /read/context_tags bream_rnd_version is not set\n");
            }
            //additional attributes in 2.2
            if(operator_data->meta->verbose && operator_data->slow5_header->asic_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id asic_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->configuration_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id configuration_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->device_type == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id device_type is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->distribution_status == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id distribution_status is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->distribution_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id distribution_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->flow_cell_product_code == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id flow_cell_product_code is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->guppy_version == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id guppy_version is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->protocol_group_id == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id protocol_group_id is not set\n");
            }
            if(operator_data->meta->verbose && operator_data->slow5_header->sample_id == NULL && strcmp(operator_data->slow5_header->file_version,"2.0")){
                fprintf(stderr,"warning: attribute in /read/tracking_id sample_id is not set\n");
            }
            break;
        default:
            fprintf(stderr,"unexpected behaviour\n");
            exit(EXIT_FAILURE);
    }
}

void print_header(operator_obj* operator_data) {
    check_attributes(READ, operator_data);
    check_attributes(CONTEXT_TAGS, operator_data);
    check_attributes(TRACKING_ID, operator_data);
    //  main stuff
    fprintf(operator_data->f_out,"%s", operator_data->slow5_header->file_format);
    fprintf(operator_data->f_out,"#file_version\t%s\n", operator_data->slow5_header->file_version);
    fprintf(operator_data->f_out,"#num_read_groups\t%llu\n",operator_data->slow5_header->num_read_groups);
    fprintf(operator_data->f_out,"#file_type\t%s\n", operator_data->slow5_header->file_type);

    //    READ
    fprintf(operator_data->f_out,"#pore_type\t%s\n", operator_data->slow5_header->pore_type);
    fprintf(operator_data->f_out,"#run_id\t%s\n", operator_data->slow5_header->run_id);
    //    CONTEXT_TAGS
    fprintf(operator_data->f_out,"#sample_frequency\t%s\n", operator_data->slow5_header->sample_frequency);
    //additional attributes in 2.0
    fprintf(operator_data->f_out,"#filename\t%s\n", operator_data->slow5_header->filename);
    fprintf(operator_data->f_out,"#experiment_kit\t%s\n", operator_data->slow5_header->experiment_kit);
    fprintf(operator_data->f_out,"#user_filename_input\t%s\n", operator_data->slow5_header->user_filename_input);
    //additional attributes in 2.2
    fprintf(operator_data->f_out,"#barcoding_enabled\t%s\n", operator_data->slow5_header->barcoding_enabled);
    fprintf(operator_data->f_out,"#experiment_duration_set\t%s\n", operator_data->slow5_header->experiment_duration_set);
    fprintf(operator_data->f_out,"#experiment_type\t%s\n", operator_data->slow5_header->experiment_type);
    fprintf(operator_data->f_out,"#local_basecalling\t%s\n", operator_data->slow5_header->local_basecalling);
    fprintf(operator_data->f_out,"#package\t%s\n", operator_data->slow5_header->package);
    fprintf(operator_data->f_out,"#package_version\t%s\n", operator_data->slow5_header->package_version);
    fprintf(operator_data->f_out,"#sequencing_kit\t%s\n", operator_data->slow5_header->sequencing_kit);
    //    TRACKING_ID
    fprintf(operator_data->f_out,"#asic_id\t%s\n", operator_data->slow5_header->asic_id);
    fprintf(operator_data->f_out,"#asic_id_eeprom\t%s\n", operator_data->slow5_header->asic_id_eeprom);
    fprintf(operator_data->f_out,"#asic_temp\t%s\n", operator_data->slow5_header->asic_temp);
    fprintf(operator_data->f_out,"#auto_update\t%s\n", operator_data->slow5_header->auto_update);
    fprintf(operator_data->f_out,"#auto_update_source\t%s\n", operator_data->slow5_header->auto_update_source);
    fprintf(operator_data->f_out,"#bream_is_standard\t%s\n", operator_data->slow5_header->bream_is_standard);
    fprintf(operator_data->f_out,"#device_id\t%s\n", operator_data->slow5_header->device_id);
    fprintf(operator_data->f_out,"#distribution_version\t%s\n", operator_data->slow5_header->distribution_version);
    fprintf(operator_data->f_out,"#exp_script_name\t%s\n", operator_data->slow5_header->exp_script_name);
    fprintf(operator_data->f_out,"#exp_script_purpose\t%s\n", operator_data->slow5_header->exp_script_purpose);
    fprintf(operator_data->f_out,"#exp_start_time\t%s\n", operator_data->slow5_header->exp_start_time);
    fprintf(operator_data->f_out,"#flow_cell_id\t%s\n", operator_data->slow5_header->flow_cell_id);
    fprintf(operator_data->f_out,"#heatsink_temp\t%s\n", operator_data->slow5_header->heatsink_temp);
    fprintf(operator_data->f_out,"#hostname\t%s\n", operator_data->slow5_header->hostname);
    fprintf(operator_data->f_out,"#installation_type\t%s\n", operator_data->slow5_header->installation_type);
    fprintf(operator_data->f_out,"#local_firmware_file\t%s\n", operator_data->slow5_header->local_firmware_file);
    fprintf(operator_data->f_out,"#operating_system\t%s\n", operator_data->slow5_header->operating_system);
    fprintf(operator_data->f_out,"#protocol_run_id\t%s\n", operator_data->slow5_header->protocol_run_id);
    fprintf(operator_data->f_out,"#protocols_version\t%s\n", operator_data->slow5_header->protocols_version);
    fprintf(operator_data->f_out,"#tracking_id_run_id\t%s\n", operator_data->slow5_header->tracking_id_run_id);
    fprintf(operator_data->f_out,"#usb_config\t%s\n", operator_data->slow5_header->usb_config);
    fprintf(operator_data->f_out,"#version\t%s\n", operator_data->slow5_header->version);
    //additional attributes in 2.0
    fprintf(operator_data->f_out,"#bream_core_version\t%s\n", operator_data->slow5_header->bream_core_version);
    fprintf(operator_data->f_out,"#bream_ont_version\t%s\n", operator_data->slow5_header->bream_ont_version);
    fprintf(operator_data->f_out,"#bream_prod_version\t%s\n", operator_data->slow5_header->bream_prod_version);
    fprintf(operator_data->f_out,"#bream_rnd_version\t%s\n", operator_data->slow5_header->bream_rnd_version);
    //additional attributes in 2.2
    fprintf(operator_data->f_out,"#asic_version\t%s\n", operator_data->slow5_header->asic_version);
    fprintf(operator_data->f_out,"#configuration_version\t%s\n", operator_data->slow5_header->configuration_version);
    fprintf(operator_data->f_out,"#device_type\t%s\n", operator_data->slow5_header->device_type);
    fprintf(operator_data->f_out,"#distribution_status\t%s\n", operator_data->slow5_header->distribution_status);
    fprintf(operator_data->f_out,"#flow_cell_product_code\t%s\n", operator_data->slow5_header->flow_cell_product_code);
    fprintf(operator_data->f_out,"#guppy_version\t%s\n", operator_data->slow5_header->guppy_version);
    fprintf(operator_data->f_out,"#protocol_group_id\t%s\n", operator_data->slow5_header->protocol_group_id);
    fprintf(operator_data->f_out,"#sample_id\t%s\n", operator_data->slow5_header->sample_id);

    // write the column headers
    fprintf(operator_data->f_out,"%s", COLUMN_HEADERS);
}

void print_record(operator_obj* operator_data) {
    check_attributes(RAW, operator_data);
    check_attributes(CHANNEL_ID, operator_data);
    fprintf(operator_data->f_out,"%s\t",operator_data->slow5_record->read_id);
    fprintf(operator_data->f_out,"%s\t","0");//read_group
    fprintf(operator_data->f_out,"%s\t",operator_data->slow5_record->channel_number);
    fprintf(operator_data->f_out,"%f\t",operator_data->slow5_record->digitisation);
    fprintf(operator_data->f_out,"%f\t",operator_data->slow5_record->offset);
    fprintf(operator_data->f_out,"%f\t",operator_data->slow5_record->range);
    fprintf(operator_data->f_out,"%f\t",operator_data->slow5_record->sampling_rate);
    fprintf(operator_data->f_out,"%d\t",operator_data->slow5_record->duration);

    //print_signal
    size_t i;
    for(i=0;i<operator_data->slow5_record->duration-1;i++){
        fprintf(operator_data->f_out,"%hu,", operator_data->slow5_record->raw_signal[i]);
    }
    fprintf(operator_data->f_out,"%hu\t", operator_data->slow5_record->raw_signal[i]);

    fprintf(operator_data->f_out,"%d\t",operator_data->slow5_record->read_number);
    fprintf(operator_data->f_out,"%lu\t",operator_data->slow5_record->start_time);
    fprintf(operator_data->f_out,"%u\t",operator_data->slow5_record->start_mux);

    fprintf(operator_data->f_out,"%f",operator_data->slow5_record->median_before);
//    if(strcmp(slow5_header.file_version,"2.2")){
    fprintf(operator_data->f_out,"\t%u",operator_data->slow5_record->end_reason);
    //  }
    fprintf(operator_data->f_out,"\n");
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