/*  slow5idx.c -- SLOW5 (FAST5 in TSV) random access.
	adpapted from htslib/faidx.c by Hasindu Gamaarachchi <hasindu@garvan.org.au>
*/


#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>

#include "error.h"
#include "slow5idx.h"

#include "khash.h"
#include "kstring.h"


static inline int isspace_c(char c) { return isspace((unsigned char) c); }
static inline int isdigit_c(char c) { return isdigit((unsigned char) c); }


typedef struct {
    int fd;     //unused fields are for expandability for binary and compressed
    FILE *fp;
    int is_binary;
    int is_compressed;     //unused fields are for expandability for binary and compressed
    int is_gzip;     //unused fields are for expandability for binary and compressed
} SLOW5_FILE;

/**
 * Read one line from a SLOW5 file.
 *
 * @param fp     SLOW5 file handler
 * @param delim  delimitor
 * @param str    string to write to; must be initialized
 * @return       length of the string; -1 on end-of-file; <= -2 on error
 */
static inline int slow5_getline(SLOW5_FILE *fp, int delim, kstring_t *str){

    str->m = 20*1024*1024;
    str->s = (char *)malloc(sizeof(char)*20*1024*1024);
    str->l=getline(&(str->s),&(str->m),fp->fp);
    if(str->l==0 || str->m==0 || str->s==NULL ){
        ERROR("%s\n", "reading issue");
        exit(1);
    }
    return str->l;
}


//later necessary for slow5 binary version
static inline size_t f5read(SLOW5_FILE *fp, kstring_t *str, size_t num_elements){
    str->m = num_elements;
    // TODO need to free?
    str->s = (char *)malloc(sizeof(char)*str->m);
    size_t ret=fread(str->s,sizeof *str->s,num_elements,fp->fp);
    str->l = ret;
    if(ret!=num_elements){
        fprintf(stderr,"Reading error has occurred :%s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ret;
}

/**
 *  Position in uncompressed SLOW5_FILE
 *
 *  @param fp           SLOW5_FILE file handler; must be opened for reading
 *
 *  Returns the current offset on success and -1 on error.
 */
long slow5_utell(SLOW5_FILE *fp){
    return ftell(fp->fp);
}

/**
 * Close the SLOW5_FILE and free all associated resources.
 *
 * @param fp    SLOW5_FILE file handler
 * @return      0 on success and -1 on error
 */
int slow5_close(SLOW5_FILE *fp){
    fclose(fp->fp);
    free(fp);
    return 0;
}

/**
 * Open the specified file for reading or writing.
 */
SLOW5_FILE* slow5_open(const char* path, const char *mode){
    SLOW5_FILE *fp = (SLOW5_FILE *) malloc(sizeof *fp);
    fp->is_binary=0;
    fp->is_compressed=0;
    fp->is_gzip=0;
    fp->fp = fopen(path,mode);
    if(fp->fp==NULL){
        ERROR("File %s cannot be opened\n", path);
        exit(1); // TODO exit or return NULL and handle later?
    }
    return fp;
}



/** Return the file's compression format
 *
 * @param fp  SLOW5_FILE file handle
 * @return    A small integer matching the corresponding
 *            `enum htsCompression` value:
 *   - 0 / `no_compression` if the file is uncompressed
 *   - 1 / `gzip` if the file is plain GZIP-compressed
 *   - 2 / `customzip`
 */
int slow5_compression(SLOW5_FILE *fp){
    return 0;
}

    /**
 *  Position SLOW5_FILE at the uncompressed offset
 *
 *  @param fp           SLOW5_FILE file handler; must be opened for reading
 *  @param uoffset      file offset in the uncompressed data
 *  @param where        SEEK_SET supported atm
 *
 *  Returns 0 on success and -1 on error.
 */
int slow5_useek(SLOW5_FILE *fp, long uoffset, int where)  {
        return fseek(fp->fp, uoffset, SEEK_SET);
}


//following are to be implemented for compressed formats
/**
 * Tell SLOW5_FILE to build index while compressing.
 *
 * @param fp          SLOW5_FILE file handler; can be opened for reading or writing.
 *
 * Returns 0 on success and -1 on error.
 */
int slow5_index_build_init(SLOW5_FILE *fp){
    ERROR("%s\n", "Not implemented");
    exit(1);
}

/// Save SLOW5_FILE index
/**
 * @param fp          SLOW5_FILE file handler
 * @param bname       base name
 * @param suffix      suffix to add to bname (can be NULL)
 * @return 0 on success and -1 on error.
 */
int slow5_index_dump(SLOW5_FILE *fp,
                    const char *bname, const char *suffix) {
    ERROR("%s\n", "Not implemented");
    exit(1);
                    }

/// Load SLOW5_FILE index
/**
 * @param fp          SLOW5_FILE file handler
 * @param bname       base name
 * @param suffix      suffix to add to bname (can be NULL)
 * @return 0 on success and -1 on error.
 */
int slow5_index_load(SLOW5_FILE *fp,
                    const char *bname, const char *suffix){
    ERROR("%s\n", "Not implemented");
    exit(1);
}




typedef struct {
    uint64_t slow5_record_size;
    uint64_t slow5_record_offset;
} slow5idx1_t;
KHASH_MAP_INIT_STR(s, slow5idx1_t)


struct __slow5idx_t {
    SLOW5_FILE *slow5;
    int n, m;
    char **name;
    khash_t(s) *hash;
    enum slow5idx_format_options format;
};

#ifndef kroundup32
#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#endif

static inline int slow5idx_insert_index(slow5idx_t *idx, const char *name, uint64_t slow5_record_size,  uint64_t slow5_record_offset)
{
    if (!name) {
        ERROR("%s","Malformed line");
        return -1;
    }

    char *name_key = strdup(name);
    int absent;
    khint_t k = kh_put(s, idx->hash, name_key, &absent);
    slow5idx1_t *v = &kh_value(idx->hash, k);

    if (!absent) {
        WARNING("Ignoring duplicate read ID \"%s\" at byte offset %" PRIu64 "", name, slow5_record_offset);
        free(name_key);
        return 0;
    }

    if (idx->n == idx->m) {
        char **tmp;
        idx->m = idx->m ? idx->m<<1 : 16;
        if (!(tmp = (char**)realloc(idx->name, sizeof(char*) * idx->m))) {
            ERROR("%s","Out of memory");
            return -1;
        }
        idx->name = tmp;
    }
    idx->name[idx->n++] = name_key;
    v->slow5_record_size = slow5_record_size;
    v->slow5_record_offset = slow5_record_offset;

    return 0;
}

slow5idx_format_options slow5_get_format(SLOW5_FILE *slow5) {

    kstring_t linebuffer = { 0, 0, NULL };
    slow5idx_format_options ret = SLOW5IDX_ASCII; // TODO make default 'no format found' format

    // Check for file format type
    if (slow5_getline(slow5, '\n', &linebuffer) > 0) {
        // Split "##file_format=[NAME]v[VERSION]"
        
        bool bad_header = true;
        char *format = strtok(linebuffer.s, "="); // "#file_format"

        if (format != NULL && strcmp(format, GLOBAL_HEADER_PREFIX FILE_FORMAT_HEADER) == 0) {
            format = strtok(NULL, "="); // "[NAME]v[VERSION]"

            if (format != NULL) {
                char *filetype = strtok(format, "v"); // "[NAME]"

                if (filetype != NULL) {
                    for (size_t i = 0; i < sizeof(formats)/sizeof(*formats); ++ i) {
                        if (strcmp(filetype, formats[i].name) == 0) {
                            bad_header = false;
                            ret = formats[i].format;
                            break;
                        }
                    }
                }
            }
        }

        if (bad_header) {
            // TODO deal better with bad header
            ERROR("bad file type%s", "");
            exit(1);
        }
    }

    // Place fp to beginning
    // TODO needed?
    //slow5_useek(slow5, 0L, SEEK_SET);

    return ret;
}

static slow5idx_t *slow5idx_build_core(SLOW5_FILE *slow5) {

    kstring_t name = { 0, 0, NULL };
    kstring_t linebuffer = { 0, 0, NULL };
    //int c, read_done, line_num;
    slow5idx_t *idx;
    uint64_t slow5_record_offset;
    uint64_t cl, slow5_record_size, ll;
    //enum read_state {OUT_READ, IN_NAME, IN_SEQ, SEQ_END, IN_QUAL} state;

    idx = (slow5idx_t*)calloc(1, sizeof(slow5idx_t));
    idx->hash = kh_init(s);
    //idx->format = SLOW5IDX_ASCII;

    //state = OUT_READ, read_done = 0, line_num = 1;
    slow5_record_offset = cl = slow5_record_size = ll = 0;

    linebuffer.l=0;

    idx->format = slow5_get_format(slow5);

    // TODO empty file?
    
    if (idx->format == SLOW5IDX_ASCII) {

        while (slow5_getline(slow5, '\n', &linebuffer) > 0) {
            if (linebuffer.s[0] == '#' || linebuffer.s[0] == '\n' || linebuffer.s[0] == '\r') { //comments and header
                //fprintf(stderr,"%s\n",linebuffer.s);
                //line_num++;
                linebuffer.l = 0; // TODO why resetting it here
                slow5_record_offset = slow5_utell(slow5);
                continue;

            } else {
                char *name = strtok(linebuffer.s, "\t");
                slow5_record_size = linebuffer.l;
                //fprintf(stderr,"%s %ld\n",name,slow5_record_offset);
                if (slow5idx_insert_index(idx, name, slow5_record_size, slow5_record_offset) != 0){
                    goto slow5idxl;
                }
                slow5_record_size = 0;
                slow5_record_offset = slow5_utell(slow5);
                //line_num++;
                linebuffer.l = 0; // TODO why resetting it here
                //name.l=0;
            }
        }

    } else if (idx->format == SLOW5IDX_BINARY) {
        
        bool past_header = false;
        while (!past_header && slow5_getline(slow5, '\n', &linebuffer) > 0) {
            if (linebuffer.l > 1 && linebuffer.s[1] != '#') { // column header reached
                past_header = true;
            }
        }
        
        slow5_record_offset = slow5_utell(slow5);

        // Get size of file
        struct stat buf;
        if (fstat(fileno(slow5->fp), &buf) == -1) {
            ERROR("fstat failed%s", ""); // TODO change
        }

        while (slow5_record_offset < buf.st_size) {
            slow5_record_size = 0;

            // TODO do ERROR handling

            // Obtain read id
            size_t read_id_len = 0;
            if (fread(&read_id_len, sizeof read_id_len, 1, slow5->fp) <= 0) {
                //ERROR
            }
            if (f5read(slow5, &linebuffer, read_id_len) <= 0) {
                //ERROR
            }
            
            // Get size of record

            uint64_t nsamples = 0;
            if (fread(&nsamples, sizeof nsamples, 1, slow5->fp) <= 0) {
                //ERROR
            }
            size_t bytes_to_raw_signal = sizeof ((fast5_t){0}).digitisation + 
                sizeof ((fast5_t){0}).offset +
                sizeof ((fast5_t){0}).range +
                sizeof ((fast5_t){0}).sample_rate +
                sizeof *((fast5_t){0}).rawptr * nsamples +
                sizeof (uint64_t); // num_bases size TODO remove or put in header

            if (fseek(slow5->fp, bytes_to_raw_signal, SEEK_CUR) == -1) {
                //ERROR
            }

            // Obtain sequence length
            size_t sequence_len = 0;
            if (fread(&sequence_len, sizeof sequence_len, 1, slow5->fp) <= 0) {
                //ERROR
            }
            if (fseek(slow5->fp, sequence_len * sizeof (char), SEEK_CUR) == -1) {
                //ERROR
            }

            // Obtain fast5_path length
            size_t fast5_path_len = 0;
            if (fread(&fast5_path_len, sizeof fast5_path_len, 1, slow5->fp) <= 0) {
                //ERROR
            }
            if (fseek(slow5->fp, fast5_path_len * sizeof (char), SEEK_CUR) == -1) {
                //ERROR
            }

            long curr_pos = slow5_utell(slow5);
            slow5_record_size = curr_pos - slow5_record_offset;

            if (slow5idx_insert_index(idx, linebuffer.s, slow5_record_size, slow5_record_offset) != 0){
                goto slow5idxl;
            }
            slow5_record_offset = curr_pos;
        }
    }


    free(name.s);
    free(linebuffer.s);
    return idx;

slow5idxl:
    free(name.s);
    free(linebuffer.s);
    slow5idx_destroy(idx);
    return NULL;
}


static int slow5idx_save(const slow5idx_t *slow5idx, FILE *fp) {
    khint_t k;
    int i;
    char buf[96]; // Must be big enough for format below.

    for (i = 0; i < slow5idx->n; ++i) {
        slow5idx1_t x;
        k = kh_get(s, slow5idx->hash, slow5idx->name[i]);
        assert(k < kh_end(slow5idx->hash));
        x = kh_value(slow5idx->hash, k);

        if (slow5idx->format == SLOW5IDX_ASCII) {
            snprintf(buf, sizeof(buf),
                 "\t%" PRIu64 "\t%" PRIu64 "\n",
                 x.slow5_record_offset, x.slow5_record_size);
        } else if (slow5idx->format == SLOW5IDX_BINARY) {
            snprintf(buf, sizeof(buf),
                 "\t%" PRIu64 "\t%" PRIu64 "\n",
                 x.slow5_record_offset, x.slow5_record_size);
        } else {
            assert(0);
        }

        if (fputs(slow5idx->name[i], fp) < 0) return -1;
        if (fputs(buf, fp) < 0) return -1;
    }
    return 0;
}


static slow5idx_t *slow5idx_read(FILE *fp, const char *fname, int format)
{
    slow5idx_t *slow5idx;
    char *buf = NULL, *p;
    ssize_t l, lnum = 1;
    size_t buf_size = 0x10000;

    slow5idx = (slow5idx_t*)calloc(1, sizeof(slow5idx_t));
    if (!slow5idx) return NULL;

    slow5idx->hash = kh_init(s);
    if (!slow5idx->hash) goto slow5idxl;

    buf = (char*)calloc(buf_size, 1);
    if (!buf) goto slow5idxl;

    while ((l = getline(&buf, &buf_size, fp)) > 0) {
        uint64_t slow5_record_size,  n;
        uint64_t slow5_record_offset;

        for (p = buf; *p && !isspace_c(*p); ++p);

        if (p - buf < l) {
            *p = 0; ++p;
        }

        if (format == SLOW5IDX_ASCII || format == SLOW5IDX_BINARY) {
            n = sscanf(p, "%" SCNu64 "%" SCNu64 "", &slow5_record_offset,  &slow5_record_size);

            if (n != 2) {
                ERROR("Could not understand SLOW5 index %s line %zd", fname, lnum);
                goto slow5idxl;
            }
        } else {
            assert(0);
        }

        if (slow5idx_insert_index(slow5idx, buf, slow5_record_size,  slow5_record_offset) != 0) {
            goto slow5idxl;
        }

        if (buf[l - 1] == '\n') ++lnum;
    }

    free(buf);
    return slow5idx;

 slow5idxl:
    free(buf);
    slow5idx_destroy(slow5idx);
    return NULL;
}

void slow5idx_destroy(slow5idx_t *slow5idx)
{
    int i;
    if (!slow5idx) return;
    for (i = 0; i < slow5idx->n; ++i) free(slow5idx->name[i]);
    free(slow5idx->name);
    kh_destroy(s, slow5idx->hash);
    if (slow5idx->slow5) slow5_close(slow5idx->slow5);
    free(slow5idx);
}


static int slow5idx_build3_core(const char *fn, const char *fnslow5idx, const char *fngzi)
{
    kstring_t slow5idx_kstr = { 0, 0, NULL };
    kstring_t gzi_kstr = { 0, 0, NULL };
    SLOW5_FILE *slow5 = NULL;
    FILE *fp = NULL;
    slow5idx_t *slow5idx = NULL;
    int save_errno, res;
    const char *file_type;

    slow5 = slow5_open(fn, "r");

    if ( !slow5 ) {
        ERROR("Failed to open the file %s", fn);
        goto slow5idxl;
    }

    if ( slow5->is_compressed ) {
        if (slow5_index_build_init(slow5) != 0) {
            ERROR("%s","Failed to allocate slow5 index");
            goto slow5idxl;
        }
    }

    slow5idx = slow5idx_build_core(slow5);

    if ( !slow5idx ) {
        if (slow5->is_compressed && slow5->is_gzip) {
            ERROR("%s","Cannot index files compressed with gzip, please use bgzip");
        }
        goto slow5idxl;
    }

    if (slow5idx->format == SLOW5IDX_ASCII) {
        file_type   = "SLOW5_ASCII";
        if (!fnslow5idx) {
            if (ksprintf(&slow5idx_kstr, "%s.s5i", fn) < 0) goto slow5idxl;
            fnslow5idx = slow5idx_kstr.s;
        }
    } else if (slow5idx->format == SLOW5IDX_BINARY) {
        file_type   = "SLOW5_BINARY";
        if (!fnslow5idx) {
            if (ksprintf(&slow5idx_kstr, "%s.b5i", fn) < 0) goto slow5idxl;
            fnslow5idx = slow5idx_kstr.s;
        }
    } else {
        assert(0);
    }

    if (!fngzi) {
        if (ksprintf(&gzi_kstr, "%s.gzi", fn) < 0) goto slow5idxl;
        fngzi = gzi_kstr.s;
    }

    if ( slow5->is_compressed ) {
        if (slow5_index_dump(slow5, fngzi, NULL) < 0) {
            ERROR("Failed to make slow5 index %s", fngzi);
            goto slow5idxl;
        }
    }

    res = slow5_close(slow5);
    slow5 = NULL;

    if (res < 0) {
        ERROR("Error on closing %s : %s", fn, strerror(errno));
        goto slow5idxl;
    }

    fp = fopen(fnslow5idx, "wb");

    if ( !fp ) {
        ERROR("Failed to open %s index %s : %s", file_type, fnslow5idx, strerror(errno));
        goto slow5idxl;
    }

    if (slow5idx_save(slow5idx, fp) != 0) {
        ERROR("Failed to write %s index %s : %s", file_type, fnslow5idx, strerror(errno));
        goto slow5idxl;
    }

    if (fclose(fp) != 0) {
        ERROR("Failed on closing %s index %s : %s", file_type, fnslow5idx, strerror(errno));
        goto slow5idxl;
    }

    free(slow5idx_kstr.s);
    free(gzi_kstr.s);
    slow5idx_destroy(slow5idx);
    return 0;

 slow5idxl:
    save_errno = errno;
    free(slow5idx_kstr.s);
    free(gzi_kstr.s);
    slow5_close(slow5);
    slow5idx_destroy(slow5idx);
    errno = save_errno;
    return -1;
}

//this wrapper is there for future expandability for a library API to allow custom file extensions and multiplexing
int slow5idx_build3(const char *fn, const char *fnslow5idx, const char *fngzi) {
    return slow5idx_build3_core(fn, fnslow5idx, fngzi);
}

//this wrapper is there for future expandability for a library API to allow custom file extensions and multiplexing
int slow5idx_build(const char *fn) {
    return slow5idx_build3(fn, NULL, NULL);
}


static slow5idx_t *slow5idx_load3_core(const char *fn, const char *fnslow5idx, const char *fngzi,
                   int flags, int format)
{
    kstring_t slow5idx_kstr = { 0, 0, NULL };
    kstring_t gzi_kstr = { 0, 0, NULL };
    FILE *fp = NULL;
    slow5idx_t *slow5idx = NULL;
    int res, gzi_index_needed = 0;
    const char *file_type;

    // TODO refactor this
    SLOW5_FILE *slow5 = slow5_open(fn, "rb");
    format = slow5_get_format(slow5);

    if (format == SLOW5IDX_ASCII) {
        file_type   = "SLOW5_ASCII";
        if (fnslow5idx == NULL) {
            if (ksprintf(&slow5idx_kstr, "%s.s5i", fn) < 0) goto slow5idxl;
            fnslow5idx = slow5idx_kstr.s;
        }
    } else if (format == SLOW5IDX_BINARY) {
        file_type   = "SLOW5_BINARY";
        if (fnslow5idx == NULL) {
            if (ksprintf(&slow5idx_kstr, "%s.b5i", fn) < 0) goto slow5idxl;
            fnslow5idx = slow5idx_kstr.s;
        }
    } else {
        assert(0);
    }

    if (fn == NULL)
        return NULL;

    if (fngzi == NULL) {
        if (ksprintf(&gzi_kstr, "%s.gzi", fn) < 0) goto slow5idxl;
        fngzi = gzi_kstr.s;
    }

    fp = fopen(fnslow5idx, "rb");

    if (fp) {
        // index file present, check if a compressed index is needed
        FILE *gz = NULL;
        SLOW5_FILE *slow5 = slow5_open(fn, "rb");

        if (slow5 == 0) {
            ERROR("Failed to open %s file %s", file_type, fn);
            goto slow5idxl;
        }

        if (slow5_compression(slow5) == 2) { // SLOW5_FILE compression
            if ((gz = fopen(fngzi, "rb")) == 0) {

                if (!(flags & SLOW5IDX_CREATE) || errno != ENOENT) {
                    ERROR("Failed to open %s index %s: %s", file_type, fngzi, strerror(errno));
                    slow5_close(slow5);
                    goto slow5idxl;
                }

                gzi_index_needed = 1;
                res = fclose(fp); // closed as going to be re-indexed

                if (res < 0) {
                    ERROR("Failed on closing %s index %s : %s", file_type, fnslow5idx, strerror(errno));
                    goto slow5idxl;
                }
            } else {
                res = fclose(gz);

                if (res < 0) {
                    ERROR("Failed on closing %s index %s : %s", file_type, fngzi, strerror(errno));
                    goto slow5idxl;
                }
            }
        }

        slow5_close(slow5);
    }

    if (fp == 0 || gzi_index_needed) {
        if (!(flags & SLOW5IDX_CREATE) || errno != ENOENT) {
            ERROR("Failed to open %s index %s: %s", file_type, fnslow5idx, strerror(errno));
            goto slow5idxl;
        }

        INFO("Build %s index", file_type);

        if (slow5idx_build3_core(fn, fnslow5idx, fngzi) < 0) {
            goto slow5idxl;
        }

        fp = fopen(fnslow5idx, "rb");
        if (fp == 0) {
            ERROR("Failed to open %s index %s: %s", file_type, fnslow5idx, strerror(errno));
            goto slow5idxl;
        }
    }

    slow5idx = slow5idx_read(fp, fnslow5idx, format);
    if (slow5idx == NULL) {
        ERROR("Failed to read %s index %s", file_type, fnslow5idx);
        goto slow5idxl;
    }

    res = fclose(fp);
    fp = NULL;
    if (res < 0) {
        ERROR("Failed on closing %s index %s : %s", file_type, fnslow5idx, strerror(errno));
        goto slow5idxl;
    }

    slow5idx->slow5 = slow5_open(fn, "rb");
    if (slow5idx->slow5 == 0) {
        ERROR("Failed to open %s file %s", file_type, fn);
        goto slow5idxl;
    }

    if ( slow5idx->slow5->is_compressed==1 ) {
        if ( slow5_index_load(slow5idx->slow5, fngzi, NULL) < 0 ) {
            ERROR("Failed to load .gzi index: %s", fngzi);
            goto slow5idxl;
        }
    }
    free(slow5idx_kstr.s);
    free(gzi_kstr.s);
    return slow5idx;

 slow5idxl:
    if (slow5idx) slow5idx_destroy(slow5idx);
    if (fp) fclose(fp);
    free(slow5idx_kstr.s);
    free(gzi_kstr.s);
    return NULL;
}


slow5idx_t *slow5idx_load3(const char *fn, const char *fnslow5idx, const char *fngzi,
                   int flags) {
    return slow5idx_load3_core(fn, fnslow5idx, fngzi, flags, SLOW5IDX_ASCII); // TODO change format option
}


slow5idx_t *slow5idx_load(const char *fn)
{
    return slow5idx_load3(fn, NULL, NULL, SLOW5IDX_CREATE);
}


static char *slow5idx_retrieve(const slow5idx_t *slow5idx, const slow5idx1_t *val,
                          uint64_t offset, long beg, long end, int *len) {
    char *s;
    size_t l;
    int c = 0;
    // int ret = slow5_useek(slow5idx->slow5,
    //                      offset
    //                      + beg / val->line_blen * val->slow5_record_size
    //                      + beg % val->line_blen, SEEK_SET);
    int ret = slow5_useek(slow5idx->slow5,
                         offset, SEEK_SET);
    if (ret < 0) {
        *len = -1;
        ERROR("%s","Failed to retrieve block. (Seeking in a compressed, .gzi unindexed, file?)");
        return NULL;
    }

    // l = 0;
    // s = (char*)malloc((size_t) end - beg + 2);
    // if (!s) {
    //     *len = -1;
    //     return NULL;
    // }

    kstring_t linebuffer = { 0, 0, NULL };
    //ret=slow5_getline(slow5idx->slow5, '\n', &linebuffer);
    
    // testing
    /*printf("%ld\n", offset);
    ret = slow5_useek(slow5idx->slow5,
                         2, SEEK_SET);
    char *line = (char*) malloc(3000 + 1);
    fread(line, 3000, 1, slow5idx->slow5->fp);
    printf("%s\n", line);
    return line;
    */

    ret = f5read(slow5idx->slow5, &linebuffer, end);

    // while ( l < end - beg && (c=slow5_getc(slow5idx->slow5))>=0 )
    //     if (isgraph(c)) s[l++] = c;
    // if (c < 0) {
    if(ret<0){
        ERROR("Failed to retrieve block: %s",
            c == -1 ? "unexpected end of file" : "error reading file");
        if(linebuffer.s){
            free(linebuffer.s);
        }
        *len = -1;
        return NULL;
    }

    l=linebuffer.l;
    s=linebuffer.s;
    //s[l] = '\0';
    *len = l < INT_MAX ? l : INT_MAX;
    return s;
}


//todo: no need to parse a region string. only need to parse a readID
static int slow5idx_get_val(const slow5idx_t *slow5idx, const char *str, int *len, slow5idx1_t *val, long *fbeg, long *fend) {
    char *s, *ep;
    size_t i, l, k, name_end;
    khiter_t iter;
    khash_t(s) *h;
    long beg, end;

    beg = end = -1;
    h = slow5idx->hash;
    name_end = l = strlen(str);
    s = (char*)malloc(l+1);
    if (!s) {
        *len = -1;
        return 1;
    }

    // remove space
    for (i = k = 0; i < l; ++i)
        if (!isspace_c(str[i])) s[k++] = str[i];
    s[k] = 0;
    name_end = l = k;
    // determine the sequence name
    for (i = l; i > 0; --i) if (s[i - 1] == ':') break; // look for colon from the end
    if (i > 0) name_end = i - 1;
    if (name_end < l) { // check if this is really the end
        int n_hyphen = 0;
        for (i = name_end + 1; i < l; ++i) {
            if (s[i] == '-') ++n_hyphen;
            else if (!isdigit_c(s[i]) && s[i] != ',') break;
        }
        if (i < l || n_hyphen > 1) name_end = l; // malformated region string; then take str as the name
        s[name_end] = 0;
        iter = kh_get(s, h, s);
        if (iter == kh_end(h)) { // cannot find the sequence name
            iter = kh_get(s, h, str); // try str as the name
            if (iter != kh_end(h)) {
                s[name_end] = ':';
                name_end = l;
            }
        }
    } else iter = kh_get(s, h, str);
    if(iter == kh_end(h)) {
        WARNING("Read ID %s not found in file, returning empty record", str);
        free(s);
        *len = -2;
        return 1;
    }
    *val = kh_value(h, iter);
    // parse the interval
    if (name_end < l) {
        int save_errno = errno;
        errno = 0;
        for (i = k = name_end + 1; i < l; ++i)
            if (s[i] != ',') s[k++] = s[i];
        s[k] = 0;
        if (s[name_end + 1] == '-') {
            beg = 0;
            i = name_end + 2;
        } else {
            beg = strtol(s + name_end + 1, &ep, 10);
            for (i = ep - s; i < k;) if (s[i++] == '-') break;
        }
        end = i < k? strtol(s + i, &ep, 10) : val->slow5_record_size;
        if (beg > 0) --beg;
        // Check for out of range numbers.  Only going to be a problem on
        // 32-bit platforms with >2Gb sequence length.
        if (errno == ERANGE && (uint64_t) val->slow5_record_size > LONG_MAX) {
            ERROR("Positions in range %s are too large for this platform", s);
            free(s);
            *len = -3;
            return 1;
        }
        errno = save_errno;
    } else beg = 0, end = val->slow5_record_size;
    if (beg >= val->slow5_record_size) beg = val->slow5_record_size;
    if (end >= val->slow5_record_size) end = val->slow5_record_size;
    if (beg > end) beg = end;
    free(s);

    *fbeg = beg;
    *fend = end;

    return 0;
}


char *slow5idx_fetch(const slow5idx_t *slow5idx, const char *str, int *len)
{
    slow5idx1_t val;
    long beg, end;

    if (slow5idx_get_val(slow5idx, str, len, &val, &beg, &end)) {
        return NULL;
    }

    // now retrieve the slow5 record
    return slow5idx_retrieve(slow5idx, &val, val.slow5_record_offset, beg, end, len);
}
