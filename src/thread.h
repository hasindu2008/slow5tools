#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <slow5/slow5.h>
#include "error.h"
#include <vector>
#include <string>

/**********************************
 * what you may have to modify *
 * - core_t struct
 * - db_t struct
 * - work_per_single_read function
 * - main function
 * compile as *
 * - gcc -Wall thread.c -lpthread
 **********************************/

#define WORK_STEAL 1 //simple work stealing enabled or not (no work stealing mean no load balancing)
#define STEAL_THRESH 1 //stealing threshold

#define NEG_CHK(ret) neg_chk(ret, __func__, __FILE__, __LINE__ - 1)

/* core data structure that has information that are global to all the threads */
typedef struct {
    int32_t num_thread;
    slow5_file_t *fp;
    slow5_fmt format_out;
    slow5_press_method_t press_method;
    //for view
    bool benchmark;
    //for merge
    int lossy;
    int slow5_file_index;
    slow5_aux_meta_t* aux_meta;
} core_t;

typedef struct{
    int len;
    void* buffer;
} raw_record_t;

/* data structure for a batch of reads*/
typedef struct {
    int64_t n_batch;    // number of records in this batch
    int64_t n_err;      // number of errors in this batch
    raw_record_t *read_record; // the list of read records (output) //change to whatever the data type
    //for get
    char **read_id;     // the list of read ids (input)
    //for view
    char** mem_records; // list of slow5_get_next_mem() records
    size_t* mem_bytes; // lengths of slow5_get_next_mem() records
    //for merge
    std::vector<std::string> slow5_files;
    std::vector<std::vector<size_t>> list;
    std::vector<int> slow5_file_indices;
    std::string output_dir;
    slow5_file_t **slow5_file_pointers;
    //for split
    uint32_t* read_group_vector;
} db_t;

/* argument wrapper for the multithreaded framework used for data processing */
typedef struct {
    core_t* core;
    db_t* db;
    int32_t starti;
    int32_t endi;
    void (*func)(core_t*,db_t*,int);
    int32_t thread_index;
#ifdef WORK_STEAL
    void *all_pthread_args;
#endif
} pthread_arg_t;


/*
int main(void) {

    core_t core;
    db_t db;

    core.num_thread = 4;
    db.n_batch = 1000;

    db.read_id = malloc(db.n_batch * sizeof *db.read_id);
    db.read_record = malloc(db.n_batch * sizeof *db.read_record);
    for (int i = 0; i < db.n_batch; ++ i) {
        db.read_id[i] = malloc(100 * sizeof **db.read_id);
        sprintf(db.read_id[i], "id%d", i);
    }

    work_db(&core, &db);

    for (int i = 0; i < db.n_batch; ++ i) {
        free(db.read_id[i]);
        free(db.read_record[i]);
    }
    free(db.read_id);
    free(db.read_record);

    return 0;
}
*/

void* pthread_single(void* voidargs);
void pthread_db(core_t* core, db_t* db, void (*func)(core_t*,db_t*,int));
void work_per_single_read(core_t* core,db_t* db, int32_t i);
/* process all reads in the given batch db */
void work_db(core_t* core, db_t* db, void (*func)(core_t*,db_t*,int));

#endif
