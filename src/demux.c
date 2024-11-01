/**
 * @file demux.c
 * @brief demultiplex a slow5 file
 * @author Sasha Jenner (me AT sjenner DOT com)
 * @date 01/11/2024
 */
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>
#include "demux.h"
#include "error.h"
#include "khash.h"
#include "kvec.h"
#include "slow5_extra.h"
#include "thread.h"

#define BARCODE_COUNT(m) (kh_size(m) + 2) // +2 for multi and missing
#define BARCODE_MISSING_INDEX(n) (n - 1)
#define BARCODE_MULTI_INDEX(n) (n - 2)
#define BARCODE_MISSING(d) ((d)->codes[BARCODE_MISSING_INDEX((d)->count)])
#define BARCODE_MULTI(d) ((d)->codes[BARCODE_MULTI_INDEX((d)->count)])

extern int slow5tools_verbosity_level;

struct kvec_u16 {
    uint16_t n;
    uint16_t m;
    uint16_t *a;
};
KHASH_MAP_INIT_STR(su16, uint16_t);
KHASH_MAP_INIT_STR(svu16, struct kvec_u16);

struct bsum {
    FILE *fp;
    char *line;            // Buffer for file parsing
    size_t n;              // Buffer size
    struct bsum_meta meta; // Metadata
    uint16_t code_pos;     // Barcode arrangement column number
    uint16_t rid_pos;      // Read ID column number
};

struct demux_info {
    char **codes;            // Barcode arrangements
    khash_t(svu16) *rid_map; // Hash map of read ID to barcode indices
    uint16_t count;          // Number of unique barcode arrangements
    uint32_t nofile;         // Number of open file descriptors
};

static char *path_append(const char *path, const char *suf);
static char *path_move(const char *path, const char *dir);
static char *path_move_append(const char *path, const char *dir,
                              const char *suf);
static char **getcodes(khash_t(su16) *code_map, struct bsum_meta *bs_meta,
                       uint16_t *n);
static char *path_spawn(const char *in_path, const char *name,
                        const opt_t *opt);
static core_t *demux_core_init(struct slow5_file *in, struct demux_info *d,
                               const opt_t *opt);
static db_t *demux_db_init(int n);
static inline void slow5_hdr_link(const struct slow5_hdr *in_hdr,
                                  struct slow5_hdr *out_hdr, int lossy);
static inline void slow5_hdr_unlink(struct slow5_hdr *hdr);
static int addcode(char **a, int i, const char *s);
static int bsum_close(struct bsum *bs);
static int bsum_getnext(struct bsum *bs, char **rid, char **code);
static int bsum_parsehdr(struct bsum *bs);
static int demux2(struct slow5_file *in, struct demux_info *d,
                  const opt_t *opt);
static int demux3(struct slow5_file *in, struct slow5_file **out,
                  struct demux_info *d, const opt_t *opt);
static int demux_db_setup(db_t *db, const struct slow5_file *in, int max);
static int demux_write(struct slow5_file **out, const db_t *db,
                       struct slow5_file *in, struct demux_info *d,
                       const opt_t *opt);
static int exist(char *const *a, int n, const char *s);
static int extmod(char *path, enum slow5_fmt fmt);
static int map_su16_getpush(khash_t(su16) *m, char *s, uint16_t *v);
static int map_su16_push(khash_t(su16) *m, char *s, khint_t *k);
static int setnofile(rlim_t n);
static int slow5_unbirth(struct slow5_file *s);
static int strdupadd(char **a, int i, const char *s);
static int update_db(core_t *core, db_t *db, struct slow5_rec *rec, int i);
static int update_db_missing(core_t *core, db_t *db, struct slow5_rec *rec,
                             int i);
static int update_db_missing_handle(core_t *core, db_t *db,
                                    struct slow5_rec *rec, int i);
static int update_maps(khash_t(su16) *code_map, khash_t(svu16) *rid_map,
                       char *code, char *rid);
static struct bsum *bsum_open(const struct bsum_meta *bs_meta);
static struct demux_info *demux_info_init(void);
static struct demux_info *demux_info_get(const struct bsum_meta *bs_meta);
static struct demux_info *demux_info_get2(struct bsum *bs);
static struct kvec_u16 *map_svu16_getpush(khash_t(svu16) *m, char *s);
static struct slow5_file *demux_spawn(const struct slow5_file *in,
                                      const char *name, uint32_t nofile,
                                      const opt_t *opt);
static struct slow5_file *slow5_birth(const struct slow5_file *in,
                                      const char *path, const opt_t *opt);
static struct slow5_file *slow5_spawn(const struct slow5_file *in,
                                      const char *name, const opt_t *opt);
static void demux_db_destroy(db_t *db);
static void demux_info_destroy(struct demux_info *d);
static void demux_setup(core_t *core, db_t *db, int i);
static void fillcodes(char **c, khash_t(su16) *code_map);
static void map_svu16_destroy(khash_t(svu16) *m);
static void underscore_prepend(const char *s, char **out, size_t *n);
static void update_db_multi(db_t *db, const struct demux_info *d, int i);
static void update_db_rec(core_t *core, db_t *db, struct slow5_rec *rec, int i);
static void vec_chkpush(struct kvec_u16 *v, uint16_t x);

/*
 * Demultiplex a slow5 file given the barcode summary file metadata and user
 * options. Return -1 on error, 0 on success.
 */
int demux(struct slow5_file *in, const struct bsum_meta *bs_meta,
          const opt_t *opt)
{
    int ret;
    struct demux_info *d;

    d = demux_info_get(bs_meta);
    if (!d)
        return -1;

    ret = demux2(in, d, opt);
    if (ret)
        return -1;

    demux_info_destroy(d);

    return 0;
}

/*
 * Append the suffix to the path before its extension.
 * Return a malloced string to be freed.
 */
static char *path_append(const char *path, const char *suf)
{
    const char *ext;
    char *ret;
    size_t len;
    size_t namelen;
    size_t pathlen;
    size_t suflen;

    pathlen = strlen(path);
    suflen = strlen(suf);

    ext = strrchr(path, PATH_EXT_DELIM);
    if (ext)
        namelen = ext - path;
    else
        namelen = pathlen;

    len = pathlen + suflen + 1;
    ret = (char *) malloc(len * sizeof (*ret));
    MALLOC_CHK(ret);

    (void) memcpy(ret, path, namelen);
    (void) memcpy(ret + namelen, suf, suflen);
    (void) memcpy(ret + namelen + suflen, ext, pathlen - namelen);
    ret[len - 1] = '\0';

    return ret;
}

/*
 * Move the path to the directory. Return a malloced string to be freed.
 */
static char *path_move(const char *path, const char *dir)
{
    const char *base;
    char *ret;
    size_t baselen;
    size_t dirlen;
    size_t len;

    dirlen = strlen(dir);
    if (dirlen)
        dirlen++;

    base = strrchr(path, PATH_DIR_DELIM);
    if (!base)
        base = path;
    else
        base++;
    baselen = strlen(base);

    len = dirlen + baselen + 1;
    ret = (char *) malloc(len * sizeof (*ret));
    MALLOC_CHK(ret);

    if (dirlen) {
        (void) memcpy(ret, dir, dirlen - 1);
        ret[dirlen - 1] = PATH_DIR_DELIM;
    }
    (void) memcpy(ret + dirlen, base, baselen);
    ret[len - 1] = '\0';

    return ret;
}

/*
 * Move the path to the directory and append the suffix before its extension.
 * Return a malloced string to be freed.
 */
static char *path_move_append(const char *path, const char *dir,
                              const char *suf)
{
    char *ret;
    char *tmp;

    if (dir) {
        tmp = path_move(path, dir);
        ret = path_append(tmp, suf);
        free(tmp);
    } else {
        ret = path_append(path, suf);
    }

    return ret;
}

/*
 * Get the array of barcode arrangements given the barcode arrangement to index
 * hash map and the barcode summary metadata. Set *n to size of the array.
 * Return NULL on error, the array to be freed on success.
 */
static char **getcodes(khash_t(su16) *code_map, struct bsum_meta *bs_meta,
                       uint16_t *n)
{
    char **c;
    const uint16_t m = BARCODE_COUNT(code_map);
    int ret;

    if (m > UINT16_MAX) {
        ERROR("Too many categories (%d)", m);
        return NULL;
    }

    c = (char **) calloc(m, sizeof (*c));
    MALLOC_CHK(c);

    fillcodes(c, code_map);

    ret = addcode(c, BARCODE_MULTI_INDEX(m), bs_meta->multi);
    if (ret) {
        if (ret == 1) {
            ERROR("Multi-category '%s' already exists in demux TSV",
                  bs_meta->multi);
        }
        return NULL;
    }

    ret = addcode(c, BARCODE_MISSING_INDEX(m), bs_meta->missing);
    if (ret) {
        if (ret == 1) {
            ERROR("Uncategorised reads category '%s' already exists",
                  bs_meta->missing);
        }
        return NULL;
    }

    *n = m;
    return c;
}

/*
 * Get a path appended with an underscore and name before its extension.
 * Return NULL on error.
 */
static char *path_spawn(const char *in_path, const char *name,
                        const opt_t *opt)
{
    char *path;
    char *suf;
    int ret;
    size_t len;

    suf = NULL;
    underscore_prepend(name, &suf, &len);
    path = path_move_append(in_path, opt->arg_dir_out, suf);
    ret = extmod(path, opt->fmt_out);
    if (ret)
        return NULL;
    free(suf);

    return path;
}

/*
 * Initialise the demultiplexing multi-threading core.
 */
static core_t *demux_core_init(struct slow5_file *in, struct demux_info *d,
                               const opt_t *opt)
{
    core_t *c;

    c = (core_t *) calloc(1, sizeof (*c));
    MALLOC_CHK(c);

    if (!opt->flag_lossy)
        c->aux_meta = in->header->aux_meta;
    c->format_out = opt->fmt_out;
    c->fp = in;
    c->lossy = opt->flag_lossy;
    c->num_thread = opt->num_threads;
    c->param = (void *) d;
    c->press_method.record_method = opt->record_press_out;
    c->press_method.signal_method = opt->signal_press_out;

    return c;
}

/*
 * Initialise the demultiplexing multi-threading database for n records.
 */
static db_t *demux_db_init(int n)
{
    db_t *db;

    db = (db_t *) calloc(1, sizeof (*db));
    MALLOC_CHK(db);

    db->mem_bytes = (size_t *) malloc(n * sizeof (*db->mem_bytes));
    db->mem_records = (char **) malloc(n * sizeof (*db->mem_records));
    db->read_record = (raw_record_t *) malloc(n * sizeof (*db->read_record));
    db->read_group_vector = (uint32_t *) malloc(n * sizeof (struct kvec_u16));

    MALLOC_CHK(db->mem_bytes);
    MALLOC_CHK(db->mem_records);
    MALLOC_CHK(db->read_record);
    MALLOC_CHK(db->read_group_vector);

    return db;
}

/*
 * If s does not already exist, add the duplicate of s to a at index i.
 * Return -1 on error, 0 on success, 1 if already exists.
 */
static int addcode(char **a, int i, const char *s)
{
    if (!s)
        return 0;
    if (exist(a, i, s))
        return 1;
    if (strdupadd(a, i, s))
        return -1;
    return 0;
}

/*
 * Close the barcode summary file and free the structure.
 * Return -1 on error, 0 on success.
 */
static int bsum_close(struct bsum *bs)
{
    int ret;

    ret = fclose(bs->fp);
    if (ret == EOF) {
        ERROR("Failed to close demux TSV: %s", strerror(errno));
        return -1;
    }
    free(bs->line);
    free(bs);

    return 0;
}

/*
 * Get the next read ID and barcode arrangement from the barcode summary file.
 * *rid and *code are malloced pointers to be freed by the caller.
 * Return -1 on error, 0 on success, 1 on end of file.
 */
static int bsum_getnext(struct bsum *bs, char **rid, char **code)
{
    char *tok;
    ssize_t nread;
    uint16_t i;

    nread = getline(&bs->line, &bs->n, bs->fp);
    if (nread == -1) {
        if (feof(bs->fp))
            return 1;
        ERROR("Failed to read demux TSV line: %s", strerror(errno));
        return -1;
    }

    if (bs->line[nread - 1] == '\n')
        bs->line[nread - 1] = '\0';

    i = 1;
    tok = strtok(bs->line, BSUM_DELIM);
    while (tok && (i <= bs->code_pos || i <= bs->rid_pos)) {
        if (i == bs->rid_pos) {
            *rid = strdup(tok);
            if (!*rid) {
                perror("strdup");
                return -1;
            }
        } else if (i == bs->code_pos) {
            *code = strdup(tok);
            if (!*code) {
                perror("strdup");
                return -1;
            }
        }
        tok = strtok(NULL, BSUM_DELIM);
        i++;
    }

    return 0;
}

/*
 * Parse the barcode summary header. Return -1 on error, 0 on success.
 */
static int bsum_parsehdr(struct bsum *bs)
{
    char *tok;
    int ret;
    uint16_t i;
    ssize_t nread;

    nread = getline(&bs->line, &bs->n, bs->fp);
    if (nread == -1) {
        ERROR("Failed to read demux TSV header: %s", strerror(errno));
        return -1;
    }

    if (bs->line[nread - 1] == '\n')
        bs->line[nread - 1] = '\0';
    i = 1;
    bs->code_pos = 0;
    bs->rid_pos = 0;

    tok = strtok(bs->line, BSUM_DELIM);
    while (tok && BSUM_HEADER_MISSING(bs)) {
        if (!bs->rid_pos) {
            ret = strcmp(tok, bs->meta.rid_hdr);
            if (!ret)
                bs->rid_pos = i;
        } else if (!bs->code_pos) {
            ret = strcmp(tok, bs->meta.code_hdr);
            if (!ret)
                bs->code_pos = i;
        }
        tok = strtok(NULL, BSUM_DELIM);
        i++;
    }
    if (!bs->rid_pos) {
        ERROR("Invalid demux TSV header: missing '%s'", bs->meta.rid_hdr);
        return -1;
    } else if (!bs->code_pos) {
        ERROR("Invalid demux TSV header: missing '%s'", bs->meta.code_hdr);
        return -1;
    }

    return 0;
}

/*
 * Demultiplex a slow5 file given the demultiplexing information and user
 * options. Return -1 on error, 0 on success.
 */
static int demux2(struct slow5_file *in, struct demux_info *d, const opt_t *opt)
{
    int ret;
    struct slow5_file **out;
    uint16_t i;

    out = (struct slow5_file **) calloc(d->count, sizeof (*out));
    MALLOC_CHK(out);

    ret = demux3(in, out, d, opt);
    if (ret)
        return -1;

    for (i = 0; i < d->count; i++) {
        ret = slow5_unbirth(out[i]);
        if (ret)
            return -1;
    }
    free(out);

    return 0;
}

/*
 * Demultiplex a slow5 file given the output files, demultiplexing information
 * and user options. Return -1 on error, 0 on success.
 */
static int demux3(struct slow5_file *in, struct slow5_file **out,
                  struct demux_info *d, const opt_t *opt)
{
    core_t *core;
    db_t *db;
    int iseof;
    int ret;
    khint_t n;

    core = demux_core_init(in, d, opt);
    db = demux_db_init(opt->read_id_batch_capacity);

    iseof = 0;
    n = 0;
    while (!iseof) {
        ret = demux_db_setup(db, in, opt->read_id_batch_capacity);
        if (ret == 1)
            iseof = 1;
        else if (ret == -1)
            return -1;

        work_db(core, db, demux_setup);
        n += db->n_batch;

        ret = demux_write(out, db, in, d, opt);
        if (ret)
            return -1;
    }

    if (n < kh_size(d->rid_map)) {
        ERROR("Extra read(s) in demux TSV%s", "");
        return -1;
    }

    free(core);
    demux_db_destroy(db);

    return 0;
}

/*
 * Store the next max records in the demultiplexing multi-threading database.
 * Return -1 on error, 0 on success, 1 on end of file.
 */
static int demux_db_setup(db_t *db, const struct slow5_file *in, int max)
{
    char *mem;
    int n;
    int ret;
    size_t len;

    n = 0;
    ret = 0;
    while (!ret && n < max) {
        mem = (char *) slow5_get_next_mem(&len, in);
        if (!mem) {
            if (slow5_errno == SLOW5_ERR_EOF)
                ret = 1;
            else
                ret = -1;
        } else {
            db->mem_records[n] = mem;
            db->mem_bytes[n] = len;
            n++;
        }
    }
    db->n_batch = (int64_t) n;

    return ret;
}

/*
 * Write the demultiplexing multi-threading database records to their
 * corresponding barcode files given the input slow5 file.
 * Return -1 on error, 0 on success.
 */
static int demux_write(struct slow5_file **out, const db_t *db,
                       struct slow5_file *in, struct demux_info *d,
                       const opt_t *opt)
{
    const struct kvec_u16 *rec_codes;
    int i;
    size_t len;
    uint16_t j;
    uint16_t k;

    rec_codes = (const struct kvec_u16 *) db->read_group_vector;

    for (i = 0; i < (int) db->n_batch; i++) {
        for (j = 0; j < kv_size(rec_codes[i]); j++) {
            k = kv_A(rec_codes[i], j);

            if (!out[k]) {
                out[k] = demux_spawn(in, d->codes[k], d->nofile, opt);
                if (!out[k])
                    return -1;
                d->nofile++;
            }

            len = fwrite(db->read_record[i].buffer, 1, db->read_record[i].len,
                         out[k]->fp);
            if (len != (size_t) db->read_record[i].len) {
                ERROR("%s", "Failed to write slow5 record");
                return -1;
            }
        }
        free(db->read_record[i].buffer);
    }
    return 0;
}

/*
 * Does string s exist in array a of length n?
 * Return 1 if exists, 0 if does not exist.
 */
static int exist(char *const *a, int n, const char *s)
{
    int i;

    for (i = 0; i < n; i++) {
        if (a[i] && !strcmp(a[i], s))
            return 1;
    }
    return 0;
}

/*
 * Write the format's slow5 extension to the path with capacity cap.
 * Return -1 on error, 0 on success.
 */
static int extmod(char *path, enum slow5_fmt fmt)
{
    char *ext;
    const char *name;
    size_t namelen;

    ext = strrchr(path, PATH_EXT_DELIM);
    if (!ext) {
        ERROR("Path '%s' has no extension", path);
        return -1;
    }
    ext++;

    name = slow5_fmt_get_name(fmt);
    if (!name) {
        ERROR("Unknown slow5 format %d", fmt);
        return -1;
    }

    namelen = strlen(name);
    if (ext - path + namelen > strlen(path)) {
        ERROR("Buffer too small to modify extension to '%s'", name);
        return -1;
    }

    (void) memcpy(ext, name, namelen + 1);
    return 0;
}

/*
 * Get the value of map m at key s. Set *v to the value. If s does not exist,
 * add it to m with the number of elements - 1 as its value.
 * Return -1 on error, 0 on key already exists, 1 on key was added.
 */
static int map_su16_getpush(khash_t(su16) *m, char *s, uint16_t *v)
{
    int err;
    int ret;
    khint_t k;

    ret = 0;
    k = kh_get(su16, m, s);

    if (k == kh_end(m)) {
        err = map_su16_push(m, s, &k);
        if (err)
            return -1;
        ret = 1;
    }

    *v = kh_val(m, k);
    return ret;
}

/*
 * Add s to m with the number of elements - 1 as its value.
 * Return -1 on error, 0 on success.
 */
static int map_su16_push(khash_t(su16) *m, char *s, khint_t *k)
{
    int err;

    *k = kh_put(su16, m, s, &err);
    if (err == -1) {
        ERROR("Failed to put '%s' into hash map", s);
        return -1;
    }

    kh_val(m, *k) = kh_size(m) - 1;
    return 0;
}

/*
 * Set the maximum file descriptor number for the current process. If n is less
 * than or equal to the current maxima, do not update it.
 * Return -1 on error, 0 on success.
 */
static int setnofile(rlim_t n)
{
    int ret;
    struct rlimit rlim;

    (void) memset(&rlim, 0, sizeof rlim);
    ret = getrlimit(RLIMIT_NOFILE, &rlim);
    if (ret) {
        ERROR("Failed to get maximum number of open files: %s",
              strerror(errno));
        return -1;
    }

    if (n <= rlim.rlim_cur)
        return 0;

    rlim.rlim_cur = n;
    if (n > rlim.rlim_max)
        rlim.rlim_max = n;

    ret = setrlimit(RLIMIT_NOFILE, &rlim);
    if (ret) {
        ERROR("Failed to set maximum number of open files to %ld: %s", n,
              strerror(errno));
        return -1;
    }

    return 0;
}

/*
 * Write the binary eof, unlink the header and close the slow5 file.
 * Return -1 on error, 0 on success.
 */
static int slow5_unbirth(struct slow5_file *s)
{
    int ret;
    ssize_t n;

    if (!s)
        return 0;

    if (s->format == SLOW5_FORMAT_BINARY) {
        n = slow5_eof_fwrite(s->fp);
        if (n == SLOW5_ERR_IO)
            return -1;
    }
    slow5_hdr_unlink(s->header);
    ret = slow5_close(s);
    if (ret)
        return -1;
    return 0;
}

/*
 * Add the duplicate of s to a at index i.
 * Return -1 on error, 0 on success.
 */
static int strdupadd(char **a, int i, const char *s)
{
    char *d;

    d = strdup(s);
    if (!d) {
        perror("strdup");
        return -1;
    }
    a[i] = d;
    return 0;
}

/*
 * Shallow copy a slow5 header. If lossy, do not copy the auxiliary metadata.
 */
static inline void slow5_hdr_link(const struct slow5_hdr *in_hdr,
                                  struct slow5_hdr *out_hdr, int lossy)
{
    out_hdr->version = in_hdr->version;
    out_hdr->num_read_groups = in_hdr->num_read_groups;
    out_hdr->data = in_hdr->data;
    if (!lossy)
        out_hdr->aux_meta = in_hdr->aux_meta;
}

/*
 * Un-shallow copy a slow5 header.
 */
static inline void slow5_hdr_unlink(struct slow5_hdr *hdr)
{
    (void) memset(&(hdr->data), 0, sizeof (hdr->data));
    hdr->aux_meta = NULL;
}

/*
 * Wrapper around update_db_rec which checks if the record is missing or if the
 * multi-category should be used. Return -1 on error, 0 on success.
 */
static int update_db(core_t *core, db_t *db, struct slow5_rec *rec, int i)
{
    const struct demux_info *d;
    khint_t k;
    struct kvec_u16 *rec_codes;

    d = (const struct demux_info *) core->param;
    rec_codes = (struct kvec_u16 *) db->read_group_vector;

    k = kh_get(svu16, d->rid_map, rec->read_id);
    if (k == kh_end(d->rid_map)) {
        return update_db_missing(core, db, rec, i);
    } else {
        rec_codes[i] = kh_val(d->rid_map, k);
        if (BARCODE_MULTI(d) && kv_size(rec_codes[i]) > 1)
            update_db_multi(db, d, i);
        update_db_rec(core, db, rec, i);
    }
    return 0;
}

/*
 * If the missing category is unset, zero the output buffer at index i.
 * Otherwise, handle the missing record. Return -1 on error, 0 on success.
 */
static int update_db_missing(core_t *core, db_t *db, struct slow5_rec *rec,
                             int i)
{
    const struct demux_info *d;
    struct kvec_u16 *rec_codes;

    d = (const struct demux_info *) core->param;
    rec_codes = (struct kvec_u16 *) db->read_group_vector;

    if (!BARCODE_MISSING(d)) {
        WARNING("Read ID '%s' is missing from demux TSV", rec->read_id);
        (void) memset(rec_codes + i, 0, sizeof (*rec_codes));
        (void) memset(db->read_record + i, 0, sizeof (*db->read_record));
    } else {
        return update_db_missing_handle(core, db, rec, i);
    }
    return 0;
}

/*
 * Add a new barcode indices array storing the missing category to the read ID
 * hash map. Update the database at index i.
 * Return -1 on error, 0 on success.
 */
static int update_db_missing_handle(core_t *core, db_t *db,
                                    struct slow5_rec *rec, int i)
{
    char *rid;
    const struct demux_info *d;
    struct kvec_u16 *rec_codes;
    struct kvec_u16 *v;

    d = (const struct demux_info *) core->param;
    rec_codes = (struct kvec_u16 *) db->read_group_vector;

    rid = strdup(rec->read_id);
    if (!rid) {
        perror("strdup");
        return -1;
    }

    v = map_svu16_getpush(d->rid_map, rid);
    if (!v)
        return -1;
    kv_push(uint16_t, *v, BARCODE_MISSING_INDEX(d->count));
    rec_codes[i] = *v;

    update_db_rec(core, db, rec, i);
    return 0;
}

/*
 * Update the barcode arrangement to index and read ID to barcode indices hash
 * maps given a read ID and one of its barcode arrangements.
 * Return -1 on error, 0 on success.
 */
static int update_maps(khash_t(su16) *code_map, khash_t(svu16) *rid_map,
                       char *code, char *rid)
{
    int ret;
    struct kvec_u16 *rid_codes;
    uint16_t i;

    rid_codes = map_svu16_getpush(rid_map, rid);
    if (!rid_codes)
        return -1;
    if (kv_size(*rid_codes))
        free(rid);

    ret = map_su16_getpush(code_map, code, &i);
    if (!ret)
        free(code);
    else if (ret == -1)
        return -1;

    vec_chkpush(rid_codes, i);

    return 0;
}

/*
 * Open a barcode summary file and parse the header. Return NULL on error.
 */
static struct bsum *bsum_open(const struct bsum_meta *bs_meta)
{
    int ret;
    struct bsum *bs;

    bs = (struct bsum *) malloc(sizeof (*bs));
    MALLOC_CHK(bs);

    bs->fp = fopen(bs_meta->path, "r");
    if (!bs->fp) {
        ERROR("Failed to open '%s': %s", bs_meta->path, strerror(errno));
        return NULL;
    }

    bs->line = NULL;
    bs->n = 0;
    bs->meta = *bs_meta;

    ret = bsum_parsehdr(bs);
    if (ret)
        return NULL;

    return bs;
}

/*
 * Initialise the demultiplexing information.
 */
static struct demux_info *demux_info_init(void)
{
    struct demux_info *d;

    d = (struct demux_info *) malloc(sizeof (*d));
    MALLOC_CHK(d);
    d->rid_map = kh_init(svu16);
    MALLOC_CHK(d->rid_map);
    d->nofile = SLOW5_SPAWN_NOFILE;

    return d;
}

/*
 * Get the demultiplexing information given the barcode summary file metadata.
 * Return NULL on error.
 */
static struct demux_info *demux_info_get(const struct bsum_meta *bs_meta)
{
    int ret;
    struct bsum *bs;
    struct demux_info *d;

    bs = bsum_open(bs_meta);
    if (!bs)
        return NULL;

    d = demux_info_get2(bs);
    if (!d)
        return NULL;

    ret = bsum_close(bs);
    if (ret)
        return NULL;

    return d;
}

/*
 * Get the demultiplexing information given the barcode summary file.
 * Create a temporary hash map from barcode arrangement to index for querying.
 * Return NULL on error.
 */
static struct demux_info *demux_info_get2(struct bsum *bs)
{
    char *code;
    char *rid;
    int ret;
    khash_t(su16) *code_map;
    struct demux_info *d;

    d = demux_info_init();

    code_map = kh_init(su16);
    MALLOC_CHK(code_map);

    ret = bsum_getnext(bs, &rid, &code);
    while (!ret) {
        ret = update_maps(code_map, d->rid_map, code, rid);
        if (ret)
            return NULL;
        ret = bsum_getnext(bs, &rid, &code);
    }
    if (ret == -1)
        return NULL;

    d->codes = getcodes(code_map, &(bs->meta), &(d->count));
    if (!d->codes)
        return NULL;

    kh_destroy(su16, code_map);

    return d;
}

/*
 * Get the value of map m at key s. If s does not exist, add it to m and
 * initialise its value to a new vector. Return NULL on error.
 */
static struct kvec_u16 *map_svu16_getpush(khash_t(svu16) *m, char *s)
{
    int ret;
    khint_t k;

    k = kh_get(svu16, m, s);

    if (k == kh_end(m)) {
        k = kh_put(svu16, m, s, &ret);
        if (ret == -1) {
            ERROR("Failed to put '%s' into hash map", s);
            return NULL;
        }
        kv_init(kh_val(m, k));
    }

    return &kh_val(m, k);
}

/*
 * Increment the maximum file descriptor number then shallow copy a slow5 file.
 * Return NULL on error.
 */
static struct slow5_file *demux_spawn(const struct slow5_file *in,
                                      const char *name, uint32_t nofile,
                                      const opt_t *opt)
{
    int ret;

    ret = setnofile((rlim_t) nofile + 1);
    if (ret)
        return NULL;

    return slow5_spawn(in, name, opt);
}

/*
 * Shallow copy the skeleton of a slow5 file to a given path. The header is
 * shallow copied, but otherwise no records are copied over. Output options are
 * used to specify the output format, lossy-ness and compression used.
 * Return NULL on error.
 */
static struct slow5_file *slow5_birth(const struct slow5_file *in,
                                      const char *path, const opt_t *opt)
{
    FILE *fp;
    int ret;
    slow5_press_method_t press_out;
    struct slow5_file *out;

    fp = fopen(path, "w");
    if (!fp) {
        ERROR("Failed to open '%s' for writing: %s", path, strerror(errno));
        return NULL;
    }

    out = slow5_init_empty(fp, path, opt->fmt_out);
    if (!out)
        return NULL;

    slow5_hdr_link(in->header, out->header, opt->flag_lossy);

    press_out.record_method = opt->record_press_out;
    press_out.signal_method = opt->signal_press_out;
    ret = slow5_hdr_fwrite(out->fp, out->header, opt->fmt_out, press_out);
    if (ret == -1) {
        ERROR("Failed to write the header to '%s'", path);
        return NULL;
    }

    return out;
}

/*
 * Shallow copy the skeleton of a slow5 file to a new path appended with an
 * underscore and name before its extension. Return NULL on error.
 */
static struct slow5_file *slow5_spawn(const struct slow5_file *in,
                                      const char *name, const opt_t *opt)
{
    char *path;
    struct slow5_file *out;

    path = path_spawn(in->meta.pathname, name, opt);
    if (!path)
        return NULL;

    out = slow5_birth(in, path, opt);
    free(path);
    if (!out)
        return NULL;

    return out;
}

/*
 * Free the demultiplexing multi-threading database.
 */
static void demux_db_destroy(db_t *db)
{
    free(db->mem_bytes);
    free(db->mem_records);
    free(db->read_record);
    free(db->read_group_vector);
    free(db);
}

/*
 * Free the demultiplexing information and all its data.
 */
static void demux_info_destroy(struct demux_info *d)
{
    uint16_t i;

    for (i = 0; i < d->count; i++)
        free(d->codes[i]);
    free(d->codes);

    map_svu16_destroy(d->rid_map);
    free(d);
}

/*
 * Decompress, parse and convert the record at index i to the desired output
 * format.
 */
static void demux_setup(core_t *core, db_t *db, int i)
{
    int ret;
    struct slow5_rec *rec;

    rec = NULL;
    ret = slow5_rec_depress_parse(db->mem_records + i, db->mem_bytes + i, NULL,
                                  &rec, core->fp);
    free(db->mem_records[i]);
    if (ret)
        exit(EXIT_FAILURE);

    ret = update_db(core, db, rec, i);
    if (ret)
        exit(EXIT_FAILURE);
    slow5_rec_free(rec);
}

/*
 * Fill the array of barcode arrangements given the barcode arrangement to index
 * hash map.
 */
static void fillcodes(char **c, khash_t(su16) *code_map)
{
    khint_t k;
    uint16_t i;

    for (k = kh_begin(code_map); k != kh_end(code_map); k++) {
        if (kh_exist(code_map, k)) {
            i = kh_val(code_map, k);
            c[i] = (char *) kh_key(code_map, k);
        }
    }
}

/*
 * Free the khash_t(svu16) hash map, all its keys and values.
 */
static void map_svu16_destroy(khash_t(svu16) *m)
{
    khint_t k;

    for (k = kh_begin(m); k != kh_end(m); k++) {
        if (kh_exist(m, k)) {
            free((char *) kh_key(m, k));
            kv_destroy(kh_val(m, k));
        }
    }

    kh_destroy(svu16, m);
}

/*
 * Prepend s with an underscore and write it to *out. If *out is NULL or *n is
 * too small, reallocate memory for *out and update *n to its new size.
 */
static void underscore_prepend(const char *s, char **out, size_t *n)
{
    size_t len;

    len = strlen(s);
    if (!*out || len + 2 > *n) {
        *n = len + 2;
        *out = (char *) realloc(*out, *n * sizeof (**out));
        MALLOC_CHK(*out);
    }

    *out[0] = '_';
    (void) memcpy(*out + 1, s, len + 1);
}

/*
 * Update the database's barcode indices array at index i to hold only the
 * multi-category.
 */
static void update_db_multi(db_t *db, const struct demux_info *d, int i)
{
    struct kvec_u16 *rec_codes;

    rec_codes = (struct kvec_u16 *) db->read_group_vector;
    kv_size(rec_codes[i]) = 1;
    kv_A(rec_codes[i], 0) = BARCODE_MULTI_INDEX(d->count);
}

/*
 * Convert a slow5 record to its desired output format given the core metadata.
 * Update the database's output buffer at index i.
 */
static void update_db_rec(core_t *core, db_t *db, struct slow5_rec *rec, int i)
{
    size_t len;
    struct slow5_press *press;

    press = slow5_press_init(core->press_method);
    if (!press)
        exit(EXIT_FAILURE);

    db->read_record[i].buffer = slow5_rec_to_mem(rec, core->aux_meta,
                                                 core->format_out, press,
                                                 &len);
    if (!db->read_record[i].buffer)
        exit(EXIT_FAILURE);
    db->read_record[i].len = (int) len; // TODO should be size_t or uint32_t
    slow5_press_free(press);
}

/*
 * If x is not in vector v, append it.
 */
static void vec_chkpush(struct kvec_u16 *v, uint16_t x)
{
    int found;
    uint16_t i;

    i = 0;
    found = 0;
    while (!found && i < kv_size(*v)) {
        if (kv_A(*v, i) == x)
            found = 1;
        i++;
    }
    if (!found)
        kv_push(uint16_t, *v, x);
}
