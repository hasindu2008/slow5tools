#ifndef SLOW5TOOLS_H
#define SLOW5TOOLS_H

#include <dirent.h>
#include "slow5.h"


// slow5 file
struct slow5_file *slow5_init(FILE *fp, const char *pathname, enum slow5_fmt format);

// slow5 header
struct slow5_hdr *slow5_hdr_init(FILE *fp, enum slow5_fmt format);

// slow5 header data
khash_t(s2s) **slow5_hdr_data_init(FILE *fp, enum slow5_fmt format, char *buf, size_t cap, uint32_t num_rgs);
void slow5_hdr_data_free(khash_t(s2s) **hdr_data, uint32_t num_rgs);

// slow5 record
void slow5_rec_free_most(struct slow5_rec *read);
int slow5_rec_parse(char *read_str, const char *read_id, struct slow5_rec *read, enum slow5_fmt format);

// slow5 extension parsing
enum slow5_fmt name_get_slow5_fmt(const char *name);
enum slow5_fmt path_get_slow5_fmt(const char *path);
const char *slow5_fmt_get_name(enum slow5_fmt format);
char *get_slow5_idx_path(const char *path);













/* SLOW5 Extra API */

/*
// Convert fast5 files to a slow5 file
// TODO decide return type
int8_t fast5_to_slow5(const char *pathname_from, struct slow5_file *s5p_to);
// Convert slow5 file to fast5 files
int8_t slow5_to_fast5(struct slow5_file *s5p_from, const char *pathname_to);


// Header
int8_t slow5_hdr_write(struct slow5_file *s5p_from, struct slow5_file *s5p_to);
int8_t slow5_hdr_data_write(struct slow5_file *s5p_from, struct slow5_file *s5p_to);
int8_t slow5_hdr_data_attr_write(const char *attr, struct slow5_file *s5p_from, struct slow5_file *s5p_to);

// Index
inline khash_t(s2i) *slow5_idx_init_empty(void);

// Convert fast5 dir/file to a slow5 file
// TODO decide return type
int8_t fast5dir_to_slow5(DIR *dirp_from, struct slow5_file *s5p_to);
int8_t fast5fp_to_slow5(fast5_t f5p_from, struct slow5_file *s5p_to);

// Convert slow5 file to fast5 dir/file
// TODO decide return type
int8_t slow5_to_fast5dir(struct slow5_file *s5p_from, DIR *dirp_to);
int8_t slow5_to_fast5fp(struct slow5_file *s5p_from, fast5_t *f5p_to);

// Merge 2 slow5 files to another slow5 file
int8_t slow5_merge_2(struct slow5_file *s5p_from_1, struct slow5_file *s5p_from_2, struct slow5_file *s5p_to);

// Split a slow5 file to a dir
int8_t slow5_split(struct slow5_file *s5p, DIR *dirp);


// Initiate an empty slow5 read object
struct slow5_read *slow5_read_init_empty(void);
// Initiate a slow5 read
struct slow5_read *slow5_read_init(void); // TODO change void


// Print out the SLOW5 structure contents
void slow5_hdr_data_print(const struct SLOW5Header *hdr);


// Get the format from a slow5 format name
enum slow5_format str_get_slow5_format(const char *str);
// Get the format of a slow5 pathname
enum slow5_format path_get_slow5_format(const char *pathname);
// Get the format of a slow5 FILE
enum slow5_format stream_get_slow5_format(const FILE *stream);

// Get the slow5 format name from the format
const char *slow5_format_get_str(enum slow5_format format);

// Get the slow5 version array from a version string
//const uint8_t *str_get_slow5_version(const char *str);
*/


#endif