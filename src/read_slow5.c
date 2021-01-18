//
// Created by Hiruna on 2021-01-16.
//

#include "slow5.h"
#include "error.h"

size_t number_of_digits(size_t i);

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

int read_header(slow5_header_t* slow5_header, FILE* slow5, char** buffer) {

    while(1){
        if(read_line(slow5, buffer)==-1){
            ERROR("cannot read line %s","");
            return -1;
        }; // check return value
        char key_name[32];
        char value[500];
//        fprintf(stderr,"%s",*buffer);
        if(sscanf(*buffer, "#%[^\t]\t%[^\n]\n", key_name,value)!=2){
            ERROR("Slow5 format error in line: %s",*buffer);
            return -1;
        }
        if(strcmp("read_id",key_name)==0){
            break;  //column headers
        }

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

int read_single_group_slow5_header(FILE *slow5, slow5_header_t& slow5Header) {

    char *buffer = NULL;
    if(read_header(&slow5Header, slow5, &buffer)==-1){
        return -1;
    }
    if(buffer){
        free(buffer);
    }
    return 1;
}

void print_multi_group_header(FILE *f_out, std::vector<slow5_header_t>& slow5_headers, std::vector<size_t> &run_id_indices, std::vector<std::vector<size_t>> &list, size_t read_group_count) {
    fprintf(f_out, "#file_format\t%s\n", slow5_headers[0].file_format);
    fprintf(f_out, "#file_version\t%s\n", slow5_headers[0].file_version);
    fprintf(f_out, "#num_read_groups\t%llu\n", read_group_count);

    //fprintf(f_out, "#run_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].run_id);}fprintf(f_out, "\n");

    //the big part
    fprintf(f_out, "#file_type");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].file_type);}fprintf(f_out, "\n");
    fprintf(f_out, "#pore_type");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].pore_type);}fprintf(f_out, "\n");
    fprintf(f_out, "#run_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].run_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#sample_frequency");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].sample_frequency);}fprintf(f_out, "\n");
    fprintf(f_out, "#filename");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].filename);}fprintf(f_out, "\n");
    fprintf(f_out, "#experiment_kit");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].experiment_kit);}fprintf(f_out, "\n");
    fprintf(f_out, "#user_filename_input");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].user_filename_input);}fprintf(f_out, "\n");
    fprintf(f_out, "#barcoding_enabled");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].barcoding_enabled);}fprintf(f_out, "\n");
    fprintf(f_out, "#experiment_duration_set");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].experiment_duration_set);}fprintf(f_out, "\n");
    fprintf(f_out, "#experiment_type");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].experiment_type);}fprintf(f_out, "\n");
    fprintf(f_out, "#local_basecalling");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].local_basecalling);}fprintf(f_out, "\n");
    fprintf(f_out, "#package");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].package);}fprintf(f_out, "\n");
    fprintf(f_out, "#package_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].package_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#sequencing_kit");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].sequencing_kit);}fprintf(f_out, "\n");
    fprintf(f_out, "#asic_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].asic_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#asic_id_eeprom");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].asic_id_eeprom);}fprintf(f_out, "\n");
    fprintf(f_out, "#asic_temp");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].asic_temp);}fprintf(f_out, "\n");
    fprintf(f_out, "#auto_update");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].auto_update);}fprintf(f_out, "\n");
    fprintf(f_out, "#auto_update_source");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].auto_update_source);}fprintf(f_out, "\n");
    fprintf(f_out, "#bream_is_standard");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].bream_is_standard);}fprintf(f_out, "\n");
    fprintf(f_out, "#device_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].device_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#distribution_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].distribution_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#exp_script_name");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].exp_script_name);}fprintf(f_out, "\n");
    fprintf(f_out, "#exp_script_purpose");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].exp_script_purpose);}fprintf(f_out, "\n");
    fprintf(f_out, "#exp_start_time");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].exp_start_time);}fprintf(f_out, "\n");
    fprintf(f_out, "#flow_cell_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].flow_cell_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#heatsink_temp");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].heatsink_temp);}fprintf(f_out, "\n");
    fprintf(f_out, "#hostname");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].hostname);}fprintf(f_out, "\n");
    fprintf(f_out, "#installation_type");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].installation_type);}fprintf(f_out, "\n");
    fprintf(f_out, "#local_firmware_file");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].local_firmware_file);}fprintf(f_out, "\n");
    fprintf(f_out, "#operating_system");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].operating_system);}fprintf(f_out, "\n");
    fprintf(f_out, "#protocol_run_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].protocol_run_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#protocols_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].protocols_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#tracking_id_run_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].tracking_id_run_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#usb_config");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].usb_config);}fprintf(f_out, "\n");
    fprintf(f_out, "#version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].version);}fprintf(f_out, "\n");
    fprintf(f_out, "#bream_core_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].bream_core_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#bream_ont_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].bream_ont_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#bream_prod_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].bream_prod_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#bream_rnd_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].bream_rnd_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#asic_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].asic_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#configuration_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].configuration_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#device_type");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].device_type);}fprintf(f_out, "\n");
    fprintf(f_out, "#distribution_status");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].distribution_status);}fprintf(f_out, "\n");
    fprintf(f_out, "#flow_cell_product_code");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].flow_cell_product_code);}fprintf(f_out, "\n");
    fprintf(f_out, "#guppy_version");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].guppy_version);}fprintf(f_out, "\n");
    fprintf(f_out, "#protocol_group_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].protocol_group_id);}fprintf(f_out, "\n");
    fprintf(f_out, "#sample_id");for(size_t i=0; i<read_group_count; i++){fprintf(f_out, "\t%s", slow5_headers[list[run_id_indices[i]][0]].sample_id);}fprintf(f_out, "\n");

    fprintf(f_out,"%s", COLUMN_HEADERS);
}

void print_multi_group_records(FILE *f_out, std::vector<FILE*>& slow_files_pointers, std::vector<size_t> &run_id_indices, std::vector<std::vector<size_t>> &list, size_t read_group_count) {
    char* buffer = NULL;
    for(size_t i=0; i<read_group_count; i++){
        for(size_t j=0; j<list[run_id_indices[i]].size(); j++){
            read_line(slow_files_pointers[list[run_id_indices[i]][j]],&buffer);
            char read_id[50];
            if(sscanf(buffer, "%[^\t]", read_id)!=1){
                ERROR("Slow5 format error in line: %s",*buffer);
            }
            fprintf(f_out,"%s\t%lu\t%s", read_id, i, buffer+strlen(read_id)+strlen("\t")+snprintf(0,0,"%lu",i)+strlen("\t"));
        }
    }

    free(buffer);
}
