//
// Created by shan on 2020-12-29.
//

#include "fast5lite.h"
#include "slow5.h"
#include "error.h"

#define HEADER_LINES 51 // delete this later and implement a better method


#define USAGE_MSG "Usage: %s [OPTION]... [SLOW5_FILE/DIR]...\n"
#define HELP_SMALL_MSG "Try '%s --help' for more information.\n"
#define HELP_LARGE_MSG \
    "Convert slow5 or (compressed) blow5 file(s) to fast5.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    "    -h, --help             display this message and exit\n" \
    "    -o, --output=[DIR]    output converted contents to DIR\n"           \


static double init_realtime = 0;
static uint64_t bad_fast5_file = 0;
static uint64_t total_reads = 0;


void add_attribute(hid_t file_id, const char* attr_name, char *attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, int attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, unsigned long attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, unsigned int attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, double attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, uint8_t attr_value, hid_t datatype);

int read_line(FILE* slow5, char ** buffer){
    if(*buffer){
        free(*buffer);
    }
    size_t buf_size = 0;
    if (getline(buffer, &buf_size, slow5) == -1) {
        if(buf_size>0){
            free(*buffer);
        }
        return -1;
    }
//    fprintf(stderr,"%s\n",*buffer);
    return 1;
}

void read_header(slow5_header_t* slow5_header, FILE* slow5, char** buffer) {
    for(size_t i=0; i<HEADER_LINES; i++){
        if(read_line(slow5, buffer)==-1){
            fprintf(stderr, "cannot read line");
            break;
        }; // check return value
        char key_name[32];
        char value[100];
//        fprintf(stderr,"%s",buffer);
        sscanf(*buffer, "#%32[^=]=%s\n", key_name,value);
//        fprintf(stderr,"%s=%s\n",key_name,value);

        if(strcmp("file_format",key_name)==0){
            slow5_header->file_format = strdup(value);
        }
        else if(strcmp("file_version",key_name)==0){
            slow5_header->file_version = strdup(value);
        }
        else if(strcmp("file_type",key_name)==0){
            slow5_header->file_type = strdup(value);
        }
        else if(strcmp("num_read_groups",key_name)==0){
            slow5_header->num_read_groups = atoi(value);
        }
//            READ

        else if(strcmp("pore_type",key_name)==0){
            slow5_header->pore_type = strdup(value);
        }
        else if(strcmp("run_id",key_name)==0){
            slow5_header->run_id = strdup(value);
        }
//            CONTEXT_TAGS
        else if(strcmp("sample_frequency",key_name)==0){
            slow5_header->sample_frequency = strdup(value);
        }
            //additional attributes in 2.0
        else if(strcmp("filename",key_name)==0){
            slow5_header->filename = strdup(value);
        }
        else if(strcmp("experiment_kit",key_name)==0){
            slow5_header->experiment_kit = strdup(value);
        }
        else if(strcmp("user_filename_input",key_name)==0){
            slow5_header->user_filename_input = strdup(value);
        }
            //additional attributes in 2.2
        else if(strcmp("barcoding_enabled",key_name)==0){
            slow5_header->barcoding_enabled = strdup(value);
        }
        else if(strcmp("experiment_duration_set",key_name)==0){
            slow5_header->experiment_duration_set = strdup(value);
        }
        else if(strcmp("experiment_type",key_name)==0){
            slow5_header->experiment_type = strdup(value);
        }
        else if(strcmp("local_basecalling",key_name)==0){
            slow5_header->local_basecalling = strdup(value);
        }
        else if(strcmp("package",key_name)==0){
            slow5_header->package = strdup(value);
        }
        else if(strcmp("package_version",key_name)==0){
            slow5_header->package_version = strdup(value);
        }
        else if(strcmp("sequencing_kit",key_name)==0){
            slow5_header->sequencing_kit = strdup(value);
        }
//            TRACKING_ID
        else if(strcmp("asic_id",key_name)==0){
            slow5_header->asic_id = strdup(value);
        }
        else if(strcmp("asic_id_eeprom",key_name)==0){
            slow5_header->asic_id_eeprom = strdup(value);
        }
        else if(strcmp("asic_temp",key_name)==0){
            slow5_header->asic_temp = strdup(value);
        }
        else if(strcmp("auto_update",key_name)==0){
            slow5_header->auto_update = strdup(value);
        }
        else if(strcmp("auto_update_source",key_name)==0){
            slow5_header->auto_update_source = strdup(value);
        }
        else if(strcmp("bream_is_standard",key_name)==0){
            slow5_header->bream_is_standard = strdup(value);
        }
        else if(strcmp("device_id",key_name)==0){
            slow5_header->device_id = strdup(value);
        }
        else if(strcmp("exp_script_name",key_name)==0){
            slow5_header->exp_script_name = strdup(value);
        }
        else if(strcmp("exp_script_purpose",key_name)==0){
            slow5_header->exp_script_purpose = strdup(value);
        }
        else if(strcmp("exp_start_time",key_name)==0){
            slow5_header->exp_start_time = strdup(value);
        }
        else if(strcmp("flow_cell_id",key_name)==0){
            slow5_header->flow_cell_id = strdup(value);
        }
        else if(strcmp("heatsink_temp",key_name)==0){
            slow5_header->heatsink_temp = strdup(value);
        }
        else if(strcmp("hostname",key_name)==0){
            slow5_header->hostname = strdup(value);
        }
        else if(strcmp("installation_type",key_name)==0){
            slow5_header->installation_type = strdup(value);
        }
        else if(strcmp("local_firmware_file",key_name)==0){
            slow5_header->local_firmware_file = strdup(value);
        }
        else if(strcmp("operating_system",key_name)==0){
            slow5_header->operating_system = strdup(value);
        }
        else if(strcmp("protocol_run_id",key_name)==0){
            slow5_header->protocol_run_id = strdup(value);
        }
        else if(strcmp("protocols_version",key_name)==0){
            slow5_header->protocols_version = strdup(value);
        }
        else if(strcmp("tracking_id_run_id",key_name)==0){
            slow5_header->tracking_id_run_id = strdup(value);
        }
        else if(strcmp("usb_config",key_name)==0){
            slow5_header->usb_config = strdup(value);
        }
        else if(strcmp("version",key_name)==0){
            slow5_header->version = strdup(value);
        }
            //additional attributes in 2.0
        else if(strcmp("bream_core_version",key_name)==0){
            slow5_header->bream_core_version = strdup(value);
        }
        else if(strcmp("bream_ont_version",key_name)==0){
            slow5_header->bream_ont_version = strdup(value);
        }
        else if(strcmp("bream_prod_version",key_name)==0){
            slow5_header->bream_prod_version = strdup(value);
        }
        else if(strcmp("bream_rnd_version",key_name)==0){
            slow5_header->bream_rnd_version = strdup(value);
        }
            //additional attributes in 2.2
        else if(strcmp("asic_version",key_name)==0){
            slow5_header->asic_version = strdup(value);
        }
        else if(strcmp("configuration_version",key_name)==0){
            slow5_header->configuration_version = strdup(value);
        }
        else if(strcmp("device_type",key_name)==0){
            slow5_header->device_type = strdup(value);
        }
        else if(strcmp("distribution_status",key_name)==0){
            slow5_header->distribution_status = strdup(value);
        }
        else if(strcmp("distribution_version",key_name)==0){
            slow5_header->distribution_version = strdup(value);
        }
        else if(strcmp("flow_cell_product_code",key_name)==0){
            slow5_header->flow_cell_product_code = strdup(value);
        }
        else if(strcmp("guppy_version",key_name)==0){
            slow5_header->guppy_version = strdup(value);
        }
        else if(strcmp("protocol_group_id",key_name)==0){
            slow5_header->protocol_group_id = strdup(value);
        }
        else if(strcmp("sample_id",key_name)==0){
            slow5_header->sample_id = strdup(value);
        }else{
            fprintf(stderr,"[%s] unidentified header key,value pair %s\n",__func__ , *buffer);
        }
    }
}

void set_hdf5_attributes(hid_t group_id, group_flags group_flag, slow5_header_t* slow5_header, slow5_record_t* slow5_record, hid_t* end_reason_enum_id) {
    switch (group_flag) {
        case ROOT:
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"file_type",slow5_header->file_type,H5T_C_S1);
            add_attribute(group_id,"file_version",slow5_header->file_version,H5T_C_S1);
            break;
        case READ:
            // add read attributes
            add_attribute(group_id,"run_id",slow5_header->run_id,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"pore_type",slow5_header->pore_type,H5T_C_S1);
            break;
        case RAW:
            // add Raw attributes
            add_attribute(group_id,"start_time",slow5_record->start_time,H5T_STD_U64LE);
            add_attribute(group_id,"duration",slow5_record->duration,H5T_STD_U32LE);
            add_attribute(group_id,"read_number",slow5_record->read_number,H5T_STD_I32LE);
            add_attribute(group_id,"start_mux",slow5_record->start_mux,H5T_STD_U8LE);
            add_attribute(group_id,"read_id",slow5_record->read_id,H5T_C_S1);
            add_attribute(group_id,"median_before",slow5_record->median_before,H5T_IEEE_F64LE);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"end_reason",slow5_record->end_reason,*end_reason_enum_id);
            break;
        case CHANNEL_ID:
            // add channel_id attributes
            add_attribute(group_id,"channel_number",slow5_record->channel_number,H5T_C_S1);
            add_attribute(group_id,"digitisation",slow5_record->digitisation,H5T_IEEE_F64LE);
            add_attribute(group_id,"offset",slow5_record->offset,H5T_IEEE_F64LE);
            add_attribute(group_id,"range",slow5_record->range,H5T_IEEE_F64LE);
            add_attribute(group_id,"sampling_rate",slow5_record->sampling_rate,H5T_IEEE_F64LE);
            free(slow5_record->channel_number);
            break;
        case CONTEXT_TAGS:
            // add context_tags attributes
            add_attribute(group_id,"sample_frequency",slow5_header->sample_frequency,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"barcoding_enabled",slow5_header->barcoding_enabled,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"experiment_duration_set",slow5_header->experiment_duration_set,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"experiment_type",slow5_header->experiment_type,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"local_basecalling",slow5_header->local_basecalling,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"package",slow5_header->package,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"package_version",slow5_header->package_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"sequencing_kit",slow5_header->sequencing_kit,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"filename",slow5_header->filename,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"experiment_kit",slow5_header->experiment_kit,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"user_filename_input",slow5_header->user_filename_input,H5T_C_S1);
            free(slow5_header->sample_frequency);
            free(slow5_header->barcoding_enabled);
            free(slow5_header->experiment_duration_set);
            free(slow5_header->experiment_type);
            free(slow5_header->local_basecalling);
            free(slow5_header->package);
            free(slow5_header->package_version);
            free(slow5_header->sequencing_kit);
            free(slow5_header->filename);
            free(slow5_header->experiment_kit);
            free(slow5_header->user_filename_input);
            break;
        case TRACKING_ID:
            // add tracking_id attributes
            add_attribute(group_id,"asic_id",slow5_header->asic_id,H5T_C_S1);
            add_attribute(group_id,"asic_id_eeprom",slow5_header->asic_id_eeprom,H5T_C_S1);
            add_attribute(group_id,"asic_temp",slow5_header->asic_temp,H5T_C_S1);
            add_attribute(group_id,"auto_update",slow5_header->auto_update,H5T_C_S1);
            add_attribute(group_id,"auto_update_source",slow5_header->auto_update_source,H5T_C_S1);
            add_attribute(group_id,"bream_is_standard",slow5_header->bream_is_standard,H5T_C_S1);
            add_attribute(group_id,"device_id",slow5_header->device_id,H5T_C_S1);
            add_attribute(group_id,"exp_script_name",slow5_header->exp_script_name,H5T_C_S1);
            add_attribute(group_id,"exp_script_purpose",slow5_header->exp_script_purpose,H5T_C_S1);
            add_attribute(group_id,"exp_start_time",slow5_header->exp_start_time,H5T_C_S1);
            add_attribute(group_id,"flow_cell_id",slow5_header->flow_cell_id,H5T_C_S1);
            add_attribute(group_id,"heatsink_temp",slow5_header->heatsink_temp,H5T_C_S1);
            add_attribute(group_id,"hostname",slow5_header->hostname,H5T_C_S1);
            add_attribute(group_id,"installation_type",slow5_header->installation_type,H5T_C_S1);
            add_attribute(group_id,"local_firmware_file",slow5_header->local_firmware_file,H5T_C_S1);
            add_attribute(group_id,"operating_system",slow5_header->operating_system,H5T_C_S1);
            add_attribute(group_id,"protocol_run_id",slow5_header->protocol_run_id,H5T_C_S1);
            add_attribute(group_id,"protocols_version",slow5_header->protocols_version,H5T_C_S1);
            add_attribute(group_id,"run_id",slow5_header->tracking_id_run_id,H5T_C_S1);
            add_attribute(group_id,"usb_config",slow5_header->usb_config,H5T_C_S1);
            add_attribute(group_id,"version",slow5_header->version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"asic_version",slow5_header->asic_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"configuration_version",slow5_header->configuration_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"device_type",slow5_header->device_type,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"distribution_status",slow5_header->distribution_status,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"distribution_version",slow5_header->distribution_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"flow_cell_product_code",slow5_header->flow_cell_product_code,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"guppy_version",slow5_header->guppy_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"protocol_group_id",slow5_header->protocol_group_id,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.2"))add_attribute(group_id,"sample_id",slow5_header->sample_id,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"bream_core_version",slow5_header->bream_core_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"bream_ont_version",slow5_header->bream_ont_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"bream_prod_version",slow5_header->bream_prod_version,H5T_C_S1);
            if(!strcmp(slow5_header->file_version,"2.0"))add_attribute(group_id,"bream_rnd_version",slow5_header->bream_rnd_version,H5T_C_S1);

            free(slow5_header->asic_id);
            free(slow5_header->asic_id_eeprom);
            free(slow5_header->asic_temp);
            free(slow5_header->auto_update);
            free(slow5_header->auto_update_source);
            free(slow5_header->bream_is_standard);
            free(slow5_header->device_id);
            free(slow5_header->exp_script_name);
            free(slow5_header->exp_script_purpose);
            free(slow5_header->exp_start_time);
            free(slow5_header->flow_cell_id);
            free(slow5_header->heatsink_temp);
            free(slow5_header->hostname);
            free(slow5_header->installation_type);
            free(slow5_header->local_firmware_file);
            free(slow5_header->operating_system);
            free(slow5_header->protocol_run_id);
            free(slow5_header->protocols_version);
            free(slow5_header->tracking_id_run_id);
            free(slow5_header->usb_config);
            free(slow5_header->version);
            free(slow5_header->asic_version);
            //2.2
            free(slow5_header->configuration_version);
            free(slow5_header->device_type);
            free(slow5_header->distribution_status);
            free(slow5_header->distribution_version);
            free(slow5_header->flow_cell_product_code);
            free(slow5_header->guppy_version);
            free(slow5_header->protocol_group_id);
            free(slow5_header->sample_id);
            //2.0
            free(slow5_header->bream_core_version);
            free(slow5_header->bream_ont_version);
            free(slow5_header->bream_prod_version);
            free(slow5_header->bream_rnd_version);
            break;
    }
}

void initialize_end_reason(hid_t* end_reason_enum_id) {
// create end_reason enum
    //    https://support.hdfgroup.org/HDF5/doc/H5.user/DatatypesEnum.html
//    end_reason_enum_id = H5Tcreate(H5T_ENUM, sizeof(uint8_t));
    *end_reason_enum_id = H5Tenum_create(H5T_STD_U8LE);
    uint8_t val;
    H5Tenum_insert(*end_reason_enum_id, "unknown",   (val=0,&val));
    H5Tenum_insert(*end_reason_enum_id, "partial", (val=1,&val));
    H5Tenum_insert(*end_reason_enum_id, "mux_change",  (val=2,&val));
    H5Tenum_insert(*end_reason_enum_id, "unblock_mux_change", (val=3,&val));
    H5Tenum_insert(*end_reason_enum_id, "signal_positive", (val=4,&val));
    H5Tenum_insert(*end_reason_enum_id, "signal_negative", (val=5,&val));
}

int slow5_to_fast5(const char *SLOW5_FILE) {

    char* attribute_value;
    slow5_header_t slow5_header;
    slow5_record_t slow5_record;

    hid_t   file_id;
    hid_t group_read, group_raw, group_channel_id, group_tracking_id, group_context_tags;
    herr_t  status;
    hid_t end_reason_enum_id;
    initialize_end_reason(&end_reason_enum_id);

    size_t file_length = strlen(SLOW5_FILE);
    char FAST5_FILE[file_length];
    memcpy(FAST5_FILE, &SLOW5_FILE[0], file_length - 5);
    FAST5_FILE[file_length - 5] = 'f';FAST5_FILE[file_length - 4] = 'a';FAST5_FILE[file_length - 3] = 's';FAST5_FILE[file_length - 2] = 't';FAST5_FILE[file_length - 1] = '5';FAST5_FILE[file_length] = '\0';
    fprintf(stderr, "%s\n", FAST5_FILE);

    FILE* slow5;
    slow5 = fopen(SLOW5_FILE, "r"); // read mode
    if (slow5 == NULL){
        fprintf(stdout,"%s\n",SLOW5_FILE);
        perror("Error while opening input file.\n");
        exit(EXIT_FAILURE);
    }
    char * buffer = NULL;
    read_header(&slow5_header, slow5, &buffer); // fill slow5_header values

    /* Create a new file using default properties. */
    file_id = H5Fcreate(FAST5_FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    set_hdf5_attributes(file_id, ROOT, &slow5_header, &slow5_record, &end_reason_enum_id);


    read_line(slow5, &buffer); //read column headers
    read_line(slow5, &buffer);    //read first read
    attribute_value = strtok(buffer, "\t");
    slow5_record.read_id = strdup(attribute_value);

    // create first read group
    const char* read_tag = "read_";
    char read_name[strlen(read_tag)+strlen(slow5_record.read_id)];
    strcpy(read_name,read_tag);
    group_read = H5Gcreate (file_id, strcat(read_name,slow5_record.read_id), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    hid_t group_read_first = group_read;

    // create context_tags group
    group_context_tags = H5Gcreate (group_read, "context_tags", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    set_hdf5_attributes(group_context_tags, CONTEXT_TAGS, &slow5_header, &slow5_record, &end_reason_enum_id);
    status = H5Gclose (group_context_tags);

    // creat tracking_id group
    group_tracking_id = H5Gcreate (group_read, "tracking_id", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    set_hdf5_attributes(group_tracking_id, TRACKING_ID, &slow5_header, &slow5_record, &end_reason_enum_id);
    status = H5Gclose (group_tracking_id);

    for(size_t i = 0; i < slow5_header.num_read_groups; i++){
        if(i){
            read_line(slow5, &buffer);
            attribute_value = strtok(buffer, "\t");
            slow5_record.read_id = strdup(attribute_value);
            // create read group
            char read_name[strlen(read_tag)+strlen(slow5_record.read_id)];
            strcpy(read_name,read_tag);
            group_read = H5Gcreate (file_id, strcat(read_name,slow5_record.read_id), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        }
        attribute_value = strtok(NULL, ",\t");
        slow5_record.channel_number = strdup(attribute_value);
        attribute_value = strtok(NULL, ",\t");
        slow5_record.digitisation = atof(attribute_value);
        attribute_value = strtok(NULL, ",\t");
        slow5_record.offset = atof(attribute_value);
        attribute_value = strtok(NULL, "\t");
        slow5_record.range = atof(attribute_value);
        attribute_value = strtok(NULL, "\t");
        slow5_record.sampling_rate = atof(attribute_value);
        attribute_value = strtok(NULL, ",\t");
        slow5_record.duration = atoi(attribute_value);

        set_hdf5_attributes(group_read, READ, &slow5_header, &slow5_record, &end_reason_enum_id);
        // creat Raw group
        group_raw = H5Gcreate (group_read, "Raw", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        if(i>0){
            // creat context_tags group link
            status = H5Lcreate_hard(group_read_first, "context_tags", group_read, "context_tags", H5P_DEFAULT, H5P_DEFAULT);
            // creat tracking_id group link
            status = H5Lcreate_hard(group_read_first, "tracking_id", group_read, "tracking_id", H5P_DEFAULT, H5P_DEFAULT);
        }

        // signal
        int16_t* rawptr = (int16_t*)calloc(slow5_record.duration, sizeof(int16_t));
        int16_t* temp_rawptr = rawptr;
        for(size_t i=0; i<slow5_record.duration-1; i++){
            attribute_value = strtok(NULL, ",");
            int16_t temp_value = atoi(attribute_value);
            *temp_rawptr = temp_value;
            temp_rawptr++;
        }
        attribute_value = strtok(NULL, "\t");
        int16_t temp_value = atoi(attribute_value);
        *temp_rawptr = temp_value;

        // Create the data space for the dataset
        hsize_t nsample = slow5_record.duration;
        hsize_t dims[]      = {nsample};
        hsize_t maxdims[] = {H5S_UNLIMITED};
        hid_t dataspace_id = H5Screate_simple(1, dims, maxdims);

        //Create the dataset creation property list, add the gzip compression filter and set the chunk size.
        hsize_t chunk[] = {nsample};
        hid_t dcpl = H5Pcreate (H5P_DATASET_CREATE);
        status = H5Pset_chunk (dcpl, 1, chunk);
        status = H5Pset_deflate (dcpl, 1);

        // Create the dataset.
        hid_t dataset_id = H5Dcreate2(group_raw, "Signal", H5T_STD_I16LE, dataspace_id, H5P_DEFAULT, dcpl, H5P_DEFAULT);

        // Write the data to the dataset.
        status = H5Dwrite (dataset_id, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawptr);

        // Close and release resources.
        status = H5Pclose (dcpl);
        status = H5Dclose(dataset_id);
        status = H5Sclose(dataspace_id);

        attribute_value = strtok(NULL, "\t");
        slow5_record.read_number = atoi(attribute_value);
        attribute_value = strtok(NULL, "\t");
        slow5_record.start_time = strtoul(attribute_value,NULL,10);
        attribute_value = strtok(NULL, "\t");
        slow5_record.start_mux = atoi(attribute_value);
        attribute_value = strtok(NULL, "\t");
        slow5_record.median_before = atof(attribute_value);
        attribute_value = strtok(NULL, "\n");
        slow5_record.end_reason = attribute_value[0] - '0';

        set_hdf5_attributes(group_raw, RAW, &slow5_header, &slow5_record, &end_reason_enum_id);
        status = H5Gclose (group_raw);

        // creat channel_id group
        group_channel_id = H5Gcreate (group_read, "channel_id", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        set_hdf5_attributes(group_channel_id, CHANNEL_ID, &slow5_header, &slow5_record, &end_reason_enum_id);
        status = H5Gclose (group_channel_id);

        if(i>0){
            status = H5Gclose (group_read);
        }
        free(rawptr);
        free(slow5_record.read_id);


        if(i == slow5_header.num_read_groups-1)break;

        //to check if peak RAM increase over time
//    fprintf(stderr, "peak RAM = %.3f GB\n", peakrss() / 1024.0 / 1024.0 / 1024.0);

    }

    H5Tclose(end_reason_enum_id);
    if(buffer)free(buffer);
    status = H5Gclose (group_read_first);
    free((char*)slow5_header.file_format);
    free(slow5_header.file_version);
    free(slow5_header.file_type);
    free(slow5_header.pore_type);
    free(slow5_header.run_id);
    //  Close the files.
    fclose(slow5);
    status = H5Fclose(file_id);
    return 0;

}

bool has_slow5_ext(const char *f_path) {
    bool ret = false;
    if (f_path != NULL) {
        size_t f_path_len = strlen(f_path);
        size_t slow5_ext_len = strlen(SLOW5_EXTENSION);
        if (f_path_len >= slow5_ext_len &&
            strcmp(f_path + (f_path_len - slow5_ext_len), SLOW5_EXTENSION) == 0) {
            ret = true;
        }
    }
    return ret;
}

void recurse_slow5_dir(const char *f_path) {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(f_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // If it has the fast5 extension
            if (has_slow5_ext(f_path)) {
                // Open FAST5 and convert to SLOW5 into f_out
                slow5_to_fast5(f_path);
            }

        } else {
            WARNING("File '%s' failed to open - %s.",
                    f_path, strerror(errno));
        }

    } else {
        fprintf(stderr, "[%s::%.3f*%.2f] Extracting slow5 from %s\n", __func__,
                realtime() - init_realtime, cputime() / (realtime() - init_realtime), f_path);

        // Iterate through sub files
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 &&
                strcmp(ent->d_name, "..") != 0) {

                // Make sub path string
                // f_path + '/' + ent->d_name + '\0'
                size_t sub_f_path_len = strlen(f_path) + 1 + strlen(ent->d_name) + 1;
                char *sub_f_path = (char *) malloc(sizeof *sub_f_path * sub_f_path_len);
                MALLOC_CHK(sub_f_path);
                snprintf(sub_f_path, sub_f_path_len, "%s/%s", f_path, ent->d_name);

                // Recurse
                recurse_slow5_dir(sub_f_path);

                free(sub_f_path);
                sub_f_path = NULL;
            }
        }

        closedir(dir);
    }
}

int s2f_main(int argc, char **argv, struct program_meta *meta) {

    init_realtime = realtime();
    int ret; // For checking return values of functions

    // Debug: print arguments
    if (meta != NULL && meta->debug) {
        if (meta->verbose) {
            VERBOSE("printing the arguments given%s","");
        }

        fprintf(stderr, DEBUG_PREFIX "argv=[",
                __FILE__, __func__, __LINE__);
        for (int i = 0; i < argc; ++ i) {
            fprintf(stderr, "\"%s\"", argv[i]);
            if (i == argc - 1) {
                fprintf(stderr, "]");
            } else {
                fprintf(stderr, ", ");
            }
        }
        fprintf(stderr, NO_COLOUR);
    }

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h' },
            {"output", required_argument, NULL, 'o' },
            {NULL, 0, NULL, 0 }
    };

    // Input arguments
    char *arg_dir_out = NULL;

    char opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "h:o:", long_opts, NULL)) != -1) {

        if (meta->debug) {
            DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        }

        switch (opt) {
            case 'h':
                if (meta->verbose) {
                    VERBOSE("displaying large help message%s","");
                }
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                return EXIT_SUCCESS;
            case 'o':
                arg_dir_out = optarg;
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    for (int i = optind; i < argc; ++ i) {
//        fprintf(stderr,"%s",argv[i]);
        // Recursive way
        recurse_slow5_dir(argv[i]);

        // TODO iterative way
    }

    return EXIT_SUCCESS;
}


void add_attribute(hid_t file_id, const char* attr_name, char *attr_value, hid_t datatype) {
    /* Create the data space for the attribute. */
    herr_t  status;
    hid_t   dataspace_id, attribute_id; /* identifiers */
    dataspace_id = H5Screate(H5S_SCALAR);
    /* Create a dataset attribute. */
    hid_t atype = H5Tcopy(datatype);
    size_t attr_length = strlen(attr_value);
    if(strcmp(attr_value,".")==0 && attr_length==1){
        WARNING("Attribute with empty value '%s' %s", attr_name, attr_value);
//        fprintf(stderr,"warning empty value %s %s\n", attr_name, attr_value);
        attr_value = (char*)"";
        attr_length--;
    }
    H5Tset_size(atype, attr_length+1);
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, attr_value);
    H5Tclose(atype);
    /* Close the attribute. */
    status = H5Aclose(attribute_id);

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
}

void add_attribute(hid_t file_id, const char* attr_name, int attr_value, hid_t datatype) {
    /* Create the data space for the attribute. */
    herr_t  status;
    hid_t   dataspace_id, attribute_id; /* identifiers */
    dataspace_id = H5Screate(H5S_SCALAR);
    /* Create a dataset attribute. */
    hid_t atype = H5Tcopy(datatype);
//    H5Tset_size(atype, strlen(attr_value));
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, &attr_value);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
}

void add_attribute(hid_t file_id, const char* attr_name, unsigned long attr_value, hid_t datatype) {
    /* Create the data space for the attribute. */
    herr_t  status;
    hid_t   dataspace_id, attribute_id; /* identifiers */
    dataspace_id = H5Screate(H5S_SCALAR);
    /* Create a dataset attribute. */
    hid_t atype = H5Tcopy(datatype);
//    H5Tset_size(atype, strlen(attr_value));
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, &attr_value);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
}

void add_attribute(hid_t file_id, const char* attr_name, unsigned int attr_value, hid_t datatype) {
    /* Create the data space for the attribute. */
    herr_t  status;
    hid_t   dataspace_id, attribute_id; /* identifiers */
    dataspace_id = H5Screate(H5S_SCALAR);
    /* Create a dataset attribute. */
    hid_t atype = H5Tcopy(datatype);
//    H5Tset_size(atype, strlen(attr_value));
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, &attr_value);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
}

void add_attribute(hid_t file_id, const char* attr_name, double attr_value, hid_t datatype) {
    /* Create the data space for the attribute. */
    herr_t  status;
    hid_t   dataspace_id, attribute_id; /* identifiers */
    dataspace_id = H5Screate(H5S_SCALAR);
    /* Create a dataset attribute. */
    hid_t atype = H5Tcopy(datatype);
//    H5Tset_size(atype, strlen(attr_value));
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, &attr_value);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
}

void add_attribute(hid_t file_id, const char* attr_name, uint8_t attr_value, hid_t datatype) {
    /* Create the data space for the attribute. */
    herr_t  status;
    hid_t   dataspace_id, attribute_id; /* identifiers */
    dataspace_id = H5Screate(H5S_SCALAR);
    /* Create a dataset attribute. */
    hid_t atype = H5Tcopy(datatype);
//    H5Tset_size(atype, strlen(attr_value));
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, &attr_value);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
}