#ifndef SLOW5_LIBSLOW5_h
#define SLOW5_LIBSLOW5_h

#include <stdint.h>
#include <stdio.h>

//structure for a flow5 file
typedef struct{
	FILE *fp;
	uint8_t major_version;
	uint8_t minor_version;
	uint8_t slow5_or_blow5;
	uint8_t compression;    //type of compression 0 for raw bianry, 1 for gzip
}slow5_file;

//structure for a slow5 file header
typedef struct{
	char *version_str;              //slow5 version string
	uint32_t num_read_groups;       //number of read groups
    slow5_data_hdr_t **data_hdr;    //an array of pointers of size num_read_groups.
}slow5_hdr_t;
//The header for the read group 0 is accessed as header[0], 1 as header[1] and so on.
//should the header be a struct named slow5_header_t or a hash table?

//structure for a slow5 header for a particular dataset
typedef struct{
	char *file_format;
	char *exp_start_time;
	char *run_id;
    //...
}slow5_data_hdr_t;

//each slow5 record (a read) is a struct named slow5_rec_t
typedef struct{
	char *read_id;
    uint32_t read_group;
    float digitisation;
    double offset;
    double range;
    double sampling_rate;
    uint64_t size_raw_signal;
    int16_t *raw_signal;
    slow5_rec_aux_t * aux;
}slow5_rec_t;

//auxillary slow5 record data
typedef struct{
	int32_t read_number;
	uint64_t start_time;
    uint8_t start_mux;
    double median_before;
    char *end_reason;
	char *channel_number;
}slow5_rec_aux_t;

//We need a policy if nanopore changes a certain attribute/dataset name/datatype


//structure for a slow5 index
typedef struct{

}slow5_idx_t;

//the API

//open a slow5 file
slow5_file* slow5_open(char *file_name, const char * mode);

//close a slow5 file
int slow5_close(slow5_file* fp);

//read slow5 file header
slow5_hdr_t *slow5_hdr_read(slow5_file* fp);

//write a slow5 header to a file
int slow5_hdr_write(slow5_file* fp,slow5_hdr_t *hdr);

//destroy a slow5 header struct
void slow5_hdr_destroy(slow5_hdr_t *hdr);

slow5_idx_t *slow5_index_load(slow5_file* fp, char *slow5_file_name);
void slow5_index_destroy(slow5_idx_t * idx);
slow5_idx_t *slow5_index_build(char *slow5_file_name);

slow5_rec_t *slow5_rec_init();
int slow5_rec_fetch(slow5_file* fp, slow5_rec_t *rec, char *read_id);
int slow5_red_read(slow5_file* fp, slow5_rec_t *rec);
int slow5_rec_write(slow5_file* fp, slow5_rec_t *rec);
void slow5_rec_destroy(slow5_rec_t *rec);


slow5_rec_t **slow5_rec_init_batch(int batch_size);
int slow5_rec_fetch_batch(slow5_rec_t **rec_batch, char **read_id_batch, int batch_size);
void slow5_rec_destroy(slow5_rec_t **rec_batch);



#endif
