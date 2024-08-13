/**
 * @file demux.c
 * @brief demultiplex a slow5 file
 * @author Sasha Jenner (me AT sjenner DOT com)
 * @date 04/07/2024
 */
#include <stdint.h>
#include "demux.h"
#include "error.h"
#include "khash.h"
#include "kvec.h"
#include "slow5_extra.h"
#include "thread.h"

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
};

static char *path_append(const char *path, const char *suf);
static char *path_move(const char *path, const char *dir);
static char *path_move_append(const char *path, const char *dir,
                              const char *suf);
static char **paths_spawn(const char *in_path, const char **names,
                          uint16_t count, const opt_t *opt);
static core_t *demux_core_init(struct slow5_file *in,
                               struct slow5_aux_meta *aux_meta,
                               khash_t(svu16) *rid_map, const opt_t *opt);
static db_t *demux_db_init(int n);
static int bsum_close(struct bsum *bs);
static int bsum_getnext(struct bsum *bs, char **rid, char **code);
static int bsum_parsehdr(struct bsum *bs);
static int demux2(struct slow5_file *in, const struct demux_info *d,
                  const opt_t *opt);
static int demux3(struct slow5_file *in, struct slow5_file **out,
                  const struct demux_info *d, const opt_t *opt);
static int demux_db_setup(db_t *db, const struct slow5_file *in, int max);
static int demux_write(struct slow5_file **out, const db_t *db,
                       const struct kvec_u16 *rec_codes);
static int extmod(char *path, enum slow5_fmt fmt);
static int map_su16_getpush(khash_t(su16) *m, char *s, uint16_t *v);
static int slow5_aux_meta_copy(const struct slow5_hdr *in_hdr,
                               struct slow5_hdr *out_hdr);
static int slow5_aux_meta_copy_enum(const struct slow5_hdr *in_hdr,
                                    struct slow5_hdr *out_hdr, const char *attr,
                                    enum slow5_aux_type type);
static int slow5_hdr_copy(const struct slow5_hdr *in_hdr,
                          struct slow5_hdr *out_hdr, int lossy);
static struct bsum *bsum_open(const struct bsum_meta *bs_meta);
static struct demux_info *demux_info_init(void);
static struct demux_info *demux_info_get(const struct bsum_meta *bs_meta);
static struct demux_info *demux_info_get2(struct bsum *bs);
static struct kvec_u16 *map_svu16_getpush(khash_t(svu16) *m, char *s);
static struct slow5_file *slow5_birth(const struct slow5_file *in,
                                      const char *path, const opt_t *opt);
static struct slow5_file **slow5_spawn(const struct slow5_file *in,
                                       const char **names, uint16_t count,
                                       const opt_t *opt);
static void demux_db_destroy(db_t *db);
static void demux_info_destroy(struct demux_info *d);
static void demux_setup(core_t *core, db_t *db, int i);
static void map_svu16_destroy(khash_t(svu16) *m);
static void underscore_prepend(const char *s, char **out, size_t *n);
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
 * Get count new paths appended with an underscore and name before its
 * extension. Return NULL on error.
 */
static char **paths_spawn(const char *in_path, const char **names,
                          uint16_t count, const opt_t *opt)
{
    char **paths;
    char *suf;
    int ret;
    size_t len;
    uint16_t i;

    paths = (char **) malloc(count * sizeof (*paths));
    MALLOC_CHK(paths);

    suf = NULL;
    for (i = 0; i < count; i++) {
        underscore_prepend(names[i], &suf, &len);
        paths[i] = path_move_append(in_path, opt->arg_dir_out, suf);
        ret = extmod(paths[i], opt->fmt_out);
        if (ret)
            return NULL;
    }
    free(suf);

    return paths;
}

/*
 * Initialise the demultiplexing multi-threading core.
 */
static core_t *demux_core_init(struct slow5_file *in,
                               struct slow5_aux_meta *aux_meta,
                               khash_t(svu16) *rid_map, const opt_t *opt)
{
    core_t *c;

    c = (core_t *) calloc(1, sizeof (*c));
    MALLOC_CHK(c);

    if (!opt->flag_lossy)
        c->aux_meta = aux_meta;
    c->format_out = opt->fmt_out;
    c->fp = in;
    c->lossy = opt->flag_lossy;
    c->num_thread = opt->num_threads;
    c->param = (void *) rid_map;
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
 * Close the barcode summary file and free the structure.
 * Return -1 on error, 0 on success.
 */
static int bsum_close(struct bsum *bs)
{
    int ret;

    ret = fclose(bs->fp);
    if (ret == EOF) {
        ERROR("Failed to close custom TSV: %s", strerror(errno));
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
        ERROR("Failed to read custom TSV line: %s", strerror(errno));
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
        ERROR("Failed to read custom TSV header: %s", strerror(errno));
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
        ERROR("Invalid custom TSV header: missing '%s'", bs->meta.rid_hdr);
        return -1;
    } else if (!bs->code_pos) {
        ERROR("Invalid custom TSV header: missing '%s'", bs->meta.code_hdr);
        return -1;
    }

    return 0;
}

/*
 * Demultiplex a slow5 file given the demultiplexing information and user
 * options. Return -1 on error, 0 on success.
 */
static int demux2(struct slow5_file *in, const struct demux_info *d,
                  const opt_t *opt)
{
    int ret;
    ssize_t n;
    struct slow5_file **out;
    uint16_t i;

    out = slow5_spawn(in, (const char **) d->codes, d->count, opt);
    if (!out)
        return -1;

    ret = demux3(in, out, d, opt);
    if (ret)
        return -1;

    for (i = 0; i < d->count; i++) {
        if (out[i]->format == SLOW5_FORMAT_BINARY) {
            n = slow5_eof_fwrite(out[i]->fp);
            if (n == SLOW5_ERR_IO)
                return -1;
        }
        ret = slow5_close(out[i]);
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
                  const struct demux_info *d, const opt_t *opt)
{
    core_t *core;
    db_t *db;
    int iseof;
    int ret;
    khint_t n;
    struct kvec_u16 *rec_codes;

    core = demux_core_init(in, out[0]->header->aux_meta, d->rid_map, opt);
    db = demux_db_init(opt->read_id_batch_capacity);
    rec_codes = (struct kvec_u16 *) db->read_group_vector;

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

        ret = demux_write(out, db, rec_codes);
        if (ret)
            return -1;
    }

    if (n < kh_size(d->rid_map)) {
        ERROR("Extra read(s) in custom TSV%s", "");
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
 * corresponding barcode files. Return -1 on error, 0 on success.
 */
static int demux_write(struct slow5_file **out, const db_t *db,
                       const struct kvec_u16 *rec_codes)
{
    int i;
    size_t len;
    uint16_t j;

    for (i = 0; i < (int) db->n_batch; i++) {
        for (j = 0; j < kv_size(rec_codes[i]); j++) {
            len = fwrite(db->read_record[i].buffer, 1, db->read_record[i].len,
                         out[kv_A(rec_codes[i], j)]->fp);
            if (len != (size_t) db->read_record[i].len) {
                ERROR("Failed to write slow5 record%s", "");
                return -1;
            }
        }
        free(db->read_record[i].buffer);
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
        k = kh_put(su16, m, s, &err);
        if (err == -1) {
            ERROR("Failed to put '%s' into hash map", s);
            return -1;
        }
        kh_val(m, k) = kh_size(m) - 1;
        ret = 1;
    }

    *v = kh_val(m, k);
    return ret;
}

/*
 * Copy the auxiliary metadata from one slow5 header to another.
 * Return -1 on error, 0 on success.
 */
static int slow5_aux_meta_copy(const struct slow5_hdr *in_hdr,
                               struct slow5_hdr *out_hdr)
{
    const char *attr;
    enum slow5_aux_type type;
    int ret;
    uint32_t i;

    i = 0;
    while (i < in_hdr->aux_meta->num) {
        attr = in_hdr->aux_meta->attrs[i];
        type = in_hdr->aux_meta->types[i];

        if (type == SLOW5_ENUM || type == SLOW5_ENUM_ARRAY)
            ret = slow5_aux_meta_copy_enum(in_hdr, out_hdr, attr, type);
        else
            ret = slow5_aux_meta_add(out_hdr->aux_meta, attr, type);

        if (ret) {
            ERROR("Failed to copy auxiliary attribute '%s'", attr);
            return -1;
        }
        i++;
    }

    return 0;
}

/*
 * Copy an auxiliary enum attribute from one slow5 header to another.
 * Return -1 on error, 0 on success.
 */
static int slow5_aux_meta_copy_enum(const struct slow5_hdr *in_hdr,
                                    struct slow5_hdr *out_hdr, const char *attr,
                                    enum slow5_aux_type type)
{
    const char **p;
    int ret;
    uint8_t n;

    p = (const char **) slow5_get_aux_enum_labels(in_hdr, attr, &n);
    if (!p)
        return -1;
    ret = slow5_aux_meta_add_enum(out_hdr->aux_meta, attr, type, p, n);
    if (ret)
        return -1;

    return 0;
}

/*
 * Copy the first read group data from one slow5 header to another. If not
 * lossy, copy the auxiliary metadata as well. Return -1 on error, 0 on success.
 */
static int slow5_hdr_copy(const struct slow5_hdr *in_hdr,
                          struct slow5_hdr *out_hdr, int lossy)
{
    int ret;
    khash_t(slow5_s2s) *rg;

    if (!lossy) {
        out_hdr->aux_meta = slow5_aux_meta_init_empty();
        ret = slow5_aux_meta_copy(in_hdr, out_hdr);
        if (ret)
            return -1;
    }

    rg = slow5_hdr_get_data(0, in_hdr);
    ret = slow5_hdr_add_rg_data(out_hdr, rg);
    if (ret < 0) {
        ERROR("Failed to copy read group data%s", "");
        return -1;
    }

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
    khint_t k;
    struct demux_info *d;
    struct kvec_u16 *rid_codes;
    uint16_t i;

    d = demux_info_init();

    code_map = kh_init(su16);
    MALLOC_CHK(code_map);

    ret = bsum_getnext(bs, &rid, &code);
    while (!ret) {
        rid_codes = map_svu16_getpush(d->rid_map, rid);
        if (!rid_codes)
            return NULL;
        if (kv_size(*rid_codes))
            free(rid);

        ret = map_su16_getpush(code_map, code, &i);
        if (!ret)
            free(code);
        else if (ret == -1)
            return NULL;

        vec_chkpush(rid_codes, i);
        ret = bsum_getnext(bs, &rid, &code);
    }
    if (ret == -1)
        return NULL;

    d->count = kh_size(code_map);
    d->codes = (char **) malloc(d->count * sizeof(*d->codes));
    MALLOC_CHK(d->codes);
    for (k = kh_begin(code_map); k != kh_end(code_map); k++) {
        if (kh_exist(code_map, k))
            d->codes[kh_val(code_map, k)] = (char *) kh_key(code_map, k);
    }

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
 * Copy the skeleton of a slow5 file to a given path. The header is duplicated,
 * but otherwise no records are copied over. Output options are used to specify
 * the output format, lossy-ness and compression used. Return NULL on error.
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

    ret = slow5_hdr_copy(in->header, out->header, opt->flag_lossy);
    if (ret)
        return NULL;

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
 * Copy the skeleton of a slow5 file to count new paths appended with an
 * underscore and name before its extension. Return NULL on error.
 */
static struct slow5_file **slow5_spawn(const struct slow5_file *in,
                                       const char **names, uint16_t count,
                                       const opt_t *opt)
{
    char **paths;
    struct slow5_file **out;
    uint16_t i;

    paths = paths_spawn(in->meta.pathname, names, count, opt);
    if (!paths)
        return NULL;

    out = (struct slow5_file **) malloc(count * sizeof (*out));
    MALLOC_CHK(out);

    for (i = 0; i < count; i++) {
        out[i] = slow5_birth(in, paths[i], opt);
        free(paths[i]);
        if (!out[i])
            return NULL;
    }

    free(paths);
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
    const khash_t(svu16) *rid_map;
    int ret;
    khint_t k;
    size_t len;
    struct kvec_u16 *rec_codes;
    struct slow5_press *press;
    struct slow5_rec *rec;

    rec = NULL;
    ret = slow5_rec_depress_parse(db->mem_records + i, db->mem_bytes + i, NULL,
                                  &rec, core->fp);
    free(db->mem_records[i]);
    if (ret)
        exit(EXIT_FAILURE);

    rid_map = (const khash_t(svu16) *) core->param;
    rec_codes = (struct kvec_u16 *) db->read_group_vector;

    k = kh_get(svu16, rid_map, rec->read_id);
    if (k == kh_end(rid_map)) {
        WARNING("Read ID '%s' is missing from custom TSV",
                rec->read_id);
        (void) memset(rec_codes + i, 0, sizeof (*rec_codes));
        (void) memset(db->read_record + i, 0, sizeof (*db->read_record));
    } else {
        rec_codes[i] = kh_val(rid_map, k);

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
    slow5_rec_free(rec);
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
