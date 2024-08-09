#ifndef DEMUX_H
#define DEMUX_H

#include <slow5/slow5.h>
#include "misc.h"

#define BSUM_DELIM ("\t")
#define BSUM_HEADER_BARCODE ("barcode_arrangement")
#define BSUM_HEADER_READID ("parent_read_id")
#define BSUM_HEADER_MISSING(bs) (!(bs)->code_pos || !(bs)->rid_pos)
#define PATH_EXT_DELIM ('.')
#define PATH_DIR_DELIM ('/')

struct bsum_meta {
    char *path;
    const char *code_hdr; // Barcodes column header
    const char *rid_hdr;  // Read IDs column header
};

/*
 * Demultiplex a slow5 file given the barcode summary file metadata and user
 * options. Return -1 on error, 0 on success.
 */
int demux(struct slow5_file *in, const struct bsum_meta *bs_meta,
          const opt_t *opt);

#endif /* demux.h */
