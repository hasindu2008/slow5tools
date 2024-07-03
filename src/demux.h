#ifndef DEMUX_H
#define DEMUX_H

#include <slow5/slow5.h>
#include "misc.h"

#define BSUM_DELIM ("\t")
#define BSUM_HEADER_BARCODE ("barcode_arrangement")
#define BSUM_HEADER_PARENT_READID ("parent_read_id")
#define BSUM_HEADER_MISSING(bs) (!(bs)->code_pos || !(bs)->prid_pos)

/*
 * Demultiplex a slow5 file given the barcode summary file path and user
 * options. Return -1 on error, 0 on success.
 */
int demux(struct slow5_file *in, const char *bsum_path, const opt_t *opt);

#endif /* demux.h */
