//
// Created by shan on 2020-12-22.
//

#include <string>
#include <vector>

#include "slow5.h"
#include "error.h"
#include "slow5_extra.h"
#include "read_fast5.h"

#define WARNING_LIMIT 1

// Operator function to be called by H5Aiterate.
herr_t fast5_attribute_itr (hid_t loc_id, const char *name, const H5A_info_t  *info, void *op_data);
// Operator function to be called by H5Literate.
herr_t fast5_group_itr (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);

// from nanopolish_fast5_io.cpp
static inline  std::string fast5_get_string_attribute(fast5_file_t fh, const std::string& group_name, const std::string& attribute_name);

//other functions
int group_check(struct operator_obj *od, haddr_t target_addr);

void print_slow5_header(operator_obj* operator_data) {
    if(slow5_hdr_fwrite(operator_data->slow5File->fp, operator_data->slow5File->header, operator_data->format_out, operator_data->pressMethod) == -1){
        fprintf(stderr, "Could not write the header\n");
        exit(EXIT_FAILURE);
    }
}

void print_record(operator_obj* operator_data) {
    if(slow5_rec_fwrite(operator_data->slow5File->fp, operator_data->slow5_record, operator_data->slow5File->header->aux_meta, operator_data->format_out, operator_data->press_ptr) == -1){
        fprintf(stderr, "Could not write the record %s\n", operator_data->slow5_record->read_id);
        exit(EXIT_FAILURE);
    }
}

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
    switch(H5Tclass){
        case H5T_STRING:
            flag_value_string = 1;
            if(H5Tis_variable_str(attribute_type) > 0) {
                // variable length string
                ret = H5Aread(attribute, native_type, &value.attr_string);
                if(ret < 0) {
                    ERROR("error reading attribute %s", name);
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
                    ERROR("error reading attribute %s", name);
                    exit(EXIT_FAILURE);
                }
            }
            if (value.attr_string && !value.attr_string[0]) {
                WARNING("[%s] Empty: Attribute value of %s/%s is an empty string", SLOW5_FILE_FORMAT_SHORT, operator_data->group_name, name);
                if(flag_value_string){ // hack to skip the free() at the bottom of the function
                    free(value.attr_string);
                    flag_value_string = 0;
                }
                value.attr_string = (char*)"";
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
            WARNING("Unknown: %s\n", name);
    }
    H5Tclose(native_type);
    H5Tclose(attribute_type);
    H5Aclose(attribute);

    if(H5Tclass==H5T_STRING){
        size_t index = 0;
        while(value.attr_string[index]){
            int result = isspace(value.attr_string[index]);
            if (result && value.attr_string[index]!=' '){
                ERROR("Attribute '%s' has a value '%s' with white spaces", name, value.attr_string);
                exit(EXIT_FAILURE);
            }
            index++;
        }
    }

    if(strcmp("file_type",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("file_type", value.attr_string, 0, operator_data->slow5File->header) ==-1){
            WARNING("file_type attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("file_version",name)==0){
        if (H5Tclass == H5T_STRING) {
            if(slow5_hdr_set("file_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
                WARNING("file_version attribute value could not be set in the slow5 header %s", "");
            }
        } else if (H5Tclass == H5T_FLOAT) {
            char buf[50];
            sprintf(buf,"%.1f", value.attr_double);
            WARNING("Converting the attribute %s/%s from H5T_FLOAT to string ",operator_data->group_name,name);
            if(slow5_hdr_set("file_version", buf, 0, operator_data->slow5File->header) == -1){
                WARNING("file_version attribute value could not be set in the slow5 header %s", "");
            }
        }
    }
//            READ
    else if(strcmp("run_id",name)==0 && H5Tclass==H5T_STRING){
        *(operator_data->flag_run_id) = 1;
        char* run_id_stored = slow5_hdr_get("run_id", 0, operator_data->slow5File->header);
        if(strcmp(run_id_stored,"")==0 || strcmp(run_id_stored,value.attr_string)==0){
            if(slow5_hdr_set("run_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
                WARNING("run_id attribute value could not be set in the slow5 header %s", "");
            }
            if(slow5_hdr_set("tracking_id_run_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
                WARNING("tracking_id_run_id attribute value could not be set in the slow5 header %s", "");
            }
        }else if(*(operator_data->flag_allow_run_id_mismatch)){
            if(*(operator_data->warning_flag_allow_run_id_mismatch)==WARNING_LIMIT){
                WARNING("[%s] Different run_ids found in a single fast5 file. Arbitrary run_id will be set in slow5 header. This warning is suppressed now onwards.", SLOW5_FILE_FORMAT_SHORT);
            }
            else if(*(operator_data->warning_flag_allow_run_id_mismatch)<WARNING_LIMIT){
                WARNING("[%s] Different run_ids found in a single fast5 file. Arbitrary run_id will be set in slow5 header.", SLOW5_FILE_FORMAT_SHORT);
            }
            *(operator_data->warning_flag_allow_run_id_mismatch) = *(operator_data->warning_flag_allow_run_id_mismatch) + 1;
        }else{
            ERROR("Different run_ids found in a single fast5 file. Cannot create a single header slow5/blow5. Please use --out-dir option.If you are using single-fast5 files make sure they have the same run_id%s", "");
            exit(EXIT_FAILURE);
        }
    }
//            RAW
    else if(strcmp("start_time",name)==0 && H5Tclass==H5T_INTEGER && *(operator_data->flag_lossy) == 0){
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "start_time", &value.attr_int) != 0){
            WARNING("start_time auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("duration",name)==0 && H5Tclass==H5T_INTEGER){
        operator_data->slow5_record->len_raw_signal = value.attr_int;
    }
    else if(strcmp("read_number",name)==0 && H5Tclass==H5T_INTEGER && *(operator_data->flag_lossy) == 0){
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "read_number", &value.attr_int) != 0){
            WARNING("read_number auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("start_mux",name)==0 && H5Tclass==H5T_INTEGER && *(operator_data->flag_lossy) == 0){
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "start_mux", &value.attr_int) != 0){
            WARNING("start_mux auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
    else if(strcmp("read_id",name)==0 && H5Tclass==H5T_STRING){
        //make sure read_id has a proper starting character
        if(isalpha(value.attr_string[0]) || isdigit(value.attr_string[0])){
            operator_data->slow5_record->read_id_len = strlen(value.attr_string);
            operator_data->slow5_record->read_id = strdup(value.attr_string);
        }else{
            ERROR("read_id of this format is not supported [%s]", value.attr_string);
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp("median_before",name)==0 && H5Tclass==H5T_FLOAT && *(operator_data->flag_lossy) == 0){
        if(slow5_rec_set(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "median_before", &value.attr_double) != 0){
            WARNING("median_before auxiliary attribute value could not be set in the slow5 record %s", "");
        }
    }
//    else if(strcmp("end_reason",name)==0 && H5Tclass==H5T_ENUM){
//        operator_data->slow5_record->end_reason = value.attr_uint8_t;
//    }
//            CHANNEL_ID
    else if(strcmp("channel_number",name)==0 && H5Tclass==H5T_STRING && *(operator_data->flag_lossy) == 0){
        if(slow5_rec_set_string(operator_data->slow5_record, operator_data->slow5File->header->aux_meta, "channel_number", value.attr_string) != 0){
            WARNING("channel_number auxiliary attribute value could not be set in the slow5 record %s", "");
        }
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
        if(slow5_hdr_set("sample_frequency", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("sample_frequency attribute value could not be set in the slow5 header %s", "");
        }
    }
        //additional attributes in 2.2
    else if(strcmp("barcoding_enabled",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("barcoding_enabled", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("barcoding_enabled attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("experiment_duration_set",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("experiment_duration_set", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("experiment_duration_set attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("experiment_type",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("experiment_type", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("experiment_type attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("local_basecalling",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("local_basecalling", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("local_basecalling attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("package",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("package", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("package attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("package_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("package_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("package_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("sequencing_kit",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("sequencing_kit", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("sequencing_kit attribute value could not be set in the slow5 header %s", "");
        }
    }
        //additional attributes in 2.0
    else if(strcmp("filename",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("filename", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("filename attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("experiment_kit",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("experiment_kit", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("experiment_kit attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("user_filename_input",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("user_filename_input", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("user_filename_input attribute value could not be set in the slow5 header %s", "");
        }
    }
//            TRACKING_ID
    else if(strcmp("asic_id",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("asic_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("asic_id attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("asic_id_eeprom",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("asic_id_eeprom", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("asic_id_eeprom attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("asic_temp",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("asic_temp", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("asic_temp attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("auto_update",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("auto_update", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("auto_update attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("auto_update_source",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("auto_update_source", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("auto_update_source attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("bream_is_standard",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("bream_is_standard", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("bream_is_standard attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("device_id",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("device_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("device_id attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("exp_script_name",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("exp_script_name", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("exp_script_name attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("exp_script_purpose",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("exp_script_purpose", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("exp_script_purpose attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("exp_start_time",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("exp_start_time", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("exp_start_time attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("flow_cell_id",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("flow_cell_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("flow_cell_id attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("heatsink_temp",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("heatsink_temp", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("heatsink_temp attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("hostname",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("hostname", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("hostname attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("installation_type",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("installation_type", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("installation_type attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("local_firmware_file",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("local_firmware_file", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("local_firmware_file attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("operating_system",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("operating_system", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("operating_system attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("protocol_run_id",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("protocol_run_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("protocol_run_id attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("protocols_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("protocols_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("protocols_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("usb_config",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("usb_config", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("usb_config attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("version attribute value could not be set in the slow5 header %s", "");
        }
    }
        //additional attributes in 2.0
    else if(strcmp("bream_core_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("bream_core_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("bream_core_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("bream_ont_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("bream_ont_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("bream_ont_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("bream_prod_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("bream_prod_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("bream_prod_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("bream_rnd_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("bream_rnd_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("bream_rnd_version attribute value could not be set in the slow5 header %s", "");
        }
    }
        //additional attributes in 2.2
    else if(strcmp("asic_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("asic_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("asic_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("configuration_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("configuration_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("configuration_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("device_type",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("device_type", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("device_type attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("distribution_status",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("distribution_status", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("distribution_status attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("distribution_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("distribution_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("distribution_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("flow_cell_product_code",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("flow_cell_product_code", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("flow_cell_product_code attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("guppy_version",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("guppy_version", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("guppy_version attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("protocol_group_id",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("protocol_group_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("protocol_group_id attribute value could not be set in the slow5 header %s", "");
        }
    }
    else if(strcmp("sample_id",name)==0 && H5Tclass==H5T_STRING){
        if(slow5_hdr_set("sample_id", value.attr_string, 0, operator_data->slow5File->header) == -1){
            WARNING("sample_id attribute value could not be set in the slow5 header %s", "");
        }
    }else{
        if(strcmp("read_number",name) && strcmp("start_mux",name) && strcmp("start_time",name) && strcmp("median_before",name) && strcmp("channel_number",name)){
            int ret;
            int is_missing = 0;
            khint_t iter = 0;
            char key[strlen(name)+1];
            // copying str1 to str2
            strcpy(key, name);
            kh_warncount_s * warncount_hash = *operator_data->warncount_hash;
            iter = kh_get(warncount, warncount_hash, key);          // query the hash table
            is_missing = (iter == kh_end(warncount_hash));   // test if the key is present
            if(is_missing){
                iter = kh_put(warncount, warncount_hash, key, &ret);     // insert a key to the hash table
                kh_value(warncount_hash, iter) = 1;             // set the value
                WARNING("[%s] Not Stored: Attribute %s/%s is not stored", SLOW5_FILE_FORMAT_SHORT, operator_data->group_name, name);
            }else{
                uint32_t value = kh_value(warncount_hash, iter);
                if(value < WARNING_LIMIT){
                    WARNING("[%s] Not Stored: Attribute %s/%s is not stored", SLOW5_FILE_FORMAT_SHORT, operator_data->group_name, name);
                    kh_value(warncount_hash, iter) = ++value;
                }else if(value == WARNING_LIMIT){
                    WARNING("[%s] Not Stored: Attribute %s/%s is not stored. This warning is suppressed now onwards.", SLOW5_FILE_FORMAT_SHORT, operator_data->group_name, name);
                    kh_value(warncount_hash, iter) = WARNING_LIMIT+1;
                }
            }
        }
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

int
read_fast5(fast5_file_t *fast5_file, slow5_fmt format_out, press_method pressMethod, int lossy, int write_header_flag,
           int flag_allow_run_id_mismatch, struct program_meta *meta, slow5_file_t *slow5File, kh_warncount_t **warncount_hash) {

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
    tracker.press_ptr = press_init(pressMethod);
    tracker.fast5_path = fast5_file->fast5_path;
    tracker.slow5File = slow5File;

    int flag_context_tags = 0;
    int flag_tracking_id = 0;
    int flag_run_id = 0;
    int flag_lossy = lossy;
    size_t zero0 = 0;
    size_t zero2 = 0;

    tracker.flag_context_tags = &flag_context_tags;
    tracker.flag_tracking_id = &flag_tracking_id;
    tracker.flag_run_id = &flag_run_id;
    tracker.flag_lossy = &flag_lossy;
    tracker.flag_write_header = &write_header_flag;
    tracker.flag_allow_run_id_mismatch = &flag_allow_run_id_mismatch;
    tracker.nreads = &zero0;
    tracker.warning_flag_allow_run_id_mismatch = &zero2;
    tracker.slow5_record = slow5_rec_init();
    tracker.group_name = "";
    tracker.warncount_hash = warncount_hash;

    slow5_hdr_set("SLOW5_file_format", SLOW5_FILE_FORMAT_SHORT, 0, tracker.slow5File->header);

    if (fast5_file->is_multi_fast5) {
        hsize_t number_of_groups = 0;
        H5Gget_num_objs(fast5_file->hdf5_file,&number_of_groups);
        tracker.num_read_groups = &number_of_groups; //todo:check if the assumption is valid
        //obtain the root group attributes
        H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &tracker);
        //now iterate over read groups. loading records and writing them are done inside fast5_group_itr
        H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, fast5_group_itr, (void *) &tracker);
    }else{ // single-fast5
        //obtain the root group attributes
        H5Aiterate2(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &tracker);
        hsize_t number_of_groups = 1;
        tracker.num_read_groups = &number_of_groups;
        //now iterate over read groups. loading records and writing them are done inside fast5_group_itr
        H5Literate(fast5_file->hdf5_file, H5_INDEX_NAME, H5_ITER_INC, NULL, fast5_group_itr, (void *) &tracker);
        //        todo: compare header values with the previous singlefast5
        if(*(tracker.flag_run_id) != 1){
            ERROR("run_id is not set%s", ".");
            exit(EXIT_FAILURE);
        }
        if(write_header_flag == 0){
            print_slow5_header(&tracker);
        }
        print_record(&tracker);
        slow5_rec_free(tracker.slow5_record);
    }
    press_free(tracker.press_ptr);
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
                        H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                        *(operator_data->flag_tracking_id) = 1;
                    } else if (strcmp(name, "context_tags") == 0 && *(operator_data->flag_context_tags) == 0) {
                        H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                        *(operator_data->flag_context_tags) = 1;
                    } else if (strcmp(name, "tracking_id") != 0 && strcmp(name, "context_tags") != 0) {
                        H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                    }
                }else{
                    H5Aiterate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, 0, fast5_attribute_itr, (void *) &next_op);
                }

                H5Gclose(group);
                //the recursive call
                return_val = H5Literate_by_name(loc_id, name, H5_INDEX_NAME, H5_ITER_INC, 0, fast5_group_itr,
                                                (void *) &next_op, H5P_DEFAULT);

                if (operator_data->fast5_file->is_multi_fast5) {
                    //check if we are at a root-level group
                    if (operator_data->group_level == ROOT) {
                        if (*(operator_data->nreads) == 0) {
                            if (*(operator_data->flag_context_tags) != 1) {
                                ERROR("The first read does not have context_tags information%s", ".");
                                exit(EXIT_FAILURE);
                            }
                            if (*(operator_data->flag_tracking_id) != 1) {
                                ERROR("The first read does not have tracking_id information%s", ".");
                                exit(EXIT_FAILURE);
                            }
                            if(*(operator_data->flag_run_id) != 1){
                                ERROR("run_id is not set%s", ".");
                                exit(EXIT_FAILURE);
                            }
                            if(*(operator_data->flag_write_header) == 0){
                                print_slow5_header(operator_data);
                            }
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
        STDERR("Looking for '%s' files in %s", extension, path.c_str());
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


void slow5_hdr_initialize(slow5_hdr *header, int lossy){
    slow5_hdr_add_rg(header);
    header->num_read_groups = 1;

    struct slow5_aux_meta *aux_meta = slow5_aux_meta_init_empty();
    if(lossy == 0) {
        if(slow5_aux_meta_add(aux_meta, "channel_number", STRING)){
            ERROR("Could not initialize the record attribute '%s'", "channel_number");
            exit(EXIT_FAILURE);
        }
        if(slow5_aux_meta_add(aux_meta, "median_before", DOUBLE)){
            ERROR("Could not initialize the record attribute '%s'", "median_before");
            exit(EXIT_FAILURE);
        }
        if(slow5_aux_meta_add(aux_meta, "read_number", INT32_T)){
            ERROR("Could not initialize the record attribute '%s'", "read_number");
            exit(EXIT_FAILURE);
        }
        if(slow5_aux_meta_add(aux_meta, "start_mux", UINT8_T)){
            ERROR("Could not initialize the record attribute '%s'", "start_mux");
            exit(EXIT_FAILURE);
        }
        if(slow5_aux_meta_add(aux_meta, "start_time", UINT64_T)){
            ERROR("Could not initialize the record attribute '%s'", "start_time");
            exit(EXIT_FAILURE);
        }
        //    todo - add end_reason enum
    }

    header->aux_meta = aux_meta;

    //  main stuff
    if(slow5_hdr_add_attr("file_format", header) || slow5_hdr_set("file_format", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "file_format");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("file_version", header) || slow5_hdr_set("file_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "file_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("file_type", header) || slow5_hdr_set("file_type", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "file_type");
        exit(EXIT_FAILURE);
    }
    //    READ
    if(slow5_hdr_add_attr("run_id", header) || slow5_hdr_set("run_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "run_id");
        exit(EXIT_FAILURE);
    }
    //    CONTEXT_TAGS
    if(slow5_hdr_add_attr("sample_frequency", header) || slow5_hdr_set("sample_frequency", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "sample_frequency");
        exit(EXIT_FAILURE);
    }
    //additional attributes in 2.0
    if(slow5_hdr_add_attr("filename", header) || slow5_hdr_set("filename", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "filename");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("experiment_kit", header) || slow5_hdr_set("experiment_kit", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "experiment_kit");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("user_filename_input", header) || slow5_hdr_set("user_filename_input", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "user_filename_input");
        exit(EXIT_FAILURE);
    }
    //additional attributes in 2.2
    if(slow5_hdr_add_attr("barcoding_enabled", header) || slow5_hdr_set("barcoding_enabled", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "barcoding_enabled");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("experiment_duration_set", header) || slow5_hdr_set("experiment_duration_set", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "experiment_duration_set");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("experiment_type", header) || slow5_hdr_set("experiment_type", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "experiment_type");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("local_basecalling", header) || slow5_hdr_set("local_basecalling", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "local_basecalling");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("package", header) || slow5_hdr_set("package", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "package");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("package_version", header) || slow5_hdr_set("package_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "package_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("sequencing_kit", header) || slow5_hdr_set("sequencing_kit", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "sequencing_kit");
        exit(EXIT_FAILURE);
    }
    //    TRACKING_ID
    if(slow5_hdr_add_attr("asic_id", header) || slow5_hdr_set("asic_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "asic_id");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("asic_id_eeprom", header) || slow5_hdr_set("asic_id_eeprom", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "asic_id_eeprom");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("asic_temp", header) || slow5_hdr_set("asic_temp", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "asic_temp");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("auto_update", header) || slow5_hdr_set("auto_update", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "auto_update");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("auto_update_source", header) || slow5_hdr_set("auto_update_source", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "auto_update_source");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("bream_is_standard", header) || slow5_hdr_set("bream_is_standard", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "bream_is_standard");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("device_id", header) || slow5_hdr_set("device_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "device_id");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("distribution_version", header) || slow5_hdr_set("distribution_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "distribution_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("exp_script_name", header) || slow5_hdr_set("exp_script_name", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "exp_script_name");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("exp_script_purpose", header) || slow5_hdr_set("exp_script_purpose", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "exp_script_purpose");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("exp_start_time", header) || slow5_hdr_set("exp_start_time", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "exp_start_time");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("flow_cell_id", header) || slow5_hdr_set("flow_cell_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "flow_cell_id");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("heatsink_temp", header) || slow5_hdr_set("heatsink_temp", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "heatsink_temp");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("hostname", header) || slow5_hdr_set("hostname", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "hostname");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("installation_type", header) || slow5_hdr_set("installation_type", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "installation_type");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("local_firmware_file", header) || slow5_hdr_set("local_firmware_file", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "local_firmware_file");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("operating_system", header) || slow5_hdr_set("operating_system", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "operating_system");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("protocol_run_id", header) || slow5_hdr_set("protocol_run_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "protocol_run_id");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("protocols_version", header) || slow5_hdr_set("protocols_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "protocols_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("tracking_id_run_id", header) || slow5_hdr_set("tracking_id_run_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "tracking_id_run_id");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("usb_config", header) || slow5_hdr_set("usb_config", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "usb_config");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("version", header) || slow5_hdr_set("version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "version");
        exit(EXIT_FAILURE);
    }
    //additional attributes in 2.0
    if(slow5_hdr_add_attr("bream_core_version", header) || slow5_hdr_set("bream_core_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "bream_core_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("bream_ont_version", header) || slow5_hdr_set("bream_ont_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "bream_ont_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("bream_prod_version", header) || slow5_hdr_set("bream_prod_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "bream_prod_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("bream_rnd_version", header) || slow5_hdr_set("bream_rnd_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "bream_rnd_version");
        exit(EXIT_FAILURE);
    }
    //additional attributes in 2.2
    if(slow5_hdr_add_attr("asic_version", header) || slow5_hdr_set("asic_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "asic_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("configuration_version", header) || slow5_hdr_set("configuration_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "configuration_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("device_type", header) || slow5_hdr_set("device_type", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "device_type");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("distribution_status", header) || slow5_hdr_set("distribution_status", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "distribution_status");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("flow_cell_product_code", header) || slow5_hdr_set("flow_cell_product_code", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "flow_cell_product_code");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("guppy_version", header) || slow5_hdr_set("guppy_version", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "guppy_version");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("protocol_group_id", header) || slow5_hdr_set("protocol_group_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "protocol_group_id");
        exit(EXIT_FAILURE);
    }
    if(slow5_hdr_add_attr("sample_id", header) || slow5_hdr_set("sample_id", "", 0, header)){
        ERROR("Could not initialize the header attribute '%s'", "sample_id");
        exit(EXIT_FAILURE);
    }

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
