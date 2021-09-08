/**
 * @file s2f.c
 * @brief SLOW5 to FAST5 conversion
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au)
 * @date 27/02/2021
 */

#include <getopt.h>
#include <sys/wait.h>

#include <string>
#include <vector>

#include "error.h"
#include "cmd.h"
#include <slow5/slow5.h>
#include "read_fast5.h"
#include "misc.h"

#define ESSENTIAL_AUX_ATTR_COUNT (5)
#define ESSENTIAL_AUX_ATTRS ((char const*[]){ "start_time", "read_number", "start_mux" , "median_before", "channel_number"})

#define USAGE_MSG "Usage: %s [OPTION]... -d [output DIR] [SLOW5_FILE/DIR]...\n"
#define HELP_LARGE_MSG \
    "Convert SLOW5/BLOW5 files to FAST5 format.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n"       \
    "    -o, --output [FILE]        output contents to FILE. Use only when a single SLOW5_FILE is converted\n" \
    "    -d, --out-dir [STR]        output directory where files are written to\n" \
    "    -p, --iop [INT]            number of I/O processes to read fast5 files [default: 8]\n" \
    "    -h, --help                 display this message and exit\n" \

extern int slow5tools_verbosity_level;
static double init_realtime = 0;

void add_attribute(hid_t file_id, const char* attr_name, char *attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, int32_t attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, uint64_t attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, double attr_value, hid_t datatype);
void add_attribute(hid_t file_id, const char* attr_name, uint8_t attr_value, hid_t datatype);

void set_hdf5_attributes(hid_t group_id, group_flags group_flag, slow5_hdr_t *header, slow5_rec_t* slow5_record, hid_t* end_reason_enum_id) {
//    todo- check return values
    int err;
    char file_type[] = "multi-read";
    switch (group_flag) {
        char* attribute_value;
        case ROOT:
            // s2f creates multi-read fast5 files
            add_attribute(group_id,"file_type", file_type, H5T_C_S1);

            if((attribute_value=slow5_hdr_get("file_version",0,header))){
                add_attribute(group_id,"file_version", attribute_value, H5T_C_S1);
            }
            break;
        case READ:
            // add read attributes
            if((attribute_value=slow5_hdr_get("run_id",0,header))){
                add_attribute(group_id,"run_id",attribute_value,H5T_C_S1);
            }
            break;
        case RAW:
            // add Raw attributes
            if(header->aux_meta){
                uint64_t start_time = slow5_aux_get_uint64(slow5_record, "start_time", &err);
                if(err == 0){
                    add_attribute(group_id,"start_time",start_time,H5T_STD_U64LE);
                }
                int32_t read_number = slow5_aux_get_int32(slow5_record, "read_number", &err);
                if(err == 0){
                    add_attribute(group_id,"read_number", read_number,H5T_STD_I32LE);
                }
                uint8_t start_mux = slow5_aux_get_uint8(slow5_record, "start_mux", &err);
                if(err == 0){
                    add_attribute(group_id,"start_mux",start_mux,H5T_STD_U8LE);
                }
                double median_before = slow5_aux_get_double(slow5_record, "median_before", &err);
                if(err == 0){
                    add_attribute(group_id,"median_before",median_before,H5T_IEEE_F64LE);
                }
                if(check_aux_fields_in_header(header, "end_reason", 0) == 0){
                    uint8_t end_reason = slow5_aux_get_enum(slow5_record, "end_reason", &err);
                    if(err == 0){
                        add_attribute(group_id,"end_reason",end_reason,*end_reason_enum_id);
                    }
                }
            }
            add_attribute(group_id,"duration",slow5_record->len_raw_signal,H5T_STD_U32LE);
            add_attribute(group_id,"read_id",slow5_record->read_id,H5T_C_S1);
            break;
        case CHANNEL_ID:
            // add channel_id attributes
            if(header->aux_meta){
                attribute_value = slow5_aux_get_string(slow5_record, "channel_number", NULL, &err);
                if(err == 0){
                    add_attribute(group_id,"channel_number",attribute_value,H5T_C_S1);
                }
            }
            add_attribute(group_id,"digitisation",slow5_record->digitisation,H5T_IEEE_F64LE);
            add_attribute(group_id,"offset",slow5_record->offset,H5T_IEEE_F64LE);
            add_attribute(group_id,"range",slow5_record->range,H5T_IEEE_F64LE);
            add_attribute(group_id,"sampling_rate",slow5_record->sampling_rate,H5T_IEEE_F64LE);
            break;
        case CONTEXT_TAGS:
            // add context_tags attributes
            if((attribute_value=slow5_hdr_get("sample_frequency",0,header))){
                add_attribute(group_id,"sample_frequency",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("barcoding_enabled",0,header))){
                add_attribute(group_id,"barcoding_enabled",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("experiment_duration_set",0,header))){
                add_attribute(group_id,"experiment_duration_set",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("experiment_type",0,header))){
                add_attribute(group_id,"experiment_type",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("local_basecalling",0,header))){
                add_attribute(group_id,"local_basecalling",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("package",0,header))){
                add_attribute(group_id,"package",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("package_version",0,header))){
                add_attribute(group_id,"package_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("sequencing_kit",0,header))){
                add_attribute(group_id,"sequencing_kit",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("filename",0,header))){
                add_attribute(group_id,"filename",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("experiment_kit",0,header))){
                add_attribute(group_id,"experiment_kit",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("user_filename_input",0,header))){
                add_attribute(group_id,"user_filename_input",attribute_value,H5T_C_S1);
            }
            break;
        case TRACKING_ID:
            // add tracking_id attributes
            if((attribute_value=slow5_hdr_get("asic_id",0,header))){
                add_attribute(group_id,"asic_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("asic_id_eeprom",0,header))){
                add_attribute(group_id,"asic_id_eeprom",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("asic_temp",0,header))){
                add_attribute(group_id,"asic_temp",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("auto_update",0,header))){
                add_attribute(group_id,"auto_update",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("auto_update_source",0,header))){
                add_attribute(group_id,"auto_update_source",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("bream_is_standard",0,header))){
                add_attribute(group_id,"bream_is_standard",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("device_id",0,header))){
                add_attribute(group_id,"device_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("exp_script_name",0,header))){
                add_attribute(group_id,"exp_script_name",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("exp_script_purpose",0,header))){
                add_attribute(group_id,"exp_script_purpose",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("exp_start_time",0,header))){
                add_attribute(group_id,"exp_start_time",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("flow_cell_id",0,header))){
                add_attribute(group_id,"flow_cell_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("heatsink_temp",0,header))){
                add_attribute(group_id,"heatsink_temp",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("hostname",0,header))){
                add_attribute(group_id,"hostname",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("installation_type",0,header))){
                add_attribute(group_id,"installation_type",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("local_firmware_file",0,header))){
                add_attribute(group_id,"local_firmware_file",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("operating_system",0,header))){
                add_attribute(group_id,"operating_system",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("protocol_run_id",0,header))){
                add_attribute(group_id,"protocol_run_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("protocols_version",0,header))){
                add_attribute(group_id,"protocols_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("run_id",0,header))){
                add_attribute(group_id,"run_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("usb_config",0,header))){
                add_attribute(group_id,"usb_config",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("version",0,header))){
                add_attribute(group_id,"version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("asic_version",0,header))){
                add_attribute(group_id,"asic_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("configuration_version",0,header))){
                add_attribute(group_id,"configuration_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("device_type",0,header))){
                add_attribute(group_id,"device_type",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("distribution_status",0,header))){
                add_attribute(group_id,"distribution_status",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("distribution_version",0,header))){
                add_attribute(group_id,"distribution_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("flow_cell_product_code",0,header))){
                add_attribute(group_id,"flow_cell_product_code",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("guppy_version",0,header))){
                add_attribute(group_id,"guppy_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("protocol_group_id",0,header))){
                add_attribute(group_id,"protocol_group_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("sample_id",0,header))){
                add_attribute(group_id,"sample_id",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("bream_core_version",0,header))){
                add_attribute(group_id,"bream_core_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("bream_ont_version",0,header))){
                add_attribute(group_id,"bream_ont_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("bream_prod_version",0,header))){
                add_attribute(group_id,"bream_prod_version",attribute_value,H5T_C_S1);
            }
            if((attribute_value=slow5_hdr_get("bream_rnd_version",0,header))){
                add_attribute(group_id,"bream_rnd_version",attribute_value,H5T_C_S1);
            }
            break;
        default:
            ERROR("%s","Incorrect group name");
            exit(EXIT_FAILURE);
    }

}

void initialize_end_reason(slow5_hdr_t* header, hid_t* end_reason_enum_id) {
// create end_reason enum
    //    https://support.hdfgroup.org/HDF5/doc/H5.user/DatatypesEnum.html
//    end_reason_enum_id = H5Tcreate(H5T_ENUM, sizeof(uint8_t));
    *end_reason_enum_id = H5Tenum_create(H5T_STD_U8LE);
    uint8_t n;
    char **enum_labels = slow5_get_aux_enum_labels(header, "end_reason", &n);

    for(uint8_t i=0; i<n; i++){
        uint8_t val;
        H5Tenum_insert(*end_reason_enum_id, enum_labels[i], (val=i,&val));
    }
}

void write_fast5(slow5_file_t *slow5File, const char *FAST5_FILE, const char *slow5_filename) {
    hid_t   file_id;
    hid_t group_read, group_raw, group_channel_id, group_tracking_id, group_context_tags;
    herr_t  status;
    hid_t end_reason_enum_id;
    struct slow5_rec *slow5_record = NULL;

    int ret = slow5_get_next(&slow5_record, slow5File);
    if(ret < 0){
        if(ret == SLOW5_ERR_EOF){
            WARNING("No record found. Conversion skipped.%s", "");
            return;
        }else{
            ERROR("Could not read the slow5 records. exiting... %s", "");
            exit(EXIT_FAILURE);
        }
    }
    uint32_t num_essential_aux_attrs = ESSENTIAL_AUX_ATTR_COUNT;
    for(uint32_t i=0; i<num_essential_aux_attrs; i++){
        if(slow5File->header->aux_meta &&  check_aux_fields_in_header(slow5File->header, ESSENTIAL_AUX_ATTRS[i], 1)){
            ERROR("%s is missing an essential auxiliary field. s2f only creates fast5 that can be basecalled using guppy.",slow5_filename);
            exit(EXIT_FAILURE);
        }
    }
    if(slow5File->header->aux_meta &&  check_aux_fields_in_header(slow5File->header, "end_reason", 0) == 0){
        initialize_end_reason(slow5File->header, &end_reason_enum_id);
    }


    /* Create a new file using default properties. */
    file_id = H5Fcreate(FAST5_FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    set_hdf5_attributes(file_id, ROOT, slow5File->header, slow5_record, &end_reason_enum_id);

    // create first read group
    const char* read_tag = "read_";
    char read_name[strlen(read_tag)+strlen(slow5_record->read_id)];
    strcpy(read_name,read_tag);
    group_read = H5Gcreate (file_id, strcat(read_name,slow5_record->read_id), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    hid_t group_read_first = group_read;

    // create context_tags group
    group_context_tags = H5Gcreate (group_read, "context_tags", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    set_hdf5_attributes(group_context_tags, CONTEXT_TAGS, slow5File->header, slow5_record, &end_reason_enum_id);
    status = H5Gclose (group_context_tags);
    if(status<0){
        WARNING("Closing context_tags group failed. Possible memory leak. status=%d",(int)status);
    }

    // creat tracking_id group
    group_tracking_id = H5Gcreate (group_read, "tracking_id", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    set_hdf5_attributes(group_tracking_id, TRACKING_ID, slow5File->header, slow5_record, &end_reason_enum_id);
    status = H5Gclose (group_tracking_id);
    if(status<0){
        WARNING("Closing tracking_id group failed. Possible memory leak. status=%d",(int)status);
    }

    size_t i = 0;
    while(1){
        if(i){
            ret = slow5_get_next(&slow5_record, slow5File);
            if(ret < 0){
                if(ret == SLOW5_ERR_EOF){
                    break;
                }else{
                    ERROR("Could not read the slow5 records. exiting... %s", "");
                    exit(EXIT_FAILURE);
                }
            }

            // create read group
            char read_name[strlen(read_tag)+strlen(slow5_record->read_id)];
            strcpy(read_name,read_tag);
            group_read = H5Gcreate (file_id, strcat(read_name,slow5_record->read_id), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if(group_read<1){
                WARNING("A read group with read_id %s already exists, hence skipping.. \n",slow5_record->read_id);
                continue;
            }
        }

        set_hdf5_attributes(group_read, READ, slow5File->header, slow5_record, &end_reason_enum_id);
        // creat Raw group
        group_raw = H5Gcreate (group_read, "Raw", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if(i>0){
            // creat context_tags group link
            status = H5Lcreate_hard(group_read_first, "context_tags", group_read, "context_tags", H5P_DEFAULT, H5P_DEFAULT);
            // creat tracking_id group link
            status = H5Lcreate_hard(group_read_first, "tracking_id", group_read, "tracking_id", H5P_DEFAULT, H5P_DEFAULT);
        }

        // signal
        // Create the data space for the dataset
        hsize_t nsample = slow5_record->len_raw_signal;
        hsize_t dims[]      = {nsample};
        hsize_t maxdims[] = {H5S_UNLIMITED};
        hid_t dataspace_id = H5Screate_simple(1, dims, maxdims);

        //Create the dataset creation property list, add the zlib compression filter and set the chunk size.
        hsize_t chunk[] = {nsample};
        hid_t dcpl = H5Pcreate (H5P_DATASET_CREATE);
        status = H5Pset_chunk (dcpl, 1, chunk);
        status = H5Pset_deflate (dcpl, 1);

        // Create the dataset.
        hid_t dataset_id = H5Dcreate2(group_raw, "Signal", H5T_STD_I16LE, dataspace_id, H5P_DEFAULT, dcpl, H5P_DEFAULT);
        // Write the data to the dataset.
        status = H5Dwrite (dataset_id, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, slow5_record->raw_signal);
        // Close and release resources.
        status = H5Pclose (dcpl);
        status = H5Dclose(dataset_id);
        status = H5Sclose(dataspace_id);

        set_hdf5_attributes(group_raw, RAW, slow5File->header, slow5_record, &end_reason_enum_id);
        status = H5Gclose (group_raw);

        // creat channel_id group
        group_channel_id = H5Gcreate (group_read, "channel_id", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        set_hdf5_attributes(group_channel_id, CHANNEL_ID, slow5File->header, slow5_record, &end_reason_enum_id);
        status = H5Gclose (group_channel_id);

        if(i>0){
            status = H5Gclose (group_read);
        }
        i++;
        //to check if peak RAM increase over time
        //fprintf(stderr, "peak RAM = %.3f GB\n", slow5_peakrss() / 1024.0 / 1024.0 / 1024.0);
    }

    if (slow5File->header->aux_meta &&  check_aux_fields_in_header(slow5File->header, "end_reason", 0) == 0){
        H5Tclose(end_reason_enum_id);
    }
    status = H5Gclose (group_read_first);
    slow5_rec_free(slow5_record);
    status = H5Fclose(file_id);
}

void s2f_child_worker(proc_arg_t args,
                      std::vector<std::string> &slow5_files,
                      char *output_dir,
                      char* arg_fname_out,
                      program_meta *meta,
                      reads_count *readsCount) {
    for (int i = args.starti; i < args.endi; i++) {
        VERBOSE("Converting %s to fast5", slow5_files[i].c_str());
        slow5_file_t* slow5File_i = slow5_open(slow5_files[i].c_str(), "r");
        if(!slow5File_i){
            ERROR("cannot open %s. skipping...\n",slow5_files[i].c_str());
            continue;
        }
        readsCount->total_5++;
        if(slow5File_i->header->num_read_groups > 1){
            ERROR("The file %s has %u read groups. 's2f' works only with single read group slow5 files. Use 'split' to create single read group files.", slow5_files[i].c_str(), slow5File_i->header->num_read_groups);
            continue;
        }
        std::string fast5_path;
        if(output_dir){
            fast5_path = std::string(output_dir);
            std::string fast5file = slow5_files[i].substr(slow5_files[i].find_last_of('/'),
                                                          slow5_files[i].length() -
                                                          slow5_files[i].find_last_of('/') - 6) + ".fast5";
            fast5_path += fast5file;
        }
        if(arg_fname_out){
            fast5_path = std::string(arg_fname_out);
        }
        write_fast5(slow5File_i, fast5_path.c_str(), slow5_files[i].c_str());
        //  Close the slow5 file.
        slow5_close(slow5File_i);
    }
}

void s2f_iop(int iop,
             std::vector<std::string> &slow5_files,
             char *output_dir,
             char* arg_fname_out,
             program_meta *meta,
             reads_count *readsCount) {
    int64_t num_slow5_files = slow5_files.size();
    VERBOSE("%d proceses will be used",iop);
    //create processes
    pid_t* pids = (pid_t*) malloc(iop*sizeof(pid_t));
    proc_arg_t* proc_args = (proc_arg_t*)malloc(iop*sizeof(proc_arg_t));
    MALLOC_CHK(pids);
    MALLOC_CHK(proc_args);

    int32_t t;
    int32_t i = 0;
    int32_t step = (num_slow5_files + iop - 1) / iop;
    //todo : check for higher num of procs than the data
    //current works but many procs are created despite

    //set the data structures
    for (t = 0; t < iop; t++) {
        proc_args[t].starti = i;
        i += step;
        if (i > num_slow5_files) {
            proc_args[t].endi = num_slow5_files;
        } else {
            proc_args[t].endi = i;
        }
        proc_args[t].proc_index = t;
    }

    if(iop==1){
        s2f_child_worker(proc_args[0], slow5_files, output_dir, arg_fname_out, meta, readsCount);
        free(proc_args);
        free(pids);
        return;
    }

    //create processes
    STDERR("Spawning %d I/O processes to circumvent HDF hell", iop);
    for(t = 0; t < iop; t++){
        pids[t] = fork();

        if(pids[t]==-1){
            ERROR("%s","Fork failed");
            perror("");
            exit(EXIT_FAILURE);
        }
        if(pids[t]==0){ //child
            s2f_child_worker(proc_args[t],slow5_files,output_dir, arg_fname_out, meta, readsCount);
            exit(EXIT_SUCCESS);
        }
        if(pids[t]>0){ //parent
            continue;
        }
    }

    //wait for processes
    int status,w;
    for (t = 0; t < iop; t++) {
//        if(opt::verbose>1){
//            STDERR("parent : Waiting for child with pid %d",pids[t]);
//        }
        w = waitpid(pids[t], &status, 0);
        if (w == -1) {
            ERROR("%s","waitpid failed");
            perror("");
            exit(EXIT_FAILURE);
        }
        else if (WIFEXITED(status)){
//            if(opt::verbose>1){
//                STDERR("child process %d exited, status=%d", pids[t], WEXITSTATUS(status));
//            }
            if(WEXITSTATUS(status)!=0){
                ERROR("child process %d exited with status=%d",pids[t], WEXITSTATUS(status));
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (WIFSIGNALED(status)) {
                ERROR("child process %d killed by signal %d", pids[t], WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                ERROR("child process %d stopped by signal %d", pids[t], WSTOPSIG(status));
            } else {
                ERROR("child process %d did not exit propoerly: status %d", pids[t], status);
            }
            exit(EXIT_FAILURE);
        }
    }
    free(proc_args);
    free(pids);
}

int s2f_main(int argc, char **argv, struct program_meta *meta) {

    // Turn off HDF's exception printing, which is generally unhelpful for users
    H5Eset_auto(0, NULL, NULL);

    init_realtime = slow5_realtime();

    // Debug: print arguments
    print_args(argc,argv);

    // No arguments given
    if (argc <= 1) {
        fprintf(stderr, HELP_LARGE_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    static struct option long_opts[] = {
            {"help",    no_argument, NULL, 'h' }, //0
            {"output",  required_argument, NULL, 'o'},   //1
            {"out-dir", required_argument, NULL, 'd' },  //2
            { "iop",    required_argument, NULL, 'p'},   //3
            {NULL, 0, NULL, 0 }
    };

    int iop = 8;
    std::string format_low5 =  "low5";
    std::string format_fast5 =  ".fast5";

    // Input arguments
    char *arg_fname_out = NULL;
    char *arg_dir_out = NULL;

    int longindex = 0;
    int opt;
    // Parse options
    while ((opt = getopt_long(argc, argv, "h:o:d:p:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'h':
                DEBUG("displaying large help message%s","");
                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'o':
                arg_fname_out = optarg;
                break;
            case 'd':
                arg_dir_out = optarg;
                break;
            case 'p':
                iop = atoi(optarg);
                if (iop < 1) {
                    ERROR("Number of I/O processes should be larger than 0. You entered %d", iop);
                    exit(EXIT_FAILURE);
                }
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    if(arg_fname_out && arg_dir_out){
        ERROR("output file name and output directory both cannot be set%s","");
        return EXIT_FAILURE;
    }
    if(!arg_fname_out && !arg_dir_out){
        ERROR("Please set output file name or output directory%s","");
        return EXIT_FAILURE;
    }
    //measure file listing time
    std::vector<std::string> slow5_files;
    double realtime0 = slow5_realtime();
    for (int i = optind; i < argc; ++ i) {
        list_all_items(argv[i], slow5_files, 0, format_low5.c_str());
    }
    VERBOSE("%ld files found - took %.3fs",slow5_files.size(), slow5_realtime() - realtime0);
    if(slow5_files.size()==0){
        ERROR("No slow5/blow5 files found. Exiting...%s","");
        return EXIT_FAILURE;
    }
    if(slow5_files.size()==1){
        iop = 1;
    }
    if(slow5_files.size()>1 && !arg_dir_out){
        ERROR("output directory should be specified when converting more than one file%s","");
        return EXIT_FAILURE;
    }
    if(arg_dir_out){
        struct stat st = {0};
        if (stat(arg_dir_out, &st) == -1) {
            mkdir(arg_dir_out, 0700);
        }else{
            std::vector< std::string > dir_list = list_directory(arg_dir_out);
            if(dir_list.size()>2){
                ERROR("Output director %s is not empty",arg_dir_out);
                return EXIT_FAILURE;
            }
        }
    }
    if(arg_fname_out){
        std::string output_file;
        std::string extension;
        output_file = std::string(arg_fname_out);
        extension = output_file.substr(output_file.length()-6, output_file.length());
        if(extension != format_fast5){
            ERROR("Output file name extension should be %s", format_fast5.c_str());
            return EXIT_FAILURE;
        }
    }

    reads_count readsCount;
    //measure s2f conversion time
    init_realtime = slow5_realtime();
    s2f_iop(iop, slow5_files, arg_dir_out, arg_fname_out, meta, &readsCount);
    VERBOSE("Converting %ld s/blow5 files took %.3fs", slow5_files.size(), slow5_realtime() - init_realtime);

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
        attr_value = (char*)"";
        attr_length--;
    }
    H5Tset_size(atype, attr_length+1);
    attribute_id = H5Acreate2(file_id, attr_name, atype, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    /* Write the attribute data. */
    status = H5Awrite(attribute_id, atype, attr_value);
    assert(status>=0);

    H5Tclose(atype);
    /* Close the attribute. */
    status = H5Aclose(attribute_id);
    if(status<0){
        WARNING("Closing an attribute failed. Possible memory leak. status=%d",(int)status);
    }

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
    if(status<0){
        WARNING("Closing a dataspace failed. Possible memory leak. status=%d",(int)status);
    }
}
void add_attribute(hid_t file_id, const char* attr_name, int32_t attr_value, hid_t datatype) {
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
    assert(status>=0);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);
    if(status<0){
        WARNING("Closing an attribute failed. Possible memory leak. status=%d",(int)status);
    }

    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
    if(status<0){
        WARNING("Closing a dataspace failed. Possible memory leak. status=%d",(int)status);
    }
}
void add_attribute(hid_t file_id, const char* attr_name, uint64_t attr_value, hid_t datatype) {
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
    assert(status>=0);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);
    if(status<0){
        WARNING("Closing an attribute failed. Possible memory leak. status=%d",(int)status);
    }
    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
    if(status<0){
        WARNING("Closing a dataspace failed. Possible memory leak. status=%d",(int)status);
    }
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
    assert(status>=0);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);
    if(status<0){
        WARNING("Closing an attribute failed. Possible memory leak. status=%d",(int)status);
    }
    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
    if(status<0){
        WARNING("Closing a dataspace failed. Possible memory leak. status=%d",(int)status);
    }

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
    assert(status>=0);
    H5Tclose(atype);

    /* Close the attribute. */
    status = H5Aclose(attribute_id);
    if(status<0){
        WARNING("Closing an attribute failed. Possible memory leak. status=%d",(int)status);
    }
    /* Close the dataspace. */
    status = H5Sclose(dataspace_id);
    if(status<0){
        WARNING("Closing a dataspace failed. Possible memory leak. status=%d",(int)status);
    }

}
