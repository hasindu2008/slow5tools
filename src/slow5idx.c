#include <unistd.h>
#include <inttypes.h>
#include "klib/khash.h"
#include "slow5idx.h"
#include "slow5.h"
#include "slow5_extra.h"
#include "misc.h"
//TODO MALLOC_CHK for testing

#define BUF_INIT_CAP (20*1024*1024)
#define SLOW5_INDEX_BUF_INIT_CAP (64) // 2^6 TODO is this too little?

static inline struct slow5_idx *slow5_idx_init_empty(void);
static int slow5_idx_build(struct slow5_idx *index, struct slow5_file *s5p);
static void slow5_idx_read(struct slow5_idx *index);

static inline struct slow5_idx *slow5_idx_init_empty(void) {

    struct slow5_idx *index = (struct slow5_idx *) calloc(1, sizeof *index);
    MALLOC_CHK(index);
    index->hash = kh_init(s2i);

    return index;
}

// TODO return NULL if idx_init fails
struct slow5_idx *slow5_idx_init(struct slow5_file *s5p) {

    struct slow5_idx *index = slow5_idx_init_empty();
    index->pathname = get_slow5_idx_path(s5p->meta.pathname);

    if(index==NULL || index->pathname==NULL ){
        //TODO fix mem leak
        return NULL;
    }

    FILE *index_fp;

    // If file doesn't exist
    if ((index_fp = fopen(index->pathname, "rb")) == NULL) {
        if (slow5_idx_build(index, s5p) != 0) {
            slow5_idx_free(index);
            return NULL;
        }
    } else {
        index->fp = index_fp;
        slow5_idx_read(index);
    }

    return index;
}

/**
 * Create the index file for slow5 file.
 * Overrides if already exists.
 *
 * @param   s5p         slow5 file structure
 * @param   pathname    pathname to write index to
 * @return  -1 on error, 0 on success
 */
int slow5_idx_to(struct slow5_file *s5p, const char *pathname) {

    struct slow5_idx *index = slow5_idx_init_empty();
    if (slow5_idx_build(index, s5p) == -1) {
        slow5_idx_free(index);
        return -1;
    }

    index->fp = fopen(pathname, "wb");
    slow5_idx_write(index);

    slow5_idx_free(index);
    return 0;
}

static int slow5_idx_build(struct slow5_idx *index, struct slow5_file *s5p) {

    uint64_t curr_offset = ftello(s5p->fp);
    if (fseeko(s5p->fp, s5p->meta.start_rec_offset, SEEK_SET != 0)) {
        return -1;
    }

    uint64_t offset = 0;
    uint64_t size = 0;

    if (s5p->format == FORMAT_ASCII) {
        size_t cap = BUF_INIT_CAP;
        char *buf = (char *) malloc(cap * sizeof *buf);
        MALLOC_CHK(buf);
        ssize_t buf_len;
        char *bufp;

        offset = ftello(s5p->fp);
        while ((buf_len = getline(&buf, &cap, s5p->fp)) != -1) { // TODO this return is closer int64_t not unsigned
            bufp = buf;
            char *read_id = strdup(strsep_mine(&bufp, SEP_COL)); // TODO quicker to not split the whole line just the first delim
            size = buf_len;

            slow5_idx_insert(index, read_id, offset, size);
            offset += buf_len;
        }

        free(buf);

    } else if (s5p->format == FORMAT_BINARY) {
        const char eof[] = BINARY_EOF;
        char buf_eof[sizeof eof]; // TODO is this a vla?

        if (fread(buf_eof, sizeof *eof, sizeof eof, s5p->fp) != sizeof eof) {
            return -1;
        }
        while (memcmp(eof, buf_eof, sizeof *eof * sizeof eof) != 0) {
            if (fseek(s5p->fp, - sizeof *eof * sizeof eof, SEEK_CUR) != 0) { // Seek back
                return -1;
            }

            // Set start offset
            offset = ftello(s5p->fp);

            // Get record size
            slow5_rec_size_t record_size;
            if (fread(&record_size, sizeof record_size, 1, s5p->fp) != 1) {
                return -1;
            }

            size = sizeof record_size + record_size;

            uint8_t *read_comp = (uint8_t *) malloc(record_size);
            MALLOC_CHK(read_comp);
            if (fread(read_comp, record_size, 1, s5p->fp) != 1) {
                free(read_comp);
                return -1;
            }

            uint8_t *read_decomp = (uint8_t *) ptr_depress(s5p->compress, read_comp, record_size, NULL);
            if (read_decomp == NULL) {
                free(read_comp);
                free(read_decomp);
                return -1;
            }
            free(read_comp);

            // Get read id length
            uint64_t cur_len = 0;
            slow5_rid_len_t read_id_len;
            memcpy(&read_id_len, read_decomp + cur_len, sizeof read_id_len);
            cur_len += sizeof read_id_len;

            // Get read id
            char *read_id = (char *) malloc((read_id_len + 1) * sizeof *read_id); // +1 for '\0'
            MALLOC_CHK(read_id);
            memcpy(read_id, read_decomp + cur_len, read_id_len * sizeof *read_id);
            read_id[read_id_len] = '\0';

            // Insert index record
            slow5_idx_insert(index, read_id, offset, size);

            free(read_decomp);

            // Read in potential eof marker
            if (fread(buf_eof, sizeof *eof, sizeof eof, s5p->fp) != sizeof eof) {
                return -1;
            }
        }

        // Ensure actually at end of file
        if (fread(buf_eof, 1, 1, s5p->fp) != 0) {
            return -1;
        }
    }

    if (fseeko(s5p->fp, curr_offset, SEEK_SET != 0)) {
        return -1;
    }

    return 0;
}

void slow5_idx_write(struct slow5_idx *index) {

    //fprintf(index->fp, SLOW5_INDEX_HEADER);

    const char magic[] = INDEX_MAGIC_NUMBER;
    assert(fwrite(magic, sizeof *magic, sizeof magic, index->fp) == sizeof magic);

    struct slow5_version version = INDEX_VERSION;
    assert(fwrite(&version.major, sizeof version.major, 1, index->fp) == 1);
    assert(fwrite(&version.minor, sizeof version.minor, 1, index->fp) == 1);
    assert(fwrite(&version.patch, sizeof version.patch, 1, index->fp) == 1);

    uint8_t padding = INDEX_HEADER_SIZE_OFFSET -
            sizeof magic * sizeof *magic -
            sizeof version.major -
            sizeof version.minor -
            sizeof version.patch;
    uint8_t *zeroes = (uint8_t *) calloc(padding, sizeof *zeroes);
    assert(fwrite(zeroes, sizeof *zeroes, padding, index->fp) == padding);
    free(zeroes);

    for (uint64_t i = 0; i < index->num_ids; ++ i) {

        khint_t pos = kh_get(s2i, index->hash, index->ids[i]);
        assert(pos != kh_end(index->hash));

        struct slow5_rec_idx read_index = kh_value(index->hash, pos);

        /*
        assert(fprintf(index->fp, "%s" SEP_COL "%" PRIu64 SEP_COL "%" PRIu64 "\n",
                index->ids[i],
                read_index.offset,
                read_index.size) >= 0);
        */
        slow5_rid_len_t read_id_len = strlen(index->ids[i]);
        assert(fwrite(&read_id_len, sizeof read_id_len, 1, index->fp) == 1);
        assert(fwrite(index->ids[i], sizeof *index->ids[i], read_id_len, index->fp) == read_id_len);
        assert(fwrite(&read_index.offset, sizeof read_index.offset, 1, index->fp) == 1);
        assert(fwrite(&read_index.size, sizeof read_index.size, 1, index->fp) == 1);
    }

    const char eof[] = INDEX_EOF;
    assert(fwrite(eof, sizeof *eof, sizeof eof, index->fp) == sizeof eof);
}

static void slow5_idx_read(struct slow5_idx *index) {

    const char magic[] = INDEX_MAGIC_NUMBER;
    char buf_magic[sizeof magic]; // TODO is this a vla?
    assert(fread(buf_magic, sizeof *magic, sizeof magic, index->fp) == sizeof magic);
    assert(memcmp(magic, buf_magic, sizeof *magic * sizeof magic) == 0);

    assert(fread(&index->version.major, sizeof index->version.major, 1, index->fp) == 1);
    assert(fread(&index->version.minor, sizeof index->version.minor, 1, index->fp) == 1);
    assert(fread(&index->version.patch, sizeof index->version.patch, 1, index->fp) == 1);

    assert(fseek(index->fp, INDEX_HEADER_SIZE_OFFSET, SEEK_SET) != -1);

    const char eof[] = INDEX_EOF;
    char buf_eof[sizeof eof]; // TODO is this a vla?

    assert(fread(buf_eof, sizeof *eof, sizeof eof, index->fp) == sizeof eof);
    while (memcmp(eof, buf_eof, sizeof *eof * sizeof eof) != 0) {
        assert(fseek(index->fp, - sizeof *eof * sizeof eof, SEEK_CUR) == 0); // Seek back

        slow5_rid_len_t read_id_len;
        assert(fread(&read_id_len, sizeof read_id_len, 1, index->fp) == 1);
        char *read_id = (char *) malloc((read_id_len + 1) * sizeof *read_id); // +1 for '\0'
        NULL_CHK(read_id);
        assert(fread(read_id, sizeof *read_id, read_id_len, index->fp) == read_id_len);
        read_id[read_id_len] = '\0'; // Add null byte

        uint64_t offset;
        uint64_t size;

        assert(fread(&offset, sizeof offset, 1, index->fp) == 1);
        assert(fread(&size, sizeof size, 1, index->fp) == 1);

        slow5_idx_insert(index, read_id, offset, size);

        assert(fread(buf_eof, sizeof *eof, sizeof eof, index->fp) == sizeof eof);
    }
}

void slow5_idx_insert(struct slow5_idx *index, char *read_id, uint64_t offset, uint64_t size) {

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

// -1 if read_id not in the hash map
// 0 otherwise
int slow5_idx_get(struct slow5_idx *index, const char *read_id, struct slow5_rec_idx *read_index) {
    int ret = 0;

    khint_t pos = kh_get(s2i, index->hash, read_id);
    if (pos == kh_end(index->hash)) {
        ret = -1;
    } else {
        if (read_index != NULL) {
            *read_index = kh_value(index->hash, pos);
        }
    }

    return ret;
}

void slow5_idx_free(struct slow5_idx *index) {
    //NULL_CHK(index); // TODO necessary?

    if (index->fp != NULL) {
        assert(fclose(index->fp) == 0);
    }

    for (uint64_t i = 0; i < index->num_ids; ++ i) {
        free(index->ids[i]);
    }
    free(index->ids);

    kh_destroy(s2i, index->hash);

    free(index->pathname);
    free(index);
}
