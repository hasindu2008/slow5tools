/**
 * @file demux.c
 * @brief demultiplex a slow5 file
 * @author Sasha Jenner (me AT sjenner DOT com)
 * @date TODO
 * TODO: check each malloc has a free, code review, minimal testing, compile,
 * spell check
 */
#include <stdint.h>
#include "demux.h"
#include "error.h"
#include "khash.h"
#include "kvec.h"
#include "read_fast5.h"
#include "slow5_extra.h"
#include "thread.h"

extern int slow5tools_verbosity_level;

typedef kvec_t(uint16_t) kvec_uint16_t;
KHASH_MAP_INIT_STR(su16, uint16_t);
KHASH_MAP_INIT_STR(svu16, kvec_uint16_t); // TODO change internal vector buffer size

struct bsum {
    FILE *fp;
    char *line;        // Buffer for file parsing
    uint16_t code_pos; // Barcode arrangement column number
    uint16_t prid_pos; // Parent read ID column number
    size_t n;          // Buffer size
};

struct demux_info {
    char **codes;             // Barcode arrangements
    khash_t(svu16) *prid_map; // Hash map of parent read ID to barcode indices
    uint16_t count;           // Number of unique barcode arrangements
};

static char *path_append(const char *path, const char *suf);
static char *path_move(const char *path, const char *dir);
static char *path_move_append(const char *path, const char *dir,
                              const char *suf);
static core_t *demux_core_init(struct slow5_file *in,
                               struct slow5_aux_meta *aux_meta,
                               khash_t(svu16) *prid_map, const opt_t *opt);
static db_t *demux_db_init(int n);
static int bsum_close(struct bsum *bs);
static int bsum_getnext(struct bsum *bs, char **prid, char **code);
static int bsum_parsehdr(struct bsum *bs);
static int demux2(struct slow5_file *in, const struct demux_info *d,
                  const opt_t *opt);
static int demux3(struct slow5_file *in, struct slow5_file **out,
                  const struct demux_info *d, const opt_t *opt);
static int demux_db_setup(db_t *db, const struct slow5_file *in, int max);
static int demux_write(struct slow5_file **out, const db_t *db,
                       const kvec_uint16_t *rec_codes);
static int ext_fix(char *path, enum slow5_fmt fmt);
static int map_su16_get(khash_t(su16) *m, char *s, uint16_t *v);
static int slow5_aux_meta_copy(const struct slow5_file *in,
                               struct slow5_file *out);
static kvec_uint16_t *map_svu16_get(khash_t(svu16) *m, char *s);
static struct bsum *bsum_open(const char *bsum_path);
static struct demux_info *demux_info_init(void);
static struct demux_info *demux_info_get(const char *bsum_path);
static struct demux_info *demux_info_get2(struct bsum *bs);
static struct slow5_file *slow5_birth(const struct slow5_file *in,
                                      const char *path, const opt_t *opt);
static struct slow5_file **slow5_spawn(const struct slow5_file *in,
                                       char **names, uint16_t count,
                                       const opt_t *opt);
static void demux_db_destroy(db_t *db);
static void demux_info_destroy(struct demux_info *d);
static void demux_setup(core_t *core, db_t *db, int i);
static void map_svu16_destroy(khash_t(svu16) *m);
static void underscore_prepend(const char *s, char **out, size_t *n);
static void vec_chkpush(kvec_uint16_t *v, uint16_t x);

/*
 * Demultiplex a slow5 file given the barcode summary file path and user
 * options. Return -1 on error, 0 on success.
 */
int demux(struct slow5_file *in, const char *bsum_path, const opt_t *opt)
{
    int ret;
    struct demux_info *d;

    d = demux_info_get(bsum_path);
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

    ext = strrchr(path, '.');
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

    base = strrchr(path, '/');
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
        ret[dirlen - 1] = '/';
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
 * Initialise the demultiplexing multithreading core.
 */
static core_t *demux_core_init(struct slow5_file *in,
                               struct slow5_aux_meta *aux_meta,
                               khash_t(svu16) *prid_map, const opt_t *opt)
{
    core_t *c;

    c = (core_t *) calloc(1, sizeof (*c));
    MALLOC_CHK(c);

    c->aux_meta = aux_meta;
    c->format_out = opt->fmt_out;
    c->fp = in;
    c->lossy = opt->flag_lossy;
    c->num_thread = opt->num_threads;
    c->param = (void *) prid_map;
    c->press_method.record_method = opt->record_press_out;
    c->press_method.signal_method = opt->signal_press_out;

    return c;
}

/*
 * Initialise the demultiplexing multithreading database.
 */
static db_t *demux_db_init(int n)
{
    db_t *db;

    db = (db_t *) calloc(1, sizeof (*db));
    MALLOC_CHK(db);

    db->mem_bytes = (size_t *) malloc(n * sizeof (*db->mem_bytes));
    db->mem_records = (char **) malloc(n * sizeof (*db->mem_records));
    db->read_record = (raw_record_t *) malloc(n * sizeof (*db->read_record));
    db->read_group_vector = (uint32_t *) malloc(n * sizeof (kvec_uint16_t));

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
        ERROR("Failed to close barcode summary file: %s",
              strerror(errno));
        return -1;
    }
    free(bs->line);
    free(bs);

    return 0;
}

/*
 * Get the next parent read ID and barcode arrangement from the barcode summary
 * file. *prid and *code are malloced pointers to be freed by the caller.
 * Return -1 on error, 0 on success, 1 on end of file.
 */
static int bsum_getnext(struct bsum *bs, char **prid, char **code)
{
    char *tok;
    int ret;
    ssize_t nread;
    uint16_t i;

    nread = getline(&bs->line, &bs->n, bs->fp);
    if (nread == -1) {
        ret = feof(bs->fp);
        if (ret)
            return 1;
        ERROR("Failed to read barcode summary line: %s", strerror(errno));
        return -1;
    }

    i = 1;
    tok = strtok(bs->line, BSUM_DELIM);
    while (tok && (i <= bs->code_pos || i <= bs->prid_pos)) {
        if (i == bs->prid_pos) {
            *prid = strdup(tok);
            if (!*prid) {
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
        ERROR("Failed to read barcode summary header: %s", strerror(errno));
        return -1;
    }

    i = 1;
    bs->code_pos = 0;
    bs->prid_pos = 0;

    tok = strtok(bs->line, BSUM_DELIM);
    while (tok && BSUM_HEADER_MISSING(bs)) {
        if (!bs->prid_pos) {
            ret = strcmp(tok, BSUM_HEADER_PARENT_READID);
            if (!ret)
                bs->prid_pos = i;
        } else if (!bs->code_pos) {
            ret = strcmp(tok, BSUM_HEADER_BARCODE);
            if (!ret)
                bs->code_pos = i;
        }
        tok = strtok(NULL, BSUM_DELIM);
        i++;
    }
    if (BSUM_HEADER_MISSING(bs)) {
        ERROR("Invalid barcode summary header%s", "")
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

    out = slow5_spawn(in, d->codes, d->count, opt);
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
    kvec_uint16_t *rec_codes;

    core = demux_core_init(in, out[0]->header->aux_meta, d->prid_map, opt);
    db = demux_db_init(opt->read_id_batch_capacity);
    rec_codes = (kvec_uint16_t *) db->read_group_vector;

    iseof = 0;
    while (!iseof) {
        ret = demux_db_setup(db, in, opt->read_id_batch_capacity);
        if (ret == 1)
            iseof = 1;
        else if (ret == -1)
            return -1;

        work_db(core, db, demux_setup);

        ret = demux_write(out, db, rec_codes);
        if (ret)
            return -1;
    }

    free(core);
    demux_db_destroy(db);

    return 0;
}

/*
 * Store the next max records in the demultiplexing multithreading database.
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
 * Write the demultiplexing multithreading database records to their
 * corresponding barcode files. Return -1 on error, 0 on success.
 */
static int demux_write(struct slow5_file **out, const db_t *db,
                       const kvec_uint16_t *rec_codes)
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
 * Write the format's slow5 extension to the path. The path should have enough
 * memory allocated to write the extension. Return -1 on error, 0 on success.
 */
static int ext_fix(char *path, enum slow5_fmt fmt)
{
    char *ext;
    const char *name;

    ext = strrchr(path, '.');
    if (!ext) {
        ERROR("Path '%s' has no extension", path);
        return -1;
    }
    ext++;

    name = slow5_fmt_get_name(fmt);
    if (!name) {
        ERROR("Unknown slow5 format '%d'", fmt);
        return -1;
    }

    (void) strcpy(ext, name);

    return 0;
}

/*
 * Get the value of map m at key s. Set *v to the value. If s does not exist,
 * add it to m with the number of elements - 1 as its value.
 * Return -1 on error, 0 on key already exists, 1 on key was added.
 */
static int map_su16_get(khash_t(su16) *m, char *s, uint16_t *v)
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
 * Copy the auxilliary metadata from in to out.
 * Return -1 on error, 0 on success.
 */
static int slow5_aux_meta_copy(const struct slow5_file *in,
                               struct slow5_file *out)
{
    const char **p;
    char *attr;
    enum slow5_aux_type type;
    int ret;
    uint32_t i;
    uint8_t n;

    i = 0;
    ret = 0;
    while (!ret && i < in->header->aux_meta->num) {
        attr = in->header->aux_meta->attrs[i];
        type = in->header->aux_meta->types[i];

        if (type == SLOW5_ENUM || type == SLOW5_ENUM_ARRAY) {
            p = (const char **) slow5_get_aux_enum_labels(in->header, attr, &n);
            if (!p)
                ret = -1;
            else
                ret = slow5_aux_meta_add_enum(out->header->aux_meta, attr, type,
                                              p, n);
        } else {
            ret = slow5_aux_meta_add(out->header->aux_meta, attr, type);
        }

        if (ret) {
            ERROR("Failed to copy auxilliary attribute '%s'", attr);
            ret = -1;
        }
        i++;
    }
    return ret;
}

/*
 * Get the value of map m at key s. If s does not exist, add it to m and
 * initialise its value to a new vector. Return NULL on error.
 */
static kvec_uint16_t *map_svu16_get(khash_t(svu16) *m, char *s)
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
 * Open a barcode summary file and parse the header. Return NULL on error.
 */
static struct bsum *bsum_open(const char *bsum_path)
{
    int ret;
    struct bsum *bs;

    bs = (struct bsum *) malloc(sizeof (*bs));
    MALLOC_CHK(bs);

    bs->fp = fopen(bsum_path, "r");
    if (!bs->fp) {
        ERROR("Failed to open '%s': %s", bsum_path, strerror(errno));
        return NULL;
    }

    bs->line = NULL;
    bs->n = 0;

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
    d->prid_map = kh_init(svu16);
    MALLOC_CHK(d->prid_map);

    return d;
}

/*
 * Get the demultiplexing information given the barcode summary file path.
 * Return NULL on error.
 */
static struct demux_info *demux_info_get(const char *bsum_path)
{
    int ret;
    struct bsum *bs;
    struct demux_info *d;

    bs = bsum_open(bsum_path);
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
 * Get the demultiplexing information given the barcode summary file structure.
 * Create a temporary hash map from barcode arrangement to index for querying.
 * Return NULL on error.
 */
static struct demux_info *demux_info_get2(struct bsum *bs)
{
    char *code;
    char *prid;
    int ret;
    khash_t(su16) *code_map;
    khint_t k;
    kvec_uint16_t *prid_codes;
    struct demux_info *d;
    uint16_t i;

    d = demux_info_init();

    code_map = kh_init(su16);
    MALLOC_CHK(code_map);

    ret = bsum_getnext(bs, &prid, &code);
    while (!ret) {
        prid_codes = map_svu16_get(d->prid_map, prid);
        if (!prid_codes)
            return NULL;
        if (kv_size(*prid_codes))
            free(prid);

        ret = map_su16_get(code_map, code, &i);
        if (!ret)
            free(code);
        else if (ret == -1)
            return NULL;
        
        vec_chkpush(prid_codes, i);
        ret = bsum_getnext(bs, &prid, &code);
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
 * Copy the skeleton of a slow5 file to a given path. The header is duplicated,
 * but otherwise no records are copied over. Output options are used to specify
 * the output format, lossy-ness and compression used. Return NULL on error.
 */
static struct slow5_file *slow5_birth(const struct slow5_file *in,
                                      const char *path, const opt_t *opt)
{
    FILE *fp;
    int ret;
    khash_t(slow5_s2s) *rg;
    struct slow5_file *out;
    slow5_press_method_t press_out;

    fp = fopen(path, "w");
    if (!fp) {
        ERROR("Failed to open '%s' for writing: %s", path, strerror(errno));
        return NULL;
    }

    out = slow5_init_empty(fp, path, opt->fmt_out);
    if (!out)
        return NULL;

    ret = slow5_hdr_initialize(out->header, opt->flag_lossy);
    if (ret) {
        ERROR("Failed to initialise the header for '%s'", path);
        return NULL;
    }

    out->header->num_read_groups = 0; // TODO why?

    if (!opt->flag_lossy) {
        ret = slow5_aux_meta_copy(in, out);
        if (ret)
            return NULL;
    }

    rg = slow5_hdr_get_data(0, in->header);
    ret = slow5_hdr_add_rg_data(out->header, rg);
    if (ret < 0) {
        ERROR("Failed to add read group data to '%s'", path);
        return NULL;
    }

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
                                       char **names, uint16_t count,
                                       const opt_t *opt)
{
    char *suf;
    char *out_path;
    int ret;
    size_t len;
    struct slow5_file **out;
    uint16_t i;

    out = (struct slow5_file **) malloc(count * sizeof (*out));
    MALLOC_CHK(out);

    suf = NULL;
    for (i = 0; i < count; i++) {
        underscore_prepend(names[i], &suf, &len);
        out_path = path_move_append(in->meta.pathname, opt->arg_dir_out, suf);
        ret = ext_fix(out_path, opt->fmt_out);
        if (ret)
            return NULL;
        out[i] = slow5_birth(in, out_path, opt);
        free(out_path);
        if (!out[i])
            return NULL;
    }
    free(suf);

    return out;
}

/*
 * Free the demultiplexing multithreading database.
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

    map_svu16_destroy(d->prid_map);
    free(d);
}

/*
 * Decompress, parse and convert the record at index i to the desired output
 * format.
 */
static void demux_setup(core_t *core, db_t *db, int i)
{
    const khash_t(svu16) *prid_map;
    int ret;
    khint_t k;
    kvec_uint16_t *rec_codes;
    size_t len;
    struct slow5_press *press;
    struct slow5_rec *rec;

    rec = NULL;
    ret = slow5_rec_depress_parse(db->mem_records + i, db->mem_bytes + i, NULL,
                                  &rec, core->fp);
    free(db->mem_records[i]);
    if (ret)
        exit(EXIT_FAILURE);

    prid_map = (const khash_t(svu16) *) core->param;
    rec_codes = (kvec_uint16_t *) db->read_group_vector;

    k = kh_get(svu16, prid_map, rec->read_id);
    if (k == kh_end(prid_map)) {
        ERROR("Read ID '%s' is missing from barcode summary file",
              rec->read_id);
        exit(EXIT_FAILURE);
    }
    rec_codes[i] = kh_val(prid_map, k);

    press = slow5_press_init(core->press_method);
    if (!press)
        exit(EXIT_FAILURE);
    if (core->lossy)
        core->aux_meta = NULL;
    db->read_record[i].buffer = slow5_rec_to_mem(rec, core->aux_meta,
                                                 core->format_out, press, &len);
    db->read_record[i].len = (int) len; // TODO maybe this should be size_t
    slow5_press_free(press);
    slow5_rec_free(rec);
    if (!db->read_record[i].buffer)
        exit(EXIT_FAILURE);
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
static void vec_chkpush(kvec_uint16_t *v, uint16_t x)
{
    int found;
    size_t i;

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
