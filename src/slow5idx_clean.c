#include <unistd.h>
#include <inttypes.h>
#include "klib/khash.h"
#include "slow5idx_clean.h"
#include "slow5.h"
#include "misc.h"

#define BUF_INIT_CAP (20*1024*1024)
#define SLOW5_INDEX_BUF_INIT_CAP (64) // 2^6 TODO is this too little?

static inline struct slow5_idx *slow5_idx_init_empty(void);
static void slow5_idx_build(struct slow5_idx *index, struct slow5_file *s5p);
static void slow5_idx_write(struct slow5_idx *index);
static void slow5_idx_read(struct slow5_idx *index);
static void slow5_idx_insert(struct slow5_idx *index, char *read_id, uint64_t offset, uint64_t size);

static inline struct slow5_idx *slow5_idx_init_empty(void) {

    struct slow5_idx *index = calloc(1, sizeof *index);
    index->hash = kh_init(s2i);

    return index;
}

struct slow5_idx *slow5_idx_init(struct slow5_file *s5p, const char *index_pathname) {

    struct slow5_idx *index = slow5_idx_init_empty();

    FILE *index_fp;

    // If file doesn't exist
    if ((index_fp = fopen(index_pathname, "r")) == NULL) {
        index->fp = fopen(index_pathname, "w");
        slow5_idx_build(index, s5p);
        slow5_idx_write(index);
    } else {
        index->fp = index_fp;
        slow5_idx_read(index);
    }

    return index;
}

static void slow5_idx_build(struct slow5_idx *index, struct slow5_file *s5p) {

    size_t cap = BUF_INIT_CAP;
    char *buf = (char *) malloc(cap * sizeof *buf);
    ssize_t buf_len;

    uint64_t offset = 0;
    uint64_t size = 0;


    switch (s5p->format) {

        case FORMAT_ASCII: {

            while ((buf_len = getline(&buf, &cap, s5p->fp)) != -1) { // TODO this return is closer int64_t not unsigned
                offset = ftell(s5p->fp); // TODO returns long (much smaller than uint64_t)
                char *read_id = strdup(strtok_solo(buf, SEP)); // TODO quicker to just getdelim ? since don't want to split whole line
                size = buf_len;

                slow5_idx_insert(index, read_id, offset, size);
            }
        } break;

        case FORMAT_BINARY:
            break;
    }

    free(buf);
}

static void slow5_idx_write(struct slow5_idx *index) {

    fprintf(index->fp, SLOW5_INDEX_HEADER);

    for (uint64_t i = 0; i < index->num_ids; ++ i) {

        khint_t pos = kh_get(s2i, index->hash, index->ids[i]);
        assert(pos != kh_end(index->hash));

        struct slow5_rec_idx read_index = kh_value(index->hash, pos);

        assert(fprintf(index->fp, "%s" SEP "%" PRIu64 SEP "%" PRIu64 "\n",
                index->ids[i],
                read_index.offset,
                read_index.size) >= 0);
        // TODO snprintf + fputs to test
    }
}

static void slow5_idx_read(struct slow5_idx *index) {

    // Buffer for file parsing
    size_t cap = SLOW5_INDEX_BUF_INIT_CAP;
    char *buf = (char *) malloc(cap * sizeof *buf);
    MALLOC_CHK(buf);

    // Header
    assert(getline(&buf, &cap, index->fp) != -1);
    assert(strcmp(buf, SLOW5_INDEX_HEADER) == 0);

    // Index data
    ssize_t buf_len;
    while ((buf_len = getline(&buf, &cap, index->fp)) != -1) {
        buf[buf_len - 1] = '\0'; // Remove newline for later parsing

        char *read_id = strdup(strtok_solo(buf, SEP));

        char *offset_str = strtok_solo(NULL, SEP);
        uint64_t offset = ato_uint64(offset_str);

        char *size_str = strtok_solo(NULL, SEP);
        uint64_t size = ato_uint64(size_str);

        slow5_idx_insert(index, read_id, offset, size);
    }

    assert(cap == SLOW5_INDEX_BUF_INIT_CAP); // TESTING to see if getline has to realloc (if this fails often maybe put a larger buffer size)
    free(buf);
}

static void slow5_idx_insert(struct slow5_idx *index, char *read_id, uint64_t offset, uint64_t size) {

    int absent;
    khint_t k = kh_put(s2i, index->hash, read_id, &absent);
    assert(absent != -1);
    assert(absent != 0); // TODO error if read_id duplicated?

    struct slow5_rec_idx *read_index = &kh_value(index->hash, k);

    if (index->num_ids == index->cap_ids) {
        // Realloc ids array
        index->cap_ids = index->cap_ids ? index->cap_ids << 1 : 16; // TODO possibly integer overflow

        char **tmp = (char **) realloc(index->ids, index->cap_ids * sizeof *tmp);
        MALLOC_CHK(tmp);

        index->ids = tmp;
    }

    index->ids[index->num_ids ++] = read_id;

    read_index->offset = offset;
    read_index->size = size;
}

struct slow5_rec_idx slow5_idx_get(struct slow5_idx *index, const char *read_id) {
    // TODO null check?

    khint_t pos = kh_get(s2i, index->hash, read_id);
    assert(pos != kh_end(index->hash));

    return kh_value(index->hash, pos);
}

void slow5_idx_free(struct slow5_idx *index) {
    //NULL_CHK(index); // TODO necessary?

    assert(fclose(index->fp) == 0);

    for (uint64_t i = 0; i < index->num_ids; ++ i) {
        free(index->ids[i]);
    }
    free(index->ids);

    kh_destroy(s2i, index->hash);

    free(index);
}
