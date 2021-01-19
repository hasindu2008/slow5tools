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

int read_header(std::vector<slow5_header_t>& slow5_headers, FILE* slow5, char** buffer, hsize_t num_read_group) {
    size_t start_idx = slow5_headers.size() - num_read_group;
    char *attribute_value;

    while (1) {
        if (read_line(slow5, buffer) == -1) {
            ERROR("cannot read line %s", "");
            return -1;
        };

        attribute_value = strtok(*buffer, "\t");
//        fprintf(stderr, "attribute value=%s ", attribute_value);

        if (strcmp("#read_id", attribute_value) == 0) {
            break;
        }

        if (strcmp("#file_format", attribute_value) == 0) {
            attribute_value = strtok(NULL, "\t\n");
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                slow5_headers[i].file_format = strdup(attribute_value);
            }
        } else if (strcmp("#file_version", attribute_value) == 0) {
            attribute_value = strtok(NULL, "\t\n");
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                slow5_headers[i].file_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#file_type", attribute_value) == 0) {
            attribute_value = strtok(NULL, "\t\n");
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                slow5_headers[i].file_type = strdup(attribute_value);
            }
        }
        else if (strcmp("#num_read_groups", attribute_value) == 0) {
            attribute_value = strtok(NULL, "\t\n");
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                slow5_headers[i].num_read_groups = atoi(attribute_value);
            }
        }
//            READ
        else if (strcmp("#pore_type", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].pore_type = strdup(attribute_value);
            }
        }
        else if (strcmp("#run_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].run_id = strdup(attribute_value);
            }
        }
//            CONTEXT_TAGS
        else if (strcmp("#sample_frequency", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].sample_frequency = strdup(attribute_value);
            }
        }
            //additional attributes in 2.0
        else if (strcmp("#filename", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].filename = strdup(attribute_value);
            }
        }
        else if (strcmp("#experiment_kit", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].experiment_kit = strdup(attribute_value);
            }
        }
        else if (strcmp("#user_filename_input", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].user_filename_input = strdup(attribute_value);
            }
        }
            //additional attributes in 2.2
        else if (strcmp("#barcoding_enabled", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].barcoding_enabled = strdup(attribute_value);
            }
        }
        else if (strcmp("#experiment_duration_set", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].experiment_duration_set = strdup(attribute_value);
            }
        }
        else if (strcmp("#experiment_type", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].experiment_type = strdup(attribute_value);
            }
        }
        else if (strcmp("#local_basecalling", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].local_basecalling = strdup(attribute_value);
            }
        }
        else if (strcmp("#package", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].package = strdup(attribute_value);
            }
        }
        else if (strcmp("#package_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].package_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#sequencing_kit", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].sequencing_kit = strdup(attribute_value);
            }
        }
//            TRACKING_ID
        else if (strcmp("#asic_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].asic_id = strdup(attribute_value);
            }
        }
        else if (strcmp("#asic_id_eeprom", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].asic_id_eeprom = strdup(attribute_value);
            }
        }
        else if (strcmp("#asic_temp", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].asic_temp = strdup(attribute_value);
            }
        }
        else if (strcmp("#auto_update", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].auto_update = strdup(attribute_value);
            }
        }
        else if (strcmp("#auto_update_source", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].auto_update_source = strdup(attribute_value);
            }
        }
        else if (strcmp("#bream_is_standard", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].bream_is_standard = strdup(attribute_value);
            }
        }
        else if (strcmp("#device_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].device_id = strdup(attribute_value);
            }
        }
        else if (strcmp("#exp_script_name", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].exp_script_name = strdup(attribute_value);
            }
        }
        else if (strcmp("#exp_script_purpose", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].exp_script_purpose = strdup(attribute_value);
            }
        }
        else if (strcmp("#exp_start_time", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].exp_start_time = strdup(attribute_value);
            }
        }
        else if (strcmp("#flow_cell_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].flow_cell_id = strdup(attribute_value);
            }
        }
        else if (strcmp("#heatsink_temp", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].heatsink_temp = strdup(attribute_value);
            }
        }
        else if (strcmp("#hostname", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].hostname = strdup(attribute_value);
            }
        }
        else if (strcmp("#installation_type", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].installation_type = strdup(attribute_value);
            }
        }
        else if (strcmp("#local_firmware_file", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].local_firmware_file = strdup(attribute_value);
            }
        }
        else if (strcmp("#operating_system", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].operating_system = strdup(attribute_value);
            }
        }
        else if (strcmp("#protocol_run_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].protocol_run_id = strdup(attribute_value);
            }
        }
        else if (strcmp("#protocols_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].protocols_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#tracking_id_run_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].tracking_id_run_id = strdup(attribute_value);
            }
        }
        else if (strcmp("#usb_config", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].usb_config = strdup(attribute_value);
            }
        }
        else if (strcmp("#version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].version = strdup(attribute_value);
            }
        }
            //additional attributes in 2.0
        else if (strcmp("#bream_core_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].bream_core_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#bream_ont_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].bream_ont_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#bream_prod_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].bream_prod_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#bream_rnd_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].bream_rnd_version = strdup(attribute_value);
            }
        }
            //additional attributes in 2.2
        else if (strcmp("#asic_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].asic_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#configuration_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].configuration_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#device_type", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].device_type = strdup(attribute_value);
            }
        }
        else if (strcmp("#distribution_status", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].distribution_status = strdup(attribute_value);
            }
        }
        else if (strcmp("#distribution_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].distribution_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#flow_cell_product_code", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].flow_cell_product_code = strdup(attribute_value);
            }
        }
        else if (strcmp("#guppy_version", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].guppy_version = strdup(attribute_value);
            }
        }
        else if (strcmp("#protocol_group_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].protocol_group_id = strdup(attribute_value);
            }
        }
        else if (strcmp("#sample_id", attribute_value) == 0) {
            for (size_t i = start_idx; i < slow5_headers.size(); i++) {
                attribute_value = strtok(NULL, "\t\n");
                slow5_headers[i].sample_id = strdup(attribute_value);
            }
        }

        else {
            fprintf(stderr, "[%s] unidentified header line %s\n", __func__, *buffer);
        }
    }
}

int read_slow5_header(FILE *slow5, std::vector<slow5_header_t>& slow5Headers, hsize_t num_read_group) {

    char *buffer = NULL;
    if(read_header(slow5Headers, slow5, &buffer, num_read_group)==-1){
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

void print_multi_group_records(FILE *f_out, std::vector<FILE*>& slow_files_pointers, std::vector<size_t> &run_id_indices, std::vector<std::vector<size_t>> &list, size_t read_group_count, std::vector<size_t> &file_id_tracker) {
    for(size_t i=0; i<read_group_count; i++){
        for(size_t j=0; j<list[run_id_indices[i]].size(); j++){
            size_t prev_read_group_id;
            size_t first_record = 1;
            while(1){
                char* buffer = NULL;
                if(read_line(slow_files_pointers[file_id_tracker[list[run_id_indices[i]][j]]],&buffer)==-1){
                    break;
                }
                char read_id[50];
                size_t read_group_id;
                if(sscanf(buffer, "%[^\t]\t%lu", read_id, &read_group_id)!=2){
                    ERROR("Slow5 format error in line: %s",*buffer);
                }
                if(first_record==0){
                    if(read_group_id!=prev_read_group_id){
                        fseek(slow_files_pointers[file_id_tracker[list[run_id_indices[i]][j]]],-sizeof(char)*strlen(buffer),SEEK_CUR);
                        free(buffer);
                        break;
                    }
                }
                fprintf(f_out,"%s\t%lu\t%s", read_id, i, buffer+strlen(read_id)+strlen("\t")+snprintf(0,0,"%lu",i)+strlen("\t"));
                prev_read_group_id = read_group_id;
                first_record=0;
                free(buffer);
            }
        }
    }
}

int find_num_read_group(FILE* slow5, hsize_t* num_read_group){
    char *buffer = NULL;
    if(read_line(slow5, &buffer)==-1 || read_line(slow5, &buffer)==-1 || read_line(slow5, &buffer)==-1){
        return -1;
    }
    char key_name[32];
    char value[500];
//        fprintf(stderr,"%s",*buffer);
    if(sscanf(buffer, "#%[^\t]\t%[^\n]\n", key_name,value)!=2){
        ERROR("Slow5 format error in line: %s",*buffer);
        return -1;
    }
    if(strcmp("num_read_groups",key_name)==0){
        *num_read_group = atoi(value);
    } else{
        ERROR("Slow5 format error in line: %s",*buffer);
        return -1;
    }
    fseek(slow5,0,SEEK_SET);

    if(buffer){
        free(buffer);
    }
    return 1;
}