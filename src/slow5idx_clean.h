#ifndef SLOW5IDX_CLEAN_H
#define SLOW5IDX_CLEAN_H

#include <stdio.h>
#include <stdint.h>
#include "klib/khash.h"
#include "slow5.h"

#define INDEX_EXTENSION "." "index"

// SLOW5 record index
struct slow5_rec_idx {
    uint64_t offset;
    uint64_t size;
};

// Read id map: read id -> index data
KHASH_MAP_INIT_STR(s2i, struct slow5_rec_idx)

// SLOW5 index
struct slow5_idx {
    FILE *fp;
    char **ids;
    uint64_t num_ids;
    uint64_t cap_ids;
    khash_t(s2i) *hash;
};

struct slow5_idx *slow5_idx_init(struct slow5_file *s5p, const char *index_pathname);
void slow5_idx_free(struct slow5_idx *index);
struct slow5_rec_idx slow5_idx_get(struct slow5_idx *index, const char *read_id);

#endif
